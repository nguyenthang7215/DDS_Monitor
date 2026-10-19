// SPDX-License-Identifier: GPL-3.0-or-later
#include "MonitoringCore.h"
#include "Protocol.h"
#include "FlowModel.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QSet>
namespace task3 {
QVariantList MonitoringCore::hosts() const {
    auto rows = registry_.rows();
    for (auto& entry : rows) {
        auto row = entry.toMap();
        row["cpu_history"] = cpuHistory_.value(row["host_id"].toString());
        row["alert_count"] = alerts_.activeCount(row["host_id"].toString());
        entry = row;
    }
    return rows;
}
QVariantList MonitoringCore::events() const {
    auto rows = alerts_.rows();
    for (auto& value : rows) {
        auto row = value.toMap();
        if (row["state"] == "Active" && alerts_.isActive(row["rule_id"].toString()) &&
            (row["kind"] == "process_stopped" || row["kind"] == "service_stopped")) {
            const auto hostId = row["rule_id"].toString().section('/', 0, 0);
            const auto host = registry_.hosts().constFind(hostId);
            if (host == registry_.hosts().cend() || !host->sequence)
                row["verification"] = "Not verified — awaiting Agent telemetry";
            else if (!host->online)
                row["verification"] = "Not verified — Agent Offline";
            else {
                const auto applicationId = row["rule_id"].toString().section('/', 1, 1);
                QJsonObject application;
                for (const auto& entry : host->snapshot["applications"].toArray()) {
                    const auto candidate = entry.toObject();
                    if (candidate["application_id"] == applicationId) {
                        application = candidate;
                        break;
                    }
                }
                const auto state =
                    application[row["kind"] == "process_stopped" ? "state" : "service_state"]
                        .toString();
                row["verification"] =
                    state == "stopped" || state == "failed"
                        ? "Verified by current Agent telemetry"
                        : "Not verified — current application/service evidence unavailable";
            }
            value = row;
        }
    }
    return rows;
}
void MonitoringCore::loadOlderEvents() {
    alerts_.loadOlderRows();
    emit changed();
}
MonitoringCore::MonitoringCore(const QString& db, int timeoutMs, QObject* parent)
    : QObject(parent), alerts_(db), timeout_(qMax(100, timeoutMs)) {
    clock_.start();
    timer_.setInterval(500);
    alerts_.policy(qEnvironmentVariableIntValue("TASK3_ALERT_TRIGGER_MS"),
                   qEnvironmentVariableIntValue("TASK3_ALERT_RECOVERY_MS"),
                   qEnvironmentVariableIntValue("TASK3_ALERT_COOLDOWN_MS"));
    registry_.expect(qEnvironmentVariable("TASK3_EXPECTED_HOSTS").split(',', Qt::SkipEmptyParts));
    for (const auto& topic :
         qEnvironmentVariable("TASK3_EXPECTED_TOPICS").split(',', Qt::SkipEmptyParts))
        flowRules_[topic] = {
            {"mode", "periodic"},
            {"deadline_ms",
             qMax(1000, qEnvironmentVariable("TASK3_DDS_TIMEOUT_MS", "15000").toInt())}};
    QFile rules(qEnvironmentVariable("TASK3_FLOW_RULES"));
    if (!rules.fileName().isEmpty() && rules.open(QIODevice::ReadOnly) && rules.size() <= 65536) {
        for (const auto& value : QJsonDocument::fromJson(rules.readAll()).array()) {
            const auto rule = value.toObject();
            const auto topic = rule["topic"].toString();
            if (flowRules_.size() >= 128 || topic.isEmpty() || topic.size() > 200 ||
                !QStringList{"periodic", "event"}.contains(rule["mode"].toString()))
                continue;
            auto row = rule.toVariantMap();
            row["deadline_ms"] = qBound(100, rule["deadline_ms"].toInt(15000), 3600000);
            flowRules_[topic] = row;
        }
    }
    connect(&timer_, &QTimer::timeout, this, &MonitoringCore::tick);
    timer_.start();
}
bool MonitoringCore::startTelemetry(int domain, const QString& topic) {
    transport_.reset(new DdsTransport(false, domain, topic));
    connect(transport_.get(), &DdsTransport::received, this, &MonitoringCore::ingest);
    const bool ok = transport_->valid();
    status_ =
        ok ? QString("DDS telemetry domain %1 · %2").arg(domain).arg(topic) : transport_->error();
    emit changed();
    return ok;
}
void MonitoringCore::stop() {
    timer_.stop();
    transport_.reset();
    alerts_.flush();
}
bool MonitoringCore::ingest(const QByteArray& packet, const QString& source) {
    QJsonObject snapshot;
    QString error;
    if (!decode(packet, snapshot, error)) {
        emit rejected(error);
        return false;
    }
    const auto prior =
        registry_.hosts().value(snapshot["host_id"].toString()).snapshot["applications"].toArray();
    if (!registry_.ingest(snapshot,
                          source,
                          clock_.elapsed(),
                          QDateTime::currentMSecsSinceEpoch(),
                          error)) {
        emit rejected(error);
        return false;
    }
    QSet<QString> currentApplications;
    for (const auto& entry : snapshot["applications"].toArray())
        currentApplications.insert(entry.toObject()["application_id"].toString());
    for (const auto& entry : prior) {
        const auto appId = entry.toObject()["application_id"].toString();
        if (!currentApplications.contains(appId)) {
            const auto rule = snapshot["host_id"].toString() + "/" + appId;
            alerts_.unknown(rule + "/process");
            alerts_.unknown(rule + "/service");
        }
    }
    const QJsonValue cpu = snapshot["metrics"].toObject().value("cpu_percent");
    if (cpu.isDouble()) {
        auto& history = cpuHistory_[snapshot["host_id"].toString()];
        history.append(QVariantMap{{"timestamp_ms", QDateTime::currentMSecsSinceEpoch()},
                                   {"value", cpu.toDouble()}});
        while (history.size() > 120)
            history.removeFirst();
    }
    tick();
    return true;
}
void MonitoringCore::tick() {
    registry_.expire(clock_.elapsed(), timeout_);
    const auto now = QDateTime::currentMSecsSinceEpoch();
    for (auto it = registry_.hosts().cbegin(); it != registry_.hosts().cend(); ++it) {
        const auto source = it.key();
        if (!it->sequence)
            continue; // Expected but never observed is Unknown, not a detected failure.
        alerts_.condition(source + "/agent",
                          !it->online,
                          "agent_timeout",
                          source,
                          "Agent telemetry timed out; physical host state unknown",
                          now,
                          clock_.elapsed());
        for (const auto& entry : it->snapshot["applications"].toArray()) {
            const auto a = entry.toObject();
            const auto appSource = source + "/" + a["application_id"].toString();
            if (!it->online) {
                alerts_.unknown(appSource + "/process");
                alerts_.unknown(appSource + "/service");
                continue;
            }
            if (a["state"] != "unknown")
                alerts_.condition(appSource + "/process",
                                  a["state"] == "stopped" || a["state"] == "failed",
                                  "process_stopped",
                                  appSource,
                                  "Configured process stopped",
                                  now,
                                  clock_.elapsed());
            else
                alerts_.unknown(appSource + "/process");
            if (a["service_state"] != "unknown" && a["service_state"] != "not_configured")
                alerts_.condition(appSource + "/service",
                                  a["service_state"] == "stopped" || a["service_state"] == "failed",
                                  "service_stopped",
                                  appSource,
                                  "Configured service stopped",
                                  now,
                                  clock_.elapsed());
            else
                alerts_.unknown(appSource + "/service");
        }
        if (!it->online) {
            alerts_.unknown(source + "/time");
            continue;
        }
        const auto ts = it->snapshot["time_sync"].toObject();
        if (ts["state"] != "unknown")
            alerts_.condition(source + "/time",
                              ts["state"] != "synced",
                              "time_sync",
                              source,
                              "Time synchronization unhealthy: " + ts["detail"].toString(),
                              now,
                              clock_.elapsed());
        else
            alerts_.unknown(source + "/time");
    }
    remap();
    evaluateDds();
    flows_ = topology(entities_, QDateTime::currentMSecsSinceEpoch());
    emit changed();
}
void MonitoringCore::updateDds(QVariantList rows) {
    entities_ = std::move(rows);
    remap();
    evaluateDds();
    flows_ = topology(entities_, QDateTime::currentMSecsSinceEpoch());
    emit changed();
}
void MonitoringCore::remap() {
    const auto now = QDateTime::currentMSecsSinceEpoch();
    QMap<QString, QVariantMap> participantMappings;
    for (const auto& value : entities_) {
        const auto row = value.toMap();
        if (row["kind"] == "Participant")
            participantMappings[row["id"].toString()] =
                row["alive"].toBool() ? registry_.mapParticipant(row["hostname"].toString(),
                                                                 row["pid"].toString(),
                                                                 row["guid"].toString())
                                      : QVariantMap{{"mapping_state", "unknown"}};
    }
    for (auto& value : entities_) {
        auto row = value.toMap();
        for (const auto* field :
             {"host_id", "application_id", "instance_id", "binding_source", "writer_counts"})
            row.remove(field);
        row["mapping_state"] = "unknown";
        if (row["kind"] == "Participant") {
            const auto mapping = participantMappings.value(row["id"].toString());
            for (auto it = mapping.cbegin(); it != mapping.cend(); ++it)
                row[it.key()] = it.value();
            alerts_.condition("dds/" + row["id"].toString() + "/inactive",
                              !row["alive"].toBool(),
                              "participant_inactive",
                              row["id"].toString(),
                              "DDS participant inactive",
                              now,
                              clock_.elapsed());
        }
        // No-data rules are opt-in; absence of a statistic is not proof that application data stopped.
        if (row["kind"] == "Writer" || row["kind"] == "Reader") {
            const auto parents = row["participant_ids"].toList();
            if (parents.size() == 1) {
                const auto mapping = participantMappings.value(parents[0].toString());
                for (auto it = mapping.cbegin(); it != mapping.cend(); ++it)
                    row[it.key()] = it.value();
            }
            row["data_state"] =
                row["metric_fresh"].toBool() ? "statistics available" : "statistics missing/stale";
            row["match_state"] = matchingState(row, now);
            const auto thresholdText = qEnvironmentVariable("TASK3_THROUGHPUT_MIN");
            if (!thresholdText.isEmpty() && row["metric_fresh"].toBool()) {
                bool ok = false;
                const double threshold = thresholdText.toDouble(&ok);
                if (ok && threshold >= 0) {
                    const auto id = "dds/" + row["id"].toString() + "/throughput";
                    const auto hysteresis =
                        qMax(0.0,
                             qEnvironmentVariable("TASK3_THROUGHPUT_HYSTERESIS", "0").toDouble());
                    alerts_.condition(id,
                                      row["throughput_bps"].toDouble() <
                                          threshold + (alerts_.isActive(id) ? hysteresis : 0),
                                      "throughput_below_threshold",
                                      row["id"].toString(),
                                      "Measured throughput below configured threshold",
                                      now,
                                      clock_.elapsed());
                }
            }
        }
        value = row;
    }
}
void MonitoringCore::evaluateDds() {
    auto rules = flowRules_;
    const auto timeout = qMax(1000, qEnvironmentVariable("TASK3_DDS_TIMEOUT_MS", "15000").toInt());
    for (const auto& topic :
         qEnvironmentVariable("TASK3_EXPECTED_TOPICS").split(',', Qt::SkipEmptyParts))
        if (!rules.contains(topic))
            rules[topic] = {{"mode", "periodic"}, {"deadline_ms", timeout}};
    QMap<QString, bool> writersPresent, liveWriters, freshWriters;
    QMap<QString, bool> allRuntimeSilent, runtimeWriting;
    QSet<QString> currentIds;
    for (auto& entry : entities_) {
        auto row = entry.toMap();
        if (row["kind"] != "Writer" && row["kind"] != "Reader")
            continue;
        const auto id = row["id"].toString();
        currentIds.insert(id);
        const auto timestamp = row["metric_timestamp_ms"].toLongLong();
        const auto age = QDateTime::currentMSecsSinceEpoch() - timestamp;
        row["metric_fresh"] = timestamp > 0 && age >= -2000 && age < 15000;
        row["data_state"] = row["metric_fresh"].toBool() && row["throughput_bps"].toDouble() > 0
                                ? "Active (measured statistics)"
                                : "Unknown (statistics stale/unavailable)";
        const auto topic = row["topic"].toString();
        const auto rule = rules.value(topic);
        const auto count = row["writer_counts"].toMap().value(row["guid"].toString()).toString();
        bool countOk = false;
        count.toULongLong(&countOk);
        if (row["kind"] == "Writer" && countOk && row["alive"].toBool()) {
            const auto instance = row["instance_id"].toString();
            const bool existing = counters_.contains(id) && counters_[id].instance == instance;
            auto& previous = counters_[id];
            const bool advanced = existing && count.toULongLong() > previous.count.toULongLong();
            if (!existing || count != previous.count)
                previous = {instance, count, clock_.elapsed()};
            const auto deadline = rule.value("deadline_ms", 15000).toInt();
            const bool silent = clock_.elapsed() - previous.changed >= deadline;
            row["data_state"] =
                rule["mode"] == "event" && silent
                    ? "Event idle (allowed)"
                    : (silent && rule["mode"] == "periodic"
                           ? "No Data (runtime write counter)"
                           : (advanced || previous.count != "0" ? "Active (runtime write counter)"
                                                                : "Unknown (waiting for writes)"));
            if (rule["mode"] == "periodic") {
                const auto source = row["host_id"].toString() + "/" +
                                    row["application_id"].toString() + "/" + topic;
                if (!allRuntimeSilent.contains(source))
                    allRuntimeSilent[source] = true;
                allRuntimeSilent[source] = allRuntimeSilent[source] && silent;
                runtimeWriting[source] =
                    runtimeWriting[source] || (!silent && count.toULongLong() > 0);
            }
        }
        entry = row;
        if (!metricSeen_.contains(id) || timestamp > metricTimestamp_.value(id)) {
            metricSeen_[id] = clock_.elapsed();
            metricTimestamp_[id] = timestamp;
        }
        if (rule["mode"] != "periodic")
            continue;
        const bool silent = clock_.elapsed() - metricSeen_[id] > timeout;
        alerts_.condition("dds/" + id + "/stale",
                          silent,
                          "statistics_stale",
                          id,
                          "Statistics stream stale for an explicitly monitored topic",
                          QDateTime::currentMSecsSinceEpoch(),
                          clock_.elapsed());
        if (row["kind"] == "Writer") {
            const auto topic = row["topic"].toString();
            writersPresent[topic] = true;
            liveWriters[topic] |= row["alive"].toBool();
            freshWriters[topic] |= row["alive"].toBool() && !silent && timestamp > 0;
        }
    }
    for (auto it = metricSeen_.begin(); it != metricSeen_.end();) {
        if (!currentIds.contains(it.key())) {
            metricTimestamp_.remove(it.key());
            counters_.remove(it.key());
            it = metricSeen_.erase(it);
        } else
            ++it;
    }
    for (auto it = allRuntimeSilent.cbegin(); it != allRuntimeSilent.cend(); ++it) {
        if (it.value() || runtimeWriting[it.key()])
            alerts_.condition(
                "flow/" + it.key() + "/no_data",
                it.value(),
                "dds_no_data",
                it.key(),
                "Expected periodic application/topic has no new writes from any instrumented writer within deadline",
                QDateTime::currentMSecsSinceEpoch(),
                clock_.elapsed());
    }
    for (auto it = rules.cbegin(); it != rules.cend(); ++it) {
        const auto topic = it.key();
        if (it.value()["mode"] != "periodic")
            continue;
        const auto deadline = it.value().value("deadline_ms", timeout).toInt();
        const auto rule = "topic/" + topic + "/no_data";
        if (!writersPresent[topic])
            continue; // No discovery evidence yet.
        const bool failure = !liveWriters[topic];
        if (failure && !ruleSince_.contains(rule))
            ruleSince_[rule] = clock_.elapsed();
        if (!failure)
            ruleSince_.remove(rule);
        if (failure && clock_.elapsed() - ruleSince_[rule] >= deadline)
            alerts_.condition(
                rule,
                true,
                "dds_no_data",
                topic,
                "Expected periodic topic: all discovered writers inactive throughout timeout",
                QDateTime::currentMSecsSinceEpoch(),
                clock_.elapsed());
        else if (freshWriters[topic])
            alerts_.condition(rule,
                              false,
                              "dds_no_data",
                              topic,
                              "",
                              QDateTime::currentMSecsSinceEpoch(),
                              clock_.elapsed());
    }
}
}
