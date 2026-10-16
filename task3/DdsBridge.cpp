// SPDX-License-Identifier: GPL-3.0-or-later
#include "DdsBridge.h"
#include "Units.h"
#include "RollingSeries.h"
#include <QtConcurrent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <fastdds_statistics_backend/StatisticsBackend.hpp>
namespace task3 {
namespace sb = eprosima::statistics_backend;
DdsBridge::DdsBridge(QObject* parent) : QObject(parent) {
    connect(&watcher_, &QFutureWatcher<QVariantList>::finished, this, [this] { emit updated(watcher_.result()); });
    connect(&timer_, &QTimer::timeout, this, &DdsBridge::poll);
    const int interval = qEnvironmentVariable("TASK3_QUERY_INTERVAL_MS", "2000").toInt();
    timer_.start(qBound(200, interval, 60000));
}
DdsBridge::~DdsBridge() { stop(); }
void DdsBridge::stop() { timer_.stop(); watcher_.waitForFinished(); }
void DdsBridge::poll() {
    if (!watcher_.isRunning()) watcher_.setFuture(QtConcurrent::run(&DdsBridge::snapshot));
}
static QString text(const sb::Info& info, const char* key) {
    if (!info.contains(key)) return {};
    const auto& v = info[key];
    return v.is_string() ? QString::fromStdString(v.get<std::string>()) : QString::fromStdString(v.dump());
}
static QString related(sb::EntityKind kind, sb::EntityId id, const char* key) {
    const auto ids = sb::StatisticsBackend::get_entities(kind, id);
    return ids.size() == 1 ? text(sb::StatisticsBackend::get_info(ids.front()), key) : QString();
}
static QVariantList samples(const std::vector<sb::StatisticsData>& data, double divisor) {
    QVariantList result;
    std::vector<RollingSeries::Sample> raw;
    // Bounded rolling raw window is re-read each poll. Same snapshot replaces prior one, no append duplicates.
    for (const auto& point : data) if (std::isfinite(point.second)) {
        raw.emplace_back(std::chrono::duration_cast<std::chrono::milliseconds>(
                    point.first.time_since_epoch()).count(), point.second / divisor);
    }
    RollingSeries series(qBound(1000, qEnvironmentVariable("TASK3_QUERY_WINDOW_MS", "120000").toInt(), 3600000),
                         size_t(qBound(16, qEnvironmentVariable("TASK3_MAX_SAMPLES", "256").toInt(), 10000)));
    series.refresh(raw, QDateTime::currentMSecsSinceEpoch());
    for (const auto& p : series.points()) result.append(QVariantMap{{"timestamp_ms", qint64(p.first)}, {"value", p.second}});
    return result;
}
QVariantList DdsBridge::snapshot() {
    QVariantList rows;
    const auto now = sb::now();
    const auto from = now - std::chrono::milliseconds(qBound(1000,
        qEnvironmentVariable("TASK3_QUERY_WINDOW_MS", "120000").toInt(), 3600000));
    const std::pair<sb::EntityKind, const char*> kinds[] = {
        {sb::EntityKind::DOMAIN, "Domain"}, {sb::EntityKind::PARTICIPANT, "Participant"},
        {sb::EntityKind::TOPIC, "Topic"}, {sb::EntityKind::DATAWRITER, "Writer"}, {sb::EntityKind::DATAREADER, "Reader"}};
    try {
        for (const auto& kind : kinds) {
            const auto ids = sb::StatisticsBackend::get_entities(kind.first);
            for (const auto id : ids) {
                if (rows.size() >= 4096) break;
                try {
                    const auto info = sb::StatisticsBackend::get_info(id);
                    if ((kind.first == sb::EntityKind::DATAWRITER || kind.first == sb::EntityKind::DATAREADER ||
                         kind.first == sb::EntityKind::TOPIC) && info.value("metatraffic", false)) continue;
                    QVariantMap row{{"id", QString::number(id.value())}, {"kind", kind.second},
                        {"name", text(info, "name")}, {"guid", text(info, "guid")},
                        {"alive", sb::StatisticsBackend::is_active(id)},
                        {"details", QString::fromStdString(info.dump(2))},
                        {"hostname", related(sb::EntityKind::HOST, id, "name")},
                        {"pid", related(sb::EntityKind::PROCESS, id, "pid")},
                        {"mapping_state", "unknown"}, {"match_state", "not inferred from discovery"}};
                    if (kind.first == sb::EntityKind::DATAWRITER || kind.first == sb::EntityKind::DATAREADER) {
                        try {
                            sb::ConnectionListSample status;
                            sb::StatisticsBackend::get_status_data(id, status);
                            const auto statusMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                status.src_ts.time_since_epoch()).count();
                            const auto age = QDateTime::currentMSecsSinceEpoch() - statusMs;
                            row["connection_status_timestamp_ms"] = qint64(statusMs);
                            row["connection_count"] = int(status.connection_list.size());
                            QStringList connections;
                            for (const auto& connection : status.connection_list) {
                                if (connections.size() >= 256) break;
                                connections.append(QString::fromStdString(sb::StatisticsBackend::deserialize_guid(connection.guid())));
                            }
                            row["connection_guids"] = connections;
                            if (statusMs > 0 && age >= -2000 && age < 15000)
                                row["match_state"] = status.connection_list.empty() ? "unmatched (reported)" : "matched (reported)";
                            else row["matching_detail"] = "connection status unavailable/stale";
                        } catch (const std::exception& e) {
                            row["matching_detail"] = QString::fromUtf8(e.what());
                        }
                        row["participant_ids"] = QVariantList();
                        QVariantList participants;
                        for (auto p : sb::StatisticsBackend::get_entities(sb::EntityKind::PARTICIPANT, id))
                            participants.append(QString::number(p.value()));
                        row["participant_ids"] = participants;
                        row["topic"] = related(sb::EntityKind::TOPIC, id, "name");
                        const auto dataKind = kind.first == sb::EntityKind::DATAWRITER ?
                            sb::DataKind::PUBLICATION_THROUGHPUT : sb::DataKind::SUBSCRIPTION_THROUGHPUT;
                        const auto points = samples(sb::StatisticsBackend::get_data(dataKind, {id}, 0, from, now), 1);
                        row["samples"] = points; row["metric_fresh"] = false;
                        if (!points.isEmpty()) {
                            const auto last = points.last().toMap();
                            row["throughput_bps"] = last["value"]; row["metric_timestamp_ms"] = last["timestamp_ms"];
                            row["throughput_measurement"] = throughputMeasurement(last["value"].toDouble());
                            const auto age = QDateTime::currentMSecsSinceEpoch() - last["timestamp_ms"].toLongLong();
                            row["metric_fresh"] = age >= -2000 && age < 15000;
                        }
                        if (kind.first == sb::EntityKind::DATAWRITER) {
                            QVariantList links;
                            auto readers = sb::StatisticsBackend::get_entities(sb::EntityKind::DATAREADER, id);
                            // Include reported peers even if the graph's relationship set lags discovery.
                            const auto peerGuids = row["connection_guids"].toStringList();
                            if (!peerGuids.isEmpty()) for (auto reader : sb::StatisticsBackend::get_entities(sb::EntityKind::DATAREADER)) {
                                if (peerGuids.contains(text(sb::StatisticsBackend::get_info(reader), "guid")) &&
                                    std::find(readers.begin(), readers.end(), reader) == readers.end()) readers.push_back(reader);
                            }
                            for (auto reader : readers) {
                                if (links.size() >= 64) break;
                                const auto latency = samples(sb::StatisticsBackend::get_data(sb::DataKind::FASTDDS_LATENCY,
                                    {id}, {reader}, 0, from, now), 1000);
                                links.append(QVariantMap{{"reader_id", QString::number(reader.value())},
                                    {"relation", latency.isEmpty() ? "related endpoint; matching unknown" : "latency measurements available"},
                                    {"latency_us", latency.isEmpty() ? QVariant() : latency.last().toMap()["value"]},
                                    {"latency_timestamp_ms", latency.isEmpty() ? QVariant() : latency.last().toMap()["timestamp_ms"]}});
                            }
                            row["links"] = links;
                        }
                    }
                    if (kind.first == sb::EntityKind::PARTICIPANT) {
                        QVariantList network;
                        for (auto locator : sb::StatisticsBackend::get_entities(sb::EntityKind::LOCATOR, id)) {
                            if (network.size() >= 16) break;
                            const auto data = samples(sb::StatisticsBackend::get_data(
                                sb::DataKind::NETWORK_LATENCY, {id}, {locator}, 0, from, now), 1000);
                            if (!data.isEmpty()) network.append(QVariantMap{{"locator_id", QString::number(locator.value())},
                                {"latency_us", data.last().toMap()["value"]},
                                {"timestamp_ms", data.last().toMap()["timestamp_ms"]}});
                        }
                        row["network_latency"] = network;
                    }
                    row["qos"] = info.contains("qos") ? QString::fromStdString(info["qos"].dump(2)) : QString();
                    rows.append(row);
                } catch (const std::exception& e) {
                    qWarning() << "DDS entity query:" << id.value() << e.what();
                }
            }
        }
    } catch (const std::exception& e) {
        qWarning() << "DDS snapshot:" << e.what();
        rows.append(QVariantMap{{"kind", "Error"}, {"name", e.what()}, {"id", ""}});
    }
    return rows;
}
}
