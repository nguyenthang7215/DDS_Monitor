// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QJsonObject>
#include <QMap>
#include <QVariantList>
#include <QStringList>
namespace task3 {
struct Host {
    QJsonObject snapshot;
    QString address;
    qint64 seenMono = 0, seenWall = 0;
    quint64 sequence = 0;
    QString sessionId;
    QStringList retiredSessions; // Bounded in-memory protection, not persistent authentication.
    bool online = true;
};
class Registry {
public:
    void expect(const QStringList& hostIds);
    bool ingest(const QJsonObject&, const QString& address, qint64 monotonicMs, qint64 wallMs, QString& error);
    void expire(qint64 monotonicMs, qint64 timeoutMs);
    QVariantList rows() const;
    QVariantMap mapParticipant(const QString& hostname, const QString& pid, const QString& guid = {}) const;
    const QMap<QString, Host>& hosts() const { return hosts_; }
private:
    QMap<QString, Host> hosts_;
};
}
