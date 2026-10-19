// SPDX-License-Identifier: GPL-3.0-or-later
#include "Registry.h"
#include "Protocol.h"
#include <QJsonArray>
#include <QDateTime>
namespace task3 {
void Registry::expect(const QStringList& hostIds) {
    for (const auto& id : hostIds) {
        if (id.isEmpty() || id.size() > 128 || hosts_.contains(id) || hosts_.size() >= 512)
            continue;
        Host host;
        host.online = false;
        host.snapshot = QJsonObject{{"host_id", id},
                                    {"hostname", id},
                                    {"os", "unknown"},
                                    {"metrics", QJsonObject{}},
                                    {"applications", QJsonArray{}},
                                    {"time_sync", QJsonObject{}}};
        hosts_.insert(id, host);
    }
}
bool Registry::ingest(const QJsonObject& s,
                      const QString& address,
                      qint64 mono,
                      qint64 wall,
                      QString& error) {
    if (!validate(s, error))
        return false;
    const auto id = s["host_id"].toString(), session = s["session_id"].toString();
    const quint64 seq = s["sequence"].toString().toULongLong();
    if (!hosts_.contains(id) && hosts_.size() >= 512) {
        error = "host capacity reached";
        return false;
    }
    auto it = hosts_.find(id);
    if (it != hosts_.end()) {
        if (session == it->sessionId && seq <= it->sequence) {
            error = "duplicate/out-of-order";
            return false;
        }
        if (session != it->sessionId && it->retiredSessions.contains(session)) {
            error = "retired session";
            return false;
        }
    }
    auto& h = hosts_[id];
    if (!h.sessionId.isEmpty() && session != h.sessionId) {
        h.retiredSessions.append(h.sessionId);
        if (h.retiredSessions.size() > 256)
            h.retiredSessions.removeFirst();
    }
    h.snapshot = s;
    h.address = address;
    h.seenMono = mono;
    h.seenWall = wall;
    h.sessionId = session;
    h.sequence = seq;
    h.online = true;
    return true;
}
void Registry::expire(qint64 now, qint64 timeout) {
    for (auto& h : hosts_)
        h.online = h.sequence > 0 && now - h.seenMono <= timeout;
}
QVariantList Registry::rows() const {
    QVariantList rows;
    for (auto it = hosts_.cbegin(); it != hosts_.cend(); ++it) {
        auto row = it->snapshot.toVariantMap();
        row["online"] = it->online;
        row["metrics_state"] = !it->sequence ? "unknown" : (it->online ? "fresh" : "stale");
        row["address"] = it->address;
        row["last_seen"] =
            !it->sequence
                ? "never"
                : QDateTime::fromMSecsSinceEpoch(it->seenWall).toString(Qt::ISODateWithMs);
        row["host_state"] =
            !it->sequence
                ? "Unknown (no telemetry yet)"
                : (it->online ? "Online" : "Offline (agent unreachable; host cause unknown)");
        rows.append(row);
    }
    return rows;
}
QVariantMap
Registry::mapParticipant(const QString& hostname, const QString& pid, const QString& guid) const {
    QVariantMap result{{"mapping_state", "unknown"}, {"host_id", ""}, {"application_id", ""}};
    int candidates = 0;
    for (auto it = hosts_.cbegin(); it != hosts_.cend(); ++it) {
        if (!it->online || (it->snapshot["hostname"].toString() != hostname &&
                            it->snapshot["dds_host_identifier"].toString() != hostname))
            continue;
        for (const auto& entry : it->snapshot["applications"].toArray()) {
            const auto app = entry.toObject();
            auto instances = app["instances"].toArray();
            if (instances.isEmpty() && app["state"] == "running")
                instances.append(app);
            for (const auto& value : instances) {
                auto process = value.toObject();
                // Legacy manually configured GUIDs apply only to a single live process.
                if (instances.size() == 1 && !process.contains("binding_source"))
                    process["participant_guids"] = app["participant_guids"];
                if (process["binding_conflict"].toBool())
                    continue;
                if (QString::number(qint64(process["pid"].toDouble())) != pid || guid.isEmpty() ||
                    process["instance_id"].toString().isEmpty() ||
                    !process["participant_guids"].toArray().contains(guid))
                    continue;
                ++candidates;
                result = {{"mapping_state", "mapped: explicit GUID binding"},
                          {"host_id", it.key()},
                          {"application_id", app["application_id"].toString()},
                          {"instance_id", process["instance_id"].toString()},
                          {"binding_source", process["binding_source"].toString("configured_guid")},
                          {"writer_counts", process["writer_counts"].toObject().toVariantMap()}};
            }
        }
    }
    if (candidates != 1)
        return {{"mapping_state", "unknown"}, {"host_id", ""}, {"application_id", ""}};
    // Explicit per-run binding is required; hostname/PID alone never establishes an association.
    return result;
}
}
