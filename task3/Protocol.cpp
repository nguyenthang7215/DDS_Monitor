// SPDX-License-Identifier: GPL-3.0-or-later
#include "Protocol.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QSet>
#include <QRegularExpression>
#include <cmath>
namespace task3 {
static bool identifier(const QString& s) {
    static const QRegularExpression re("^[A-Za-z0-9_.:-]{1,128}$");
    return re.match(s).hasMatch();
}
static bool number(const QJsonObject& o, const char* field, double low, double high) {
    const auto v = o.value(field);
    return v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble() >= low &&
           v.toDouble() <= high;
}
bool validate(const QJsonObject& s, QString& error) {
    auto fail = [&](const QString& why) {
        error = why;
        return false;
    };
    if (s["schema_version"] != 1 || s["type"] != "telemetry")
        return fail("unsupported schema/type");
    if (!identifier(s["host_id"].toString()) || !identifier(s["session_id"].toString()))
        return fail("invalid host/session identity");
    bool ok = false;
    static const QRegularExpression sequencePattern("^[1-9][0-9]{0,19}$");
    if (!sequencePattern.match(s["sequence"].toString()).hasMatch())
        return fail("invalid decimal sequence");
    const auto seq = s["sequence"].toString().toULongLong(&ok);
    if (!ok || !seq)
        return fail("sequence must be a positive decimal string");
    if (!number(s, "timestamp_ms", 1, 9e15))
        return fail("invalid timestamp");
    if (s["hostname"].toString().isEmpty() || s["hostname"].toString().size() > 255 ||
        s["os"].toString().isEmpty() || s["os"].toString().size() > 255)
        return fail("invalid host metadata");
    if (!s["metrics"].isObject() || !s["applications"].isArray() || !s["time_sync"].isObject())
        return fail("missing payload");
    const auto m = s["metrics"].toObject();
    if (!m["cpu_percent"].isNull() && !number(m, "cpu_percent", 0, 100))
        return fail("invalid CPU");
    for (const auto* k : {"ram_total",
                          "ram_used",
                          "ram_available",
                          "disk_total",
                          "disk_used",
                          "disk_free",
                          "network_rx_bytes",
                          "network_tx_bytes",
                          "network_rx_bps",
                          "network_tx_bps"})
        if (!number(m, k, 0, 9e18))
            return fail(QString("invalid metric: ") + k);
    if (m["ram_used"].toDouble() > m["ram_total"].toDouble() ||
        m["ram_available"].toDouble() > m["ram_total"].toDouble() ||
        m["disk_used"].toDouble() > m["disk_total"].toDouble())
        return fail("inconsistent metrics");
    const auto apps = s["applications"].toArray();
    if (apps.size() > 128)
        return fail("too many applications");
    QSet<QString> ids;
    for (const auto& v : apps) {
        if (!v.isObject())
            return fail("invalid application");
        const auto a = v.toObject();
        const auto id = a["application_id"].toString();
        if (!identifier(id) || ids.contains(id))
            return fail("invalid/duplicate application_id");
        ids.insert(id);
        if (!QStringList{"running", "stopped", "failed", "unknown"}.contains(a["state"].toString()))
            return fail("invalid process state");
        if (!QStringList{"running", "stopped", "failed", "unknown", "not_configured"}.contains(
                a["service_state"].toString()))
            return fail("invalid service state");
        if (!number(a, "pid", 0, 4294967295.0) ||
            std::floor(a["pid"].toDouble()) != a["pid"].toDouble())
            return fail("invalid pid");
        if (a.contains("instances") && !a["instances"].isArray())
            return fail("invalid process instances");
        if (a["instance_id"].toString().size() > 256)
            return fail("invalid process identity");
        const auto instances = a["instances"].toArray();
        if (instances.size() > 128)
            return fail("too many process instances");
        QSet<qint64> pids;
        for (const auto& instance : instances) {
            const auto p = instance.toObject();
            const auto pid = qint64(p["pid"].toDouble());
            if (!number(p, "pid", 1, 4294967295.0) || double(pid) != p["pid"].toDouble() ||
                pids.contains(pid) || p["instance_id"].toString().isEmpty() ||
                p["instance_id"].toString().size() > 256)
                return fail("invalid/duplicate process instance");
            pids.insert(pid);
            if (p["participant_guids"].toArray().size() > 32 ||
                p["writer_counts"].toObject().size() > 64)
                return fail("too many runtime identities");
            for (const auto& guid : p["participant_guids"].toArray())
                if (guid.toString().isEmpty() || guid.toString().size() > 128)
                    return fail("invalid GUID");
            for (const auto& count : p["writer_counts"].toObject()) {
                bool countOk = false;
                count.toString().toULongLong(&countOk);
                if (!countOk)
                    return fail("writer counters must be unsigned decimal strings");
            }
        }
        if (a["state"] == "running" && instances.isEmpty() &&
            (a["pid"].toDouble() <= 0 || a["instance_id"].toString().isEmpty()))
            return fail("missing process instance");
    }
    if (!QStringList{"synced", "unhealthy", "unknown"}.contains(
            s["time_sync"].toObject()["state"].toString()))
        return fail("invalid time state");
    return true;
}
QByteArray encode(const QJsonObject& snapshot) {
    return QJsonDocument(snapshot).toJson(QJsonDocument::Compact);
}
bool decode(const QByteArray& packet, QJsonObject& out, QString& error) {
    if (packet.size() > MaxPacket || packet.isEmpty()) {
        error = "packet size";
        return false;
    }
    QJsonParseError parse;
    const auto document = QJsonDocument::fromJson(packet, &parse);
    if (parse.error != QJsonParseError::NoError || !document.isObject()) {
        error = "malformed telemetry JSON";
        return false;
    }
    out = document.object();
    return validate(out, error);
}
}
