// SPDX-License-Identifier: GPL-3.0-or-later
#include "FlowModel.h"
#include <QMap>
namespace task3 {
QString matchingState(const QVariantMap& e, qint64 now) {
    const auto timestamp = e["connection_status_timestamp_ms"].toLongLong();
    if (!e["alive"].toBool() || timestamp <= 0 || now - timestamp < -2000 || now - timestamp >= 15000)
        return "Unknown";
    return e["connection_guids"].toStringList().isEmpty() ? "Unmatched" : "Matched";
}
QVariantList topology(const QVariantList& entities, qint64 now) {
    QMap<QString, QVariantMap> index;
    for (const auto& item : entities) index[item.toMap()["id"].toString()] = item.toMap();
    QVariantList rows;
    for (const auto& value : entities) {
        const auto w = value.toMap(); if (w["kind"] != "Writer") continue;
        auto links = w["links"].toList();
        if (links.isEmpty()) links.append(QVariantMap{});
        for (const auto& item : links) {
            if (rows.size() >= 8192) break;
            const auto link = item.toMap(), r = index.value(link["reader_id"].toString());
            const bool writerFresh = matchingState(w, now) != "Unknown";
            const bool readerFresh = matchingState(r, now) != "Unknown";
            const bool wc = w["connection_guids"].toStringList().contains(r["guid"].toString());
            const bool rc = r["connection_guids"].toStringList().contains(w["guid"].toString());
            QString match = "Unknown";
            if (w["alive"].toBool() && r["alive"].toBool() && !r.isEmpty()) {
                if ((writerFresh && wc) || (readerFresh && rc)) match = "Matched";
                if ((writerFresh && !wc) || (readerFresh && !rc)) match =
                    ((writerFresh && wc) || (readerFresh && rc)) ? "Conflicting evidence" : "Unmatched";
            }
            rows.append(QVariantMap{{"writer_id", w["id"]}, {"reader_id", r["id"]}, {"topic", w["topic"]},
                {"source_host", w["host_id"]}, {"source_application", w["application_id"]},
                {"target_host", r["host_id"]}, {"target_application", r["application_id"]},
                {"match_state", match}, {"confirmed", match == "Matched"},
                {"source_alive", w["alive"]}, {"target_alive", r["alive"]},
                {"flow_state", w["data_state"]}, {"throughput_bps", w["throughput_bps"]},
                {"throughput_timestamp_ms", w["metric_timestamp_ms"]},
                {"latency_us", link["latency_us"]}, {"latency_timestamp_ms", link["latency_timestamp_ms"]}});
        }
    }
    return rows;
}
}
