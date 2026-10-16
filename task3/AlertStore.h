// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QString>
#include <QVariantList>
#include <QMap>
#include <QJsonObject>
#include <memory>
namespace task3 {
class AlertStore {
public:
    explicit AlertStore(const QString& path);
    ~AlertStore();
    void condition(const QString& id, bool failed, const QString& kind, const QString& source,
                   const QString& message, qint64 now, qint64 monotonicMs = -1);
    QVariantList rows() const;
    QString error() const;
    bool isActive(const QString& id) const { return active_.contains(id); }
    int activeCount(const QString& source) const;
    void unknown(const QString& id) { candidates_.remove(id); }
    void policy(qint64 triggerMs, qint64 recoveryMs, qint64 cooldownMs);
    QJsonObject replayState() const;
    void saveReplay(const QJsonObject&);
    void flush();
    void applyRetention();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    QMap<QString, QVariantMap> active_;
    QVariantList rows_;
    struct Candidate { bool failed = false; qint64 since = 0; };
    QMap<QString, Candidate> candidates_;
    QMap<QString, qint64> recovered_;
    qint64 triggerMs_ = 0, recoveryMs_ = 0, cooldownMs_ = 0;
    bool append(const QVariantMap&);
};
}
