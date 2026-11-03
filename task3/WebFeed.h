// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
namespace task3 {
// Data published to the WebEngine dashboard (web/app.js) through QWebChannel.
// QWebChannel resends every property that shares a NOTIFY signal, and MonitoringCore entities carry
// QoS/details/samples the page never shows, so this keeps one signal per property, emits only on a
// real change, coalesces bursts of MonitoringCore::changed and trims entities to mapped Participants.
// The source only needs the MonitoringCore property names and a changed() signal.
class WebFeed : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList hosts READ hosts NOTIFY hostsChanged)
    Q_PROPERTY(QVariantList events READ events NOTIFY eventsChanged)
    Q_PROPERTY(bool hasMoreEvents READ hasMoreEvents NOTIFY eventsChanged)
    Q_PROPERTY(QVariantList flows READ flows NOTIFY flowsChanged)
    Q_PROPERTY(QVariantList participants READ participants NOTIFY participantsChanged)
  public:
    explicit WebFeed(QObject* source, QObject* parent = nullptr, int coalesceMs = 100);
    QVariantList hosts() const {
        return hosts_;
    }
    QVariantList events() const {
        return events_;
    }
    bool hasMoreEvents() const {
        return hasMoreEvents_;
    }
    QVariantList flows() const {
        return flows_;
    }
    QVariantList participants() const {
        return participants_;
    }
    Q_INVOKABLE void loadOlderEvents();
    // Fields of each Participant that the page renders; everything else stays in C++.
    static QVariantList participantsOf(const QVariantList& entities);
  public slots:
    void refresh();
  signals:
    void hostsChanged();
    void eventsChanged();
    void flowsChanged();
    void participantsChanged();

  private slots:
    void schedule();

  private:
    QPointer<QObject> source_;
    QTimer timer_;
    QVariantList hosts_, events_, flows_, participants_;
    bool hasMoreEvents_ = false;
};
}
