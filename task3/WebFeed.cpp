// SPDX-License-Identifier: GPL-3.0-or-later
#include "WebFeed.h"
namespace task3 {
WebFeed::WebFeed(QObject* source, QObject* parent, int coalesceMs)
    : QObject(parent), source_(source) {
    timer_.setSingleShot(true);
    timer_.setInterval(qMax(0, coalesceMs));
    connect(&timer_, &QTimer::timeout, this, &WebFeed::refresh);
    if (source)
        connect(source, SIGNAL(changed()), this, SLOT(schedule()));
    refresh();
}
void WebFeed::schedule() {
    // Not restarted while running: an update is delayed by at most one interval, even under bursts.
    if (!timer_.isActive())
        timer_.start();
}
void WebFeed::refresh() {
    if (!source_)
        return;
    const auto hosts = source_->property("hosts").toList();
    if (hosts != hosts_) {
        hosts_ = hosts;
        emit hostsChanged();
    }
    const auto events = source_->property("events").toList();
    const bool more = source_->property("hasMoreEvents").toBool();
    if (events != events_ || more != hasMoreEvents_) {
        events_ = events;
        hasMoreEvents_ = more;
        emit eventsChanged();
    }
    const auto flows = source_->property("flows").toList();
    if (flows != flows_) {
        flows_ = flows;
        emit flowsChanged();
    }
    const auto participants = participantsOf(source_->property("entities").toList());
    if (participants != participants_) {
        participants_ = participants;
        emit participantsChanged();
    }
}
void WebFeed::loadOlderEvents() {
    if (!source_)
        return;
    QMetaObject::invokeMethod(source_, "loadOlderEvents");
    refresh();
}
QVariantList WebFeed::participantsOf(const QVariantList& entities) {
    QVariantList rows;
    for (const auto& value : entities) {
        const auto entity = value.toMap();
        if (entity["kind"] != "Participant")
            continue;
        QVariantMap row;
        for (const auto* field : {"id", "guid", "pid", "host_id", "application_id", "mapping_state"})
            if (entity.contains(field))
                row[field] = entity[field];
        rows.append(row);
    }
    return rows;
}
}
