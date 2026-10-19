// SPDX-License-Identifier: GPL-3.0-or-later
#include "AlertStore.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QElapsedTimer>
#include <QUuid>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QDebug>
#include <functional>
#include <QStringList>
#include <QSet>
namespace task3 {
struct AlertStore::Impl {
    QThread thread;
    QObject* worker = new QObject;
    QObject* frontend = new QObject;
    std::function<void(QStringList)> pruned;
    QSqlDatabase db; // Accessed exclusively on worker thread, including creation/destruction.
    QString connection = QUuid::createUuid().toString(), error;
    QMutex mutex;
    QList<QVariantMap> pending;
    int retentionDays = 0; // Explicit opt-in; preserve existing data by default.
    void fail(const QString& message) { QMutexLocker lock(&mutex); error = message; }
    void prune() {
        if (retentionDays <= 0 || !db.isOpen()) return;
        QSqlQuery select(db);
        select.prepare("SELECT DISTINCT json_extract(payload,'$.alert_id') FROM events WHERE "
            "CAST(json_extract(payload,'$.recovered_ms') AS INTEGER)>0 AND "
            "CAST(json_extract(payload,'$.recovered_ms') AS INTEGER)<? LIMIT 1000");
        select.addBindValue(QDateTime::currentMSecsSinceEpoch() - qint64(retentionDays)*86400000);
        if (!select.exec()) { fail(select.lastError().text()); return; }
        QStringList ids; while (select.next()) ids.append(select.value(0).toString());
        select.finish(); if (ids.isEmpty()) return;
        if (!db.transaction()) { fail(db.lastError().text()); return; }
        QSqlQuery remove(db);
        for (const auto& id : ids) for (const auto& table : {QString("events"), QString("alert_state")}) {
            remove.prepare("DELETE FROM " + table + " WHERE json_extract(payload,'$.alert_id')=?");
            remove.addBindValue(id);
            if (!remove.exec()) { fail(remove.lastError().text()); db.rollback(); return; }
        }
        if (!db.commit()) { fail(db.lastError().text()); db.rollback(); return; }
        QMetaObject::invokeMethod(frontend, [this, ids] { pruned(ids); }, Qt::QueuedConnection);
    }
    bool drain() {
        QList<QVariantMap> batch;
        { QMutexLocker lock(&mutex); batch = pending.mid(0, 256); }
        if (batch.isEmpty()) return true;
        if (!db.isOpen() || !db.transaction()) { fail(db.lastError().text()); return false; }
        QSqlQuery q(db); bool ok = true;
        for (const auto& row : batch) {
            const auto json = QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(row)).toJson(QJsonDocument::Compact));
            q.prepare("INSERT INTO events(payload) VALUES (?)"); q.addBindValue(json);
            if (!(ok = q.exec())) break;
            q.prepare("INSERT OR REPLACE INTO alert_state(rule,payload) VALUES (?,?)");
            q.addBindValue(row["rule_id"]); q.addBindValue(json);
            if (!(ok = q.exec())) break;
        }
        if (!ok) { const auto e = q.lastError().text(); db.rollback(); fail(e); return false; }
        if (!db.commit()) { fail(db.lastError().text()); db.rollback(); return false; }
        { QMutexLocker lock(&mutex);
          pending.erase(pending.begin(), pending.begin() + batch.size());
          error.clear(); }
        return true;
    }
};
AlertStore::AlertStore(const QString& path) : impl_(new Impl) {
    auto* p = impl_.get(); p->worker->moveToThread(&p->thread); p->thread.start();
    p->pruned = [this](const QStringList& ids) {
        for (auto it = rows_.begin(); it != rows_.end();) {
            if (ids.contains(it->toMap()["alert_id"].toString())) it = rows_.erase(it); else ++it;
        }
    };
    // Startup initialization precedes display; ongoing database I/O never runs on GUI thread.
    QMetaObject::invokeMethod(p->worker, [this, p, path] {
        p->db = QSqlDatabase::addDatabase("QSQLITE", p->connection);
        p->db.setDatabaseName(path); p->db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=50");
        if (!p->db.open()) { p->fail(p->db.lastError().text()); return; }
        QSqlQuery q(p->db);
        for (const auto* sql : {"PRAGMA journal_mode=WAL", "CREATE TABLE IF NOT EXISTS events (id INTEGER PRIMARY KEY, payload TEXT NOT NULL)",
             "CREATE TABLE IF NOT EXISTS alert_state (rule TEXT PRIMARY KEY, payload TEXT NOT NULL)",
             "CREATE TABLE IF NOT EXISTS metadata (key TEXT PRIMARY KEY, value TEXT NOT NULL)"}) {
            if (!q.exec(sql)) { p->fail(q.lastError().text()); return; }
        }
        bool migrated = false;
        if (q.exec("SELECT value FROM metadata WHERE key='state_migrated'") && q.next()) migrated = true;
        if (!migrated) {
            if (q.exec("SELECT payload FROM events ORDER BY id")) while (q.next()) {
                const auto row = QJsonDocument::fromJson(q.value(0).toByteArray()).object().toVariantMap();
                if (row["state"] == "Active") active_[row["rule_id"].toString()] = row;
                else active_.remove(row["rule_id"].toString());
            }
            for (const auto& row : active_) {
                QSqlQuery insert(p->db); insert.prepare("INSERT OR REPLACE INTO alert_state VALUES (?,?)");
                insert.addBindValue(row["rule_id"]);
                insert.addBindValue(QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(row)).toJson(QJsonDocument::Compact)));
                if (!insert.exec()) { p->fail(insert.lastError().text()); return; }
            }
            if (!q.exec("INSERT OR REPLACE INTO metadata VALUES ('state_migrated','1')")) p->fail(q.lastError().text());
        } else if (q.exec("SELECT payload FROM alert_state")) while (q.next()) {
            const auto row = QJsonDocument::fromJson(q.value(0).toByteArray()).object().toVariantMap();
            if (row["state"] == "Active") active_[row["rule_id"].toString()] = row;
        }
        if (q.exec("SELECT grouped.first_id,e.payload FROM "
                   "(SELECT COALESCE(json_extract(payload,'$.alert_id'),'legacy:'||id) AS alert_id, "
                   "MIN(id) AS first_id, MAX(id) AS last_id "
                   "FROM events GROUP BY alert_id) grouped "
                   "JOIN events e ON e.id=grouped.last_id ORDER BY grouped.first_id DESC LIMIT 501")) {
            int loaded = 0;
            while (q.next()) {
                if (loaded == 500) { hasMoreRows_ = true; break; }
                oldestLoadedId_ = q.value(0).toLongLong();
                rows_.append(QJsonDocument::fromJson(q.value(1).toByteArray()).object().toVariantMap());
                ++loaded;
            }
        }
        p->retentionDays = qBound(0, qEnvironmentVariableIntValue("TASK3_EVENT_RETENTION_DAYS"), 36500);
        auto* timer = new QTimer(p->worker);
        QObject::connect(timer, &QTimer::timeout, p->worker, [p] { p->drain(); }); timer->start(100);
        if (p->retentionDays > 0) {
            auto* retention = new QTimer(p->worker);
            QObject::connect(retention, &QTimer::timeout, p->worker, [p] { p->prune(); }); retention->start(60000);
        }
    }, Qt::BlockingQueuedConnection);
}
AlertStore::~AlertStore() {
    auto* p = impl_.get();
    QMetaObject::invokeMethod(p->worker, [p] {
        QElapsedTimer deadline; deadline.start();
        while (deadline.elapsed() < 5000) {
            bool empty; { QMutexLocker lock(&p->mutex); empty = p->pending.isEmpty(); }
            if (empty || !p->drain()) break;
        }
        { QMutexLocker lock(&p->mutex); if (!p->pending.isEmpty())
            qCritical() << "Event persistence incomplete at shutdown:" << p->error << "pending:" << p->pending.size(); }
        p->db.close(); p->db = QSqlDatabase(); QSqlDatabase::removeDatabase(p->connection);
    }, Qt::BlockingQueuedConnection);
    QObject::connect(&p->thread, &QThread::finished, p->worker, &QObject::deleteLater);
    p->thread.quit(); p->thread.wait();
    delete p->frontend;
}
QString AlertStore::error() const { QMutexLocker lock(&impl_->mutex); return impl_->error; }
void AlertStore::flush() { QMetaObject::invokeMethod(impl_->worker, [this] { while (impl_->drain()) {
    QMutexLocker lock(&impl_->mutex); if (impl_->pending.isEmpty()) break;
} }, Qt::BlockingQueuedConnection); }
void AlertStore::applyRetention() { QMetaObject::invokeMethod(impl_->worker, [this] { impl_->drain(); impl_->prune(); }, Qt::BlockingQueuedConnection); }
void AlertStore::policy(qint64 trigger, qint64 recovery, qint64 cooldown) {
    triggerMs_ = qMax<qint64>(0, trigger); recoveryMs_ = qMax<qint64>(0, recovery); cooldownMs_ = qMax<qint64>(0, cooldown);
}
bool AlertStore::append(const QVariantMap& row) {
    { QMutexLocker lock(&impl_->mutex);
      if (impl_->pending.size() >= 4096) { impl_->error = "Event queue full; transition will retry on next evaluation"; return false; }
      impl_->pending.append(row); }
    const auto alertId = row["alert_id"].toString();
    if (!alertId.isEmpty()) {
        for (auto it = rows_.begin(); it != rows_.end(); ++it) {
            if (it->toMap()["alert_id"] == alertId) { rows_.erase(it); break; }
        }
    }
    rows_.prepend(row); return true;
}
void AlertStore::loadOlderRows() {
    if (!hasMoreRows_ || oldestLoadedId_ <= 0) return;
    QList<QVariantMap> older;
    qint64 nextCursor = oldestLoadedId_;
    bool more = false;
    auto* p = impl_.get();
    QMetaObject::invokeMethod(p->worker, [p, this, &older, &nextCursor, &more] {
        QSqlQuery q(p->db);
        q.prepare("SELECT grouped.first_id,e.payload FROM "
                  "(SELECT COALESCE(json_extract(payload,'$.alert_id'),'legacy:'||id) AS alert_id, "
                  "MIN(id) AS first_id, MAX(id) AS last_id "
                  "FROM events GROUP BY alert_id) grouped "
                  "JOIN events e ON e.id=grouped.last_id WHERE grouped.first_id < ? "
                  "ORDER BY grouped.first_id DESC LIMIT 501");
        q.addBindValue(oldestLoadedId_);
        if (!q.exec()) { p->fail(q.lastError().text()); return; }
        int loaded = 0;
        while (q.next()) {
            if (loaded == 500) { more = true; break; }
            nextCursor = q.value(0).toLongLong();
            older.append(QJsonDocument::fromJson(q.value(1).toByteArray()).object().toVariantMap());
            ++loaded;
        }
    }, Qt::BlockingQueuedConnection);
    QSet<QString> loadedIds;
    for (const auto& value : rows_) {
        const auto id = value.toMap()["alert_id"].toString();
        if (!id.isEmpty()) loadedIds.insert(id);
    }
    for (const auto& row : older) {
        const auto id = row["alert_id"].toString();
        if (!id.isEmpty() && loadedIds.contains(id)) continue;
        rows_.append(row);
        if (!id.isEmpty()) loadedIds.insert(id);
    }
    if (older.isEmpty()) hasMoreRows_ = false;
    else { oldestLoadedId_ = nextCursor; hasMoreRows_ = more; }
}
void AlertStore::condition(const QString& id, bool failed, const QString& kind, const QString& source,
                          const QString& message, qint64 now, qint64 mono) {
    if (mono < 0) mono = now;
    if (failed == active_.contains(id)) { candidates_.remove(id); return; }
    if ((failed && active_.size() >= 8192) || (!candidates_.contains(id) && candidates_.size() >= 8192)) {
        impl_->fail("Alert state capacity reached; new transition deferred"); return;
    }
    auto c = candidates_.find(id);
    if (c == candidates_.end() || c->failed != failed) c = candidates_.insert(id, {failed, mono});
    if (mono - c->since < (failed ? triggerMs_ : recoveryMs_)) return;
    if (failed && recovered_.contains(id) && mono - recovered_[id] < cooldownMs_) return;
    QVariantMap row;
    if (failed) {
        row = {{"rule_id", id}, {"alert_id", QUuid::createUuid().toString(QUuid::WithoutBraces)},
            {"kind", kind}, {"source", source}, {"severity", kind == "time_sync" ? "warning" : "error"},
            {"message", message}, {"triggered_ms", now}, {"recovered_ms", QVariant()},
            {"time", QDateTime::fromMSecsSinceEpoch(now).toString(Qt::ISODateWithMs)}, {"state", "Active"}};
    } else {
        row = active_.value(id); row["state"] = "Recovered"; row["recovered_ms"] = now;
        row["time"] = QDateTime::fromMSecsSinceEpoch(now).toString(Qt::ISODateWithMs);
    }
    if (!append(row)) return;
    if (failed) active_[id] = row;
    else {
        active_.remove(id);
        if (cooldownMs_ > 0) recovered_[id] = mono;
        for (auto it = recovered_.begin(); it != recovered_.end();) {
            if (mono - it.value() >= cooldownMs_) it = recovered_.erase(it); else ++it;
        }
    }
    candidates_.remove(id);
}
QVariantList AlertStore::rows() const { return rows_; }
int AlertStore::activeCount(const QString& source) const {
    int count = 0;
    for (const auto& row : active_) {
        const auto owner = row["source"].toString();
        if (owner == source || owner.startsWith(source + "/")) ++count;
    }
    return count;
}
}
