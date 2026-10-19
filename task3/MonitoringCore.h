// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "Registry.h"
#include "AlertStore.h"
#include <QObject>
#include "DdsTransport.h"
#include <memory>
#include <QElapsedTimer>
#include <QTimer>
namespace task3 {
class MonitoringCore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList hosts READ hosts NOTIFY changed)
    Q_PROPERTY(QVariantList events READ events NOTIFY changed)
    Q_PROPERTY(bool hasMoreEvents READ hasMoreEvents NOTIFY changed)
    Q_PROPERTY(QVariantList entities READ entities NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QVariantList flows READ flows NOTIFY changed)
  public:
    MonitoringCore(const QString& dbPath, int timeoutMs = 5000, QObject* parent = nullptr);
    bool startTelemetry(int domain, const QString& topic);
    bool ingest(const QByteArray&, const QString& source);
    void tick();
    void stop();
    void updateDds(QVariantList rows);
    QVariantList hosts() const;
    QVariantList events() const;
    bool hasMoreEvents() const {
        return alerts_.hasMoreRows();
    }
    Q_INVOKABLE void loadOlderEvents();
    QVariantList entities() const {
        return entities_;
    }
    QVariantList flows() const {
        return flows_;
    }
    QString status() const {
        const auto error = alerts_.error();
        return status_ + (error.isEmpty() ? QString() : " | Event database: " + error) +
               (transport_ && transport_->droppedPackets()
                    ? " | dropped telemetry: " + QString::number(transport_->droppedPackets())
                    : QString());
    }
    const Registry& registry() const {
        return registry_;
    }
  signals:
    void changed();
    void rejected(QString reason);

  private:
    Registry registry_;
    AlertStore alerts_;
    std::unique_ptr<DdsTransport> transport_;
    QElapsedTimer clock_;
    QTimer timer_;
    int timeout_;
    QString status_;
    QVariantList entities_;
    QVariantList flows_;
    QMap<QString, QVariantMap> flowRules_;
    struct Counter {
        QString instance, count;
        qint64 changed = 0;
    };
    QMap<QString, Counter> counters_;
    QMap<QString, QVariantList> cpuHistory_;
    QMap<QString, qint64> ruleSince_, metricSeen_, metricTimestamp_;
    void evaluateDds();
    void remap();
};
}
