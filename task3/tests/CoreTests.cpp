// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QJsonDocument>
#include "Collectors.h"
#include "Protocol.h"
#include "Registry.h"
#include "MonitoringCore.h"
#include "AlertStore.h"
#include "DdsTransport.h"
#include "Units.h"
#include "RollingSeries.h"
#include "RuntimeIdentity.h"
#include "FlowModel.h"
#include <QSaveFile>
#include <QProcess>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSysInfo>
#include <QElapsedTimer>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <atomic>
#include <dds_monitor/statistics/dynamic/DynamicDataChartBox.h>
class CoreTests : public QObject {
    Q_OBJECT
private:
    QJsonObject sample(QString host = "test-host") {
        task3::Collectors c({{"host_id", host}, {"applications", QJsonArray{}}});
        return c.collect();
    }
private slots:
    void legacyDynamicRawSnapshotRegression() {
        DynamicDataChartBox box("PUBLICATION_THROUGHPUT", 5000, 10000);
        auto* mapper = box.add_series("NONE", false, 0, "1", "", 0);
        QVERIFY(mapper);
        const auto params = box.get_update_parameters();
        QCOMPARE(params.window_size, quint64(10000));
        const auto id = params.series_ids.front();
        std::map<quint64, QVector<QPointF>> rows{{id, {{3000, 3}, {1000, 1}}}};
        box.update(rows, 6000); QCoreApplication::processEvents();
        QCOMPARE(box.get_data(id).size(), qsizetype(2));
        rows[id] = {{1000, 1}, {2000, 2}, {3000, 3}, {3000, 4}}; // late + equal-time distinct samples
        box.update(rows, 7000); QCoreApplication::processEvents();
        QCOMPARE(box.get_data(id).size(), qsizetype(4));
        QCOMPARE(box.get_data(id)[1], QPointF(2000, 2));
        box.update(rows, 8000); QCoreApplication::processEvents();
        QCOMPARE(box.get_data(id).size(), qsizetype(4));
        rows[id].clear(); box.update(rows, 9000); QCoreApplication::processEvents();
        QVERIFY(box.get_data(id).isEmpty());
    }
    void runtimeManifestIdentity() {
        QTemporaryDir dir;
        const auto exe = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
        task3::Collectors collector({{"host_id", "runtime"}, {"identity_directory", dir.path()},
            {"applications", QJsonArray{QJsonObject{{"application_id", "app"}, {"process_name", exe}}}}});
        QVERIFY(task3::writeRuntimeIdentity(dir.path(), "app", {"guid-a", "guid-b"}));
        auto s = collector.collect(); QString error;
        QVERIFY2(task3::validate(s, error), qPrintable(error));
        task3::Registry registry; QVERIFY(registry.ingest(s, "local", 0, 0, error));
        const auto pid = QString::number(QCoreApplication::applicationPid());
        QCOMPARE(registry.mapParticipant(s["hostname"].toString(), pid, "guid-a")["application_id"].toString(), QString("app"));
        QCOMPARE(registry.mapParticipant(s["hostname"].toString(), pid, "guid-b")["application_id"].toString(), QString("app"));
        QFile file(QDir(dir.path()).filePath("app-" + pid + ".json")); QVERIFY(file.open(QIODevice::ReadOnly));
        auto m = QJsonDocument::fromJson(file.readAll()).object(); file.close();
        m["instance_id"] = "previous-boot:old-start";
        QSaveFile bad(file.fileName()); QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write(QJsonDocument(m).toJson()); QVERIFY(bad.commit());
        s = collector.collect(); QVERIFY(registry.ingest(s, "local", 1, 1, error));
        QCOMPARE(registry.mapParticipant(s["hostname"].toString(), pid, "guid-a")["mapping_state"].toString(), QString("unknown"));
    }
    void ownedMultipleProcesses() {
#ifdef Q_OS_LINUX
        QTemporaryDir dir;
        const auto binary = dir.filePath("task3-owned-long-name");
        QVERIFY(QFile::copy("/bin/sleep", binary));
        QProcess first, second; first.start(binary, {"30"}); second.start(binary, {"30"});
        QVERIFY(first.waitForStarted()); QVERIFY(second.waitForStarted());
        QTest::qWait(50);
        task3::Collectors collector({{"host_id", "multi"}, {"applications", QJsonArray{
            QJsonObject{{"application_id", "sleep"}, {"process_name", "task3-owned-long-name"}}}}});
        auto app = collector.collect()["applications"].toArray()[0].toObject();
        QCOMPARE(app["instances"].toArray().size(), 2);
        QCOMPARE(app["state"].toString(), QString("running"));
        first.terminate(); QVERIFY(first.waitForFinished()); second.terminate(); QVERIFY(second.waitForFinished());
        QCOMPARE(collector.collect()["applications"].toArray()[0].toObject()["state"].toString(), QString("stopped"));
#else
        QSKIP("Owned Linux process fixture; Windows execution requires Windows host");
#endif
    }
    void topologyEvidence() {
        const qint64 now = 20000;
        QVariantMap writer{{"kind", "Writer"}, {"id", "w"}, {"guid", "wg"}, {"topic", "same"}, {"alive", true},
            {"host_id", "host-a"}, {"application_id", "app-a"}, {"metric_timestamp_ms", 19000},
            {"links", QVariantList{QVariantMap{{"reader_id", "r"}, {"latency_us", 42.0}, {"latency_timestamp_ms", 19500}}}}};
        QVariantMap reader{{"kind", "Reader"}, {"id", "r"}, {"guid", "rg"}, {"alive", true},
            {"host_id", "host-b"}, {"application_id", "app-b"}, {"topic", "same"}};
        QCOMPARE(task3::topology({writer, reader}, now)[0].toMap()["match_state"].toString(), QString("Unknown"));
        writer["connection_status_timestamp_ms"] = now; writer["connection_guids"] = QStringList{"rg"};
        auto flow = task3::topology({writer, reader}, now)[0].toMap();
        QVERIFY(flow["confirmed"].toBool()); QCOMPARE(flow["target_host"].toString(), QString("host-b"));
        QCOMPARE(flow["throughput_timestamp_ms"].toLongLong(), qint64(19000));
        QCOMPARE(flow["latency_timestamp_ms"].toLongLong(), qint64(19500));
        writer["connection_guids"] = QStringList{};
        QCOMPARE(task3::topology({writer, reader}, now)[0].toMap()["match_state"].toString(), QString("Unmatched"));
        QCOMPARE(task3::topology({writer, reader}, now+15000)[0].toMap()["match_state"].toString(), QString("Unknown"));
    }
    void serviceAndWindowsTimeEvidence() {
        QCOMPARE(task3::Collectors::parseService("active", true), QString("running"));
        QCOMPARE(task3::Collectors::parseService("inactive", true), QString("stopped"));
        QCOMPARE(task3::Collectors::parseService("failed", true), QString("failed"));
        QCOMPARE(task3::Collectors::parseService("activating", true), QString("unknown"));
        QCOMPARE(task3::Collectors::parseService("active", false), QString("unknown"));
        auto time = task3::Collectors::parseWindowsTime({{"service_running", true}, {"source", "master"}, {"time_sources", 1}, {"offset_us", 1250}}, 99);
        QCOMPARE(time["offset_seconds"].toDouble(), 0.00125);
        QCOMPARE(time["state"].toString(), QString("unknown")); QVERIFY(time["clock_synced"].isNull());
        time = task3::Collectors::parseWindowsTime({}, 99); QVERIFY(time["offset_seconds"].isNull());
        QCOMPARE(task3::Collectors::parseWindowsTime({{"service_running", false}}, 99)["state"].toString(), QString("unhealthy"));
    }
    void asynchronousTimeStatusCacheDoesNotBlockCollection() {
        std::atomic_int calls{0};
        task3::Collectors collector({{"host_id", "async-time"}, {"applications", QJsonArray{}}}, [&] {
            ++calls;
            QThread::msleep(250);
            return QJsonObject{{"state", "unknown"}, {"detail", "fixture query failure"}};
        }, 1000);
        QElapsedTimer elapsed; elapsed.start();
        auto first = collector.collect();
        QVERIFY(elapsed.elapsed() < 150);
        QCOMPARE(first["time_sync"].toObject()["state"].toString(), QString("unknown"));
        QCOMPARE(first["time_sync"].toObject()["offset_seconds"].isNull(), true);
        int timerTicks = 0; QTimer timer; connect(&timer, &QTimer::timeout, [&] { ++timerTicks; }); timer.start(10);
        QTRY_COMPARE_WITH_TIMEOUT(calls.load(), 1, 2000);
        QTRY_VERIFY_WITH_TIMEOUT(collector.collect()["time_sync"].toObject()["detail"].toString() == "fixture query failure", 2000);
        timer.stop(); QVERIFY(timerTicks >= 10);
        for (int i = 0; i < 5; ++i) collector.collect();
        QCOMPARE(calls.load(), 1); // The cached result is reused until the refresh interval.
    }
    void chronyCommandTimeoutIsBounded() {
#ifndef Q_OS_WIN
        QTemporaryDir dir; QVERIFY(dir.isValid());
        const auto executable = dir.filePath("chronyc");
        QFile script(executable); QVERIFY(script.open(QIODevice::WriteOnly));
        script.write("#!/bin/sh\nexec sleep 5\n"); script.close();
        QVERIFY(script.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        const auto oldPath = qgetenv("PATH"); qputenv("PATH", dir.path().toUtf8() + ":" + oldPath);
        task3::Collectors collector({{"host_id", "slow-chrony"}, {"applications", QJsonArray{}}});
        int heartbeats = 0; QTimer heartbeat; connect(&heartbeat, &QTimer::timeout, [&] { ++heartbeats; collector.collect(); });
        heartbeat.start(50); QTest::qWait(1200); heartbeat.stop();
        const auto status = collector.collect()["time_sync"].toObject();
        qputenv("PATH", oldPath);
        QVERIFY(heartbeats >= 15); // The agent's periodic collection remains responsive during the 700 ms command timeout.
        QCOMPARE(status["state"].toString(), QString("unknown"));
        QVERIFY(status["detail"].toString().contains("timed out"));
#endif
    }
    void periodicRuntimeCounterAndEventIdle() {
        QTemporaryDir dir;
        const auto config = dir.filePath("flows.json");
        auto rules = [&](const QString& mode) {
            QSaveFile file(config); if (!file.open(QIODevice::WriteOnly)) return false;
            file.write(QJsonDocument(QJsonArray{QJsonObject{{"topic", "periodic"}, {"mode", mode}, {"deadline_ms", 100}}}).toJson());
            return file.commit();
        };
        QVERIFY(rules("periodic")); qputenv("TASK3_FLOW_RULES", config.toUtf8());
        auto s = sample();
        QJsonObject instance{{"pid", 123}, {"instance_id", "boot:run"}, {"state", "running"},
            {"participant_guids", QJsonArray{"participant-guid"}}, {"binding_source", "runtime_manifest"},
            {"writer_counts", QJsonObject{{"writer-guid", "1"}}}};
        s["applications"] = QJsonArray{QJsonObject{{"application_id", "app"}, {"pid", 123}, {"instance_id", "boot:run"},
            {"state", "running"}, {"service_state", "not_configured"}, {"instances", QJsonArray{instance}}}};
        QVariantMap writer{{"kind", "Writer"}, {"id", "w"}, {"guid", "writer-guid"}, {"topic", "periodic"},
            {"alive", true}, {"participant_ids", QVariantList{"p"}}};
        QVariantMap participant{{"kind", "Participant"}, {"id", "p"}, {"guid", "participant-guid"},
            {"hostname", s["hostname"].toString()}, {"pid", "123"}, {"alive", true}};
        {
            task3::MonitoringCore core(dir.filePath("periodic.sqlite"));
            QVERIFY(core.ingest(task3::encode(s), "test"));
            core.updateDds({writer, participant}); // Deliberately endpoint before parent.
            QCOMPARE(core.entities()[0].toMap()["application_id"].toString(), QString("app"));
            QTest::qWait(150); core.tick();
            QVERIFY(core.entities()[0].toMap()["data_state"].toString().startsWith("No Data"));
            bool noData = false;
            for (const auto& row : core.events()) noData |= row.toMap()["kind"] == "dds_no_data";
            QVERIFY(noData);
            instance["writer_counts"] = QJsonObject{{"writer-guid", "2"}};
            auto app = s["applications"].toArray()[0].toObject(); app["instances"] = QJsonArray{instance};
            s["applications"] = QJsonArray{app}; s["sequence"] = "2";
            QVERIFY(core.ingest(task3::encode(s), "test"));
            QCOMPARE(core.events()[0].toMap()["state"].toString(), QString("Recovered"));
        }
        QVERIFY(rules("event"));
        {
            task3::MonitoringCore core(dir.filePath("event.sqlite"));
            QVERIFY(core.ingest(task3::encode(s), "test")); core.updateDds({writer, participant});
            QTest::qWait(150); core.tick();
            QCOMPARE(core.entities()[0].toMap()["data_state"].toString(), QString("Event idle (allowed)"));
            for (const auto& row : core.events()) QVERIFY(row.toMap()["kind"] != "dds_no_data");
        }
        qunsetenv("TASK3_FLOW_RULES");
    }
    void serviceTransitionsRemainIndependent() {
        QTemporaryDir dir; task3::MonitoringCore core(dir.filePath("events.sqlite"));
        auto s = sample();
        QJsonObject app{{"application_id", "svc"}, {"pid", 0}, {"instance_id", ""}, {"state", "stopped"}, {"service_state", "running"}};
        auto send = [&](int sequence) {
            s["applications"] = QJsonArray{app}; s["sequence"] = QString::number(sequence);
            return core.ingest(task3::encode(s), "test");
        };
        QVERIFY(send(1)); QCOMPARE(core.events().size(), 1); // Process absent; service is independently running.
        app["service_state"] = "failed"; QVERIFY(send(2)); QCOMPARE(core.events().size(), 2);
        app["service_state"] = "unknown"; QVERIFY(send(3)); QCOMPARE(core.events().size(), 2);
        app["service_state"] = "running"; QVERIFY(send(4)); QCOMPARE(core.events().size(), 2);
        QCOMPARE(core.events()[0].toMap()["kind"].toString(), QString("service_stopped"));
        QCOMPARE(core.events()[0].toMap()["state"].toString(), QString("Recovered"));
    }
    void conflictingRuntimeMetadataUnmapped() {
        task3::Registry registry; auto s = sample(); QString error;
        QJsonObject process{{"pid", 55}, {"instance_id", "boot:55"}, {"state", "running"},
            {"participant_guids", QJsonArray{"g"}}, {"binding_source", "runtime_manifest"}};
        QJsonObject app{{"application_id", "a"}, {"pid", 55}, {"instance_id", "boot:55"},
            {"state", "running"}, {"service_state", "not_configured"}, {"instances", QJsonArray{process}}};
        auto second = app; second["application_id"] = "b";
        s["applications"] = QJsonArray{app, second}; QVERIFY(registry.ingest(s, "test", 0, 0, error));
        QCOMPARE(registry.mapParticipant(s["hostname"].toString(), "55", "g")["mapping_state"].toString(), QString("unknown"));
        process["binding_conflict"] = true; app["instances"] = QJsonArray{process};
        s["applications"] = QJsonArray{app}; s["sequence"] = "2"; QVERIFY(registry.ingest(s, "test", 1, 1, error));
        QCOMPARE(registry.mapParticipant(s["hostname"].toString(), "55", "g")["mapping_state"].toString(), QString("unknown"));
    }
    void alertDebounceCooldown() {
        QTemporaryDir dir; task3::AlertStore store(dir.filePath("events.sqlite")); store.policy(100, 100, 300);
        auto condition = [&](bool failed, qint64 mono) { store.condition("rule", failed, "test", "host", "test", 10000+mono, mono); };
        condition(true, 0); condition(true, 99); QCOMPARE(store.rows().size(), 0);
        condition(false, 99); condition(true, 100); condition(true, 200); QCOMPARE(store.rows().size(), 1);
        condition(false, 201); condition(false, 301); QCOMPARE(store.rows().size(), 1);
        condition(true, 302); condition(true, 500); QCOMPARE(store.rows().size(), 1);
        condition(true, 601); QCOMPARE(store.rows().size(), 2);
    }
    void unknownEvidenceResetsPendingDebounce() {
        QTemporaryDir dir; task3::AlertStore store(dir.filePath("events.sqlite")); store.policy(100,0,0);
        store.condition("r",true,"test","h","test",1000,0);
        store.unknown("r");
        store.condition("r",true,"test","h","test",1100,100); QVERIFY(store.rows().isEmpty());
        store.condition("r",true,"test","h","test",1200,200); QCOMPARE(store.rows().size(),1);
        store.unknown("r"); QVERIFY(store.isActive("r"));
    }
    void corruptDatabaseReportsErrorWithoutCrash() {
        QTemporaryDir dir; const auto path = dir.filePath("broken.sqlite");
        QSaveFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("not a SQLite database"); QVERIFY(file.commit());
        task3::AlertStore store(path); QVERIFY(!store.error().isEmpty());
        store.condition("r",true,"test","h","test",1000); QCOMPARE(store.rows().size(),1);
        QTest::ignoreMessage(QtCriticalMsg, QRegularExpression("Event persistence incomplete at shutdown:.*"));
    }
    void corruptEventDatabaseDoesNotBlockTelemetry() {
        QTemporaryDir dir; auto s = sample(); const auto path = dir.filePath("broken.sqlite");
        QSaveFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("broken database"); QVERIFY(file.commit());
        task3::MonitoringCore core(path);
        QVERIFY(core.ingest(task3::encode(s),"test"));
        QCOMPARE(core.hosts().size(), 1); QVERIFY(core.hosts()[0].toMap()["online"].toBool());
        QVERIFY(core.status().contains("Event database"));
    }
    void throughputHysteresis() {
        qputenv("TASK3_THROUGHPUT_MIN","10"); qputenv("TASK3_THROUGHPUT_HYSTERESIS","2");
        QTemporaryDir dir; task3::MonitoringCore core(dir.filePath("events.sqlite"));
        QVariantMap writer{{"kind","Writer"},{"id","threshold"},{"alive",true},{"metric_fresh",true},
            {"metric_timestamp_ms",QDateTime::currentMSecsSinceEpoch()},{"throughput_bps",9.0}};
        core.updateDds({writer}); QCOMPARE(core.events().size(),1);
        writer["throughput_bps"]=10.5; core.updateDds({writer}); QCOMPARE(core.events().size(),1);
        writer["throughput_bps"]=12.0; core.updateDds({writer}); QCOMPARE(core.events().size(),1);
        QCOMPARE(core.events()[0].toMap()["state"].toString(),QString("Recovered"));
        qunsetenv("TASK3_THROUGHPUT_MIN"); qunsetenv("TASK3_THROUGHPUT_HYSTERESIS");
    }
    void sqliteWorkerLockAndShutdown() {
        QTemporaryDir dir; const auto path = dir.filePath("events.sqlite");
        {
            task3::AlertStore store(path);
            auto db = QSqlDatabase::addDatabase("QSQLITE", "lock-test"); db.setDatabaseName(path); QVERIFY(db.open());
            { QSqlQuery lock(db); QVERIFY(lock.exec("BEGIN IMMEDIATE")); }
            int ticks = 0; QTimer timer; connect(&timer, &QTimer::timeout, [&] { ++ticks; }); timer.start(5);
            for (int i = 0; i < 3000; ++i) store.condition(QString::number(i), true, "test", "host", "lock-test", 1000+i);
            QTest::qWait(180); QVERIFY(ticks >= 10); QCOMPARE(store.rows().size(), 3000);
            { QSqlQuery unlock(db); QVERIFY(unlock.exec("COMMIT")); }
            db.close(); db = QSqlDatabase(); QSqlDatabase::removeDatabase("lock-test");
        }
        task3::AlertStore reopened(path); QVERIFY2(reopened.error().isEmpty(), qPrintable(reopened.error()));
        QCOMPARE(reopened.rows().size(), 500); QVERIFY(reopened.hasMoreRows());
        QVERIFY(reopened.isActive("0")); QVERIFY(reopened.isActive("2999"));
        reopened.loadOlderRows(); QCOMPARE(reopened.rows().size(), 1000); QVERIFY(reopened.hasMoreRows());
        while (reopened.hasMoreRows()) reopened.loadOlderRows();
        QCOMPARE(reopened.rows().size(), 3000);
    }
    void pagedHistoryIncludesRecoveryForOlderAlert() {
        QTemporaryDir dir; const auto path = dir.filePath("paged-history.sqlite");
        {
            task3::AlertStore store(path);
            for (int i = 0; i < 501; ++i)
                store.condition(QString::number(i), true, "test", "host", "condition", 1000 + i);
            store.condition("0", false, "test", "host", "condition", 2000);
            store.flush();
        }
        task3::AlertStore reopened(path);
        QCOMPARE(reopened.rows().size(), 500); QVERIFY(reopened.hasMoreRows());
        reopened.loadOlderRows();
        QCOMPARE(reopened.rows().size(), 501); QVERIFY(!reopened.hasMoreRows());
        bool foundRecovered = false;
        for (const auto& value : reopened.rows()) {
            const auto row = value.toMap();
            if (row["rule_id"] == "0") {
                foundRecovered = row["state"] == "Recovered" && !row["recovered_ms"].isNull();
            }
        }
        QVERIFY(foundRecovered);
    }
    void pagedHistoryDoesNotReintroducePendingRecovery() {
        QTemporaryDir dir; const auto path = dir.filePath("pending-recovery.sqlite");
        {
            task3::AlertStore seed(path);
            for (int i = 0; i < 501; ++i)
                seed.condition(QString::number(i), true, "test", "host", "condition", 1000 + i);
            seed.flush();
        }
        {
            task3::AlertStore store(path);
            QCOMPARE(store.rows().size(), 500); QVERIFY(store.hasMoreRows());
            auto db = QSqlDatabase::addDatabase("QSQLITE", "pending-recovery-lock");
            db.setDatabaseName(path); QVERIFY(db.open());
            { QSqlQuery lock(db); QVERIFY(lock.exec("BEGIN IMMEDIATE")); }
            store.condition("0", false, "test", "host", "condition", 2000);
            QVERIFY(!store.isActive("0"));
            const auto recoveredId = store.rows().first().toMap()["alert_id"].toString();
            store.loadOlderRows();
            QCOMPARE(store.rows().size(), 501);
            QVERIFY(!store.hasMoreRows());
            int matching = 0;
            for (const auto& value : store.rows()) {
                const auto row = value.toMap();
                if (row["alert_id"] != recoveredId) continue;
                ++matching;
                QCOMPARE(row["state"].toString(), QString("Recovered"));
                QVERIFY(!row["recovered_ms"].isNull());
            }
            QCOMPARE(matching, 1);
            { QSqlQuery unlock(db); QVERIFY(unlock.exec("COMMIT")); }
            db.close(); db = QSqlDatabase(); QSqlDatabase::removeDatabase("pending-recovery-lock");
            store.flush();
        }
        task3::AlertStore reopened(path);
        QCOMPARE(reopened.rows().size(), 500); QVERIFY(reopened.hasMoreRows());
        reopened.loadOlderRows();
        QCOMPARE(reopened.rows().size(), 501); QVERIFY(!reopened.hasMoreRows());
        int matching = 0;
        for (const auto& value : reopened.rows()) if (value.toMap()["rule_id"] == "0") {
            ++matching;
            QCOMPARE(value.toMap()["state"].toString(), QString("Recovered"));
        }
        QCOMPARE(matching, 1);
    }
    void legacySqliteUpgradePreservesHistory() {
        QTemporaryDir dir; const auto path = dir.filePath("legacy.sqlite");
        auto db = QSqlDatabase::addDatabase("QSQLITE", "legacy-test"); db.setDatabaseName(path); QVERIFY(db.open());
        {
            QSqlQuery query(db); QVERIFY(query.exec("CREATE TABLE events (id INTEGER PRIMARY KEY,payload TEXT NOT NULL)"));
            const QJsonObject row{{"rule_id","legacy"},{"alert_id","legacy-id"},{"source","host"},{"kind","test"},
                {"state","Active"},{"triggered_ms",1000},{"message","original event"}};
            query.prepare("INSERT INTO events(payload) VALUES (?)"); query.addBindValue(QString::fromUtf8(QJsonDocument(row).toJson()));
            QVERIFY(query.exec());
        }
        db.close(); db=QSqlDatabase(); QSqlDatabase::removeDatabase("legacy-test");
        {
            task3::AlertStore store(path); QVERIFY2(store.error().isEmpty(),qPrintable(store.error()));
            QCOMPARE(store.rows().size(),1); QVERIFY(store.isActive("legacy"));
            store.condition("legacy",true,"test","host","original event",1100); QCOMPARE(store.rows().size(),1);
            store.condition("legacy",false,"test","host","original event",1200); QCOMPARE(store.rows().size(),1);
        }
        task3::AlertStore reopened(path); QCOMPARE(reopened.rows().size(),1); QVERIFY(!reopened.isActive("legacy"));
    }
    void masterRestartAcceptsTelemetryWithoutPersistentReplayState() {
        QTemporaryDir dir; const auto path = dir.filePath("events.sqlite"); auto s = sample();
        { task3::MonitoringCore core(path); QVERIFY(core.ingest(task3::encode(s), "test")); }
        {
            auto db = QSqlDatabase::addDatabase("QSQLITE", "replay-check-test"); db.setDatabaseName(path); QVERIFY(db.open());
            QSqlQuery query(db); QVERIFY(query.exec("SELECT COUNT(*) FROM metadata WHERE key='replay'"));
            QVERIFY(query.next()); QCOMPARE(query.value(0).toInt(), 0);
            db.close(); db = QSqlDatabase(); QSqlDatabase::removeDatabase("replay-check-test");
        }
        { task3::MonitoringCore core(path);
          QVERIFY(core.ingest(task3::encode(s), "after-master-restart")); }
    }
    void repeatedAgentRestartsStayBounded() {
        task3::Registry registry; auto s = sample(); QString error;
        const auto start = s["timestamp_ms"].toDouble();
        auto first = s;
        // The replay fixture must be a session this Master has actually observed.
        QVERIFY(registry.ingest(first, "initial-session", 0, 0, error));
        for (int i = 0; i < 200; ++i) {
            s["session_id"] = "session-" + QString::number(i); s["timestamp_ms"] = start+i;
            QVERIFY2(registry.ingest(s, "test", i, i, error), qPrintable(error));
        }
        first["sequence"] = "999999";
        QVERIFY(!registry.ingest(first, "replay", 201, 201, error));
        QCOMPARE(registry.hosts().first().sessionId, s["session_id"].toString());
        for (int i = 200; i < 300; ++i) {
            s["session_id"] = "session-" + QString::number(i); s["timestamp_ms"] = start-i;
            QVERIFY(registry.ingest(s, "clock-backward", i, i, error));
        }
        QCOMPARE(registry.hosts().size(), 1);
        QCOMPARE(registry.hosts().first().retiredSessions.size(), 256);
        auto retired = s; retired["session_id"] = "session-298"; retired["sequence"] = "999999";
        QVERIFY(!registry.ingest(retired, "retired-replay", 301, 301, error));
    }
    void agentRestartAfterClockRollbackRecovers() {
        QTemporaryDir dir; task3::MonitoringCore core(dir.filePath("events.sqlite"), 120);
        auto before = sample("rollback-host");
        before["timestamp_ms"] = 100000.0; before["sequence"] = "10";
        QVERIFY(core.ingest(task3::encode(before), "test"));
        QTest::qWait(160); core.tick();
        QVERIFY(!core.hosts()[0].toMap()["online"].toBool());
        auto restarted = before;
        restarted["session_id"] = "new-session-after-rollback";
        restarted["sequence"] = "1"; restarted["timestamp_ms"] = 40000.0;
        QVERIFY(core.ingest(task3::encode(restarted), "test"));
        QVERIFY(core.hosts()[0].toMap()["online"].toBool());
        bool recovered = false;
        for (const auto& event : core.events())
            recovered |= event.toMap()["kind"] == "agent_timeout" && event.toMap()["state"] == "Recovered";
        QVERIFY(recovered);
        QVERIFY(!core.ingest(task3::encode(restarted), "duplicate"));
        restarted["sequence"] = "2"; restarted["timestamp_ms"] = 39000.0;
        QVERIFY(core.ingest(task3::encode(restarted), "same-session-clock-backward"));
        restarted["sequence"] = "1";
        QVERIFY(!core.ingest(task3::encode(restarted), "old-sequence"));
        before["sequence"] = "999999"; before["timestamp_ms"] = 200000.0;
        QVERIFY(!core.ingest(task3::encode(before), "retired-session"));
        restarted["schema_version"] = 99;
        QVERIFY(!core.ingest(task3::encode(restarted), "invalid-schema"));
    }
    void activeAlertsAreUnverifiedWithoutCurrentEvidence_data() {
        QTest::addColumn<bool>("missingApplication");
        QTest::newRow("unknown-process-and-service") << false;
        QTest::newRow("application-missing-from-snapshot") << true;
    }
    void activeAlertsAreUnverifiedWithoutCurrentEvidence() {
        QFETCH(bool, missingApplication);
        QTemporaryDir dir; task3::MonitoringCore core(dir.filePath("events.sqlite"));
        auto s = sample("unknown-alert-host");
        QJsonObject app{{"application_id", "app"}, {"pid", 0}, {"state", "stopped"},
            {"service_state", "failed"}, {"service", "example.service"}};
        s["applications"] = QJsonArray{app};
        QVERIFY(core.ingest(task3::encode(s), "stopped"));
        QCOMPARE(core.events().size(), 2);
        QMap<QString, QVariantMap> initial;
        for (const auto& value : core.events()) {
            const auto row = value.toMap(); initial[row["kind"].toString()] = row;
            QCOMPARE(row["verification"].toString(), QString("Verified by current Agent telemetry"));
        }
        app["state"] = "unknown"; app["service_state"] = "unknown";
        s["applications"] = missingApplication ? QJsonArray{} : QJsonArray{app};
        s["sequence"] = "2";
        QVERIFY(core.ingest(task3::encode(s), "unknown"));
        for (const auto& value : core.events()) {
            const auto row = value.toMap(); const auto first = initial[row["kind"].toString()];
            QCOMPARE(row["state"].toString(), QString("Active"));
            QCOMPARE(row["alert_id"], first["alert_id"]); QCOMPARE(row["triggered_ms"], first["triggered_ms"]);
            QVERIFY(row["recovered_ms"].isNull());
            QVERIFY(row["verification"].toString().startsWith("Not verified"));
        }
        app["state"] = "running"; app["pid"] = 1; app["instance_id"] = "test-run";
        app["service_state"] = "running";
        s["applications"] = QJsonArray{app}; s["sequence"] = "3";
        QVERIFY(core.ingest(task3::encode(s), "running"));
        QCOMPARE(core.events().size(), 2);
        for (const auto& value : core.events()) {
            const auto row = value.toMap();
            QCOMPARE(row["state"].toString(), QString("Recovered"));
            QCOMPARE(row["alert_id"], initial[row["kind"].toString()]["alert_id"]);
            QVERIFY(!row["recovered_ms"].isNull());
        }
    }
    void missingApplicationResetsPendingDebounce_data() {
        QTest::addColumn<bool>("missingApplication");
        QTest::newRow("missing") << true;
        QTest::newRow("explicit-unknown") << false;
    }
    void missingApplicationResetsPendingDebounce() {
        QFETCH(bool, missingApplication);
        const auto previous = qgetenv("TASK3_ALERT_TRIGGER_MS");
        struct RestoreTrigger {
            QByteArray value;
            ~RestoreTrigger() { if (value.isNull()) qunsetenv("TASK3_ALERT_TRIGGER_MS");
                                else qputenv("TASK3_ALERT_TRIGGER_MS", value); }
        } restore{previous};
        qputenv("TASK3_ALERT_TRIGGER_MS", "300");
        QTemporaryDir dir; task3::MonitoringCore core(dir.filePath("debounce.sqlite"));
        auto s = sample("debounce-host");
        QJsonObject stopped{{"application_id", "app"}, {"pid", 0}, {"state", "stopped"},
            {"service_state", "failed"}, {"service", "example.service"}};
        auto send = [&](int seq, const QJsonArray& apps) {
            s["sequence"] = QString::number(seq); s["applications"] = apps;
            return core.ingest(task3::encode(s), "fixture");
        };
        QVERIFY(send(1, QJsonArray{stopped}));
        QTest::qWait(100);
        auto unknown = stopped; unknown["state"] = "unknown"; unknown["service_state"] = "unknown";
        const QJsonArray absent = missingApplication ? QJsonArray{} : QJsonArray{unknown};
        QVERIFY(send(2, absent));
        QTest::qWait(250);
        QVERIFY(send(3, QJsonArray{stopped}));
        QCOMPARE(core.events().size(), 0); // Only a new, uninterrupted interval may trigger.
        QTest::qWait(320); core.tick();
        QCOMPARE(core.events().size(), 2);
        QMap<QString, QString> ids;
        for (const auto& value : core.events()) {
            const auto row = value.toMap();
            QCOMPARE(row["state"].toString(), QString("Active"));
            ids[row["kind"].toString()] = row["alert_id"].toString();
            QCOMPARE(row["verification"].toString(), QString("Verified by current Agent telemetry"));
        }
        QVERIFY(send(4, absent));
        for (const auto& value : core.events()) {
            const auto row = value.toMap();
            QCOMPARE(row["state"].toString(), QString("Active"));
            QCOMPARE(row["alert_id"].toString(), ids[row["kind"].toString()]);
            QVERIFY(row["verification"].toString().startsWith("Not verified"));
        }
        auto running = stopped; running["pid"] = 1; running["instance_id"] = "run-1";
        running["state"] = "running"; running["service_state"] = "running";
        QVERIFY(send(5, QJsonArray{running}));
        for (const auto& value : core.events()) {
            const auto row = value.toMap();
            QCOMPARE(row["state"].toString(), QString("Recovered"));
            QCOMPARE(row["alert_id"].toString(), ids[row["kind"].toString()]);
            QVERIFY(!row["recovered_ms"].isNull());
        }
        task3::MonitoringCore interrupted(dir.filePath("interrupted.sqlite"));
        auto interruptedSample = sample("interrupted-host");
        interruptedSample["applications"] = QJsonArray{stopped};
        QVERIFY(interrupted.ingest(task3::encode(interruptedSample), "stopped"));
        QTest::qWait(100);
        interruptedSample["applications"] = absent; interruptedSample["sequence"] = "2";
        QVERIFY(interrupted.ingest(task3::encode(interruptedSample), "unavailable"));
        QTest::qWait(250);
        interruptedSample["applications"] = QJsonArray{running}; interruptedSample["sequence"] = "3";
        QVERIFY(interrupted.ingest(task3::encode(interruptedSample), "running"));
        QCOMPARE(interrupted.events().size(), 0);
        qputenv("TASK3_ALERT_TRIGGER_MS", "0");
        task3::MonitoringCore defaultCore(dir.filePath("default.sqlite"));
        QVERIFY(defaultCore.ingest(task3::encode(s), "running"));
        auto immediate = s; immediate["sequence"] = "6"; immediate["applications"] = QJsonArray{stopped};
        QVERIFY(defaultCore.ingest(task3::encode(immediate), "stopped"));
        QCOMPARE(defaultCore.events().size(), 2);
    }
    void retentionOptInPreservesActiveAndMigration() {
        QTemporaryDir dir; const auto path = dir.filePath("events.sqlite");
        {
            task3::AlertStore store(path);
            store.condition("old", true, "test", "h", "old", 1000);
            store.condition("old", false, "test", "h", "old", 1001);
            store.condition("active", true, "test", "h", "active", 1000);
        }
        { task3::AlertStore store(path); store.applyRetention(); QCoreApplication::processEvents(); QCOMPARE(store.rows().size(), 2); }
        qputenv("TASK3_EVENT_RETENTION_DAYS", "1");
        {
            task3::AlertStore store(path); store.applyRetention(); QCoreApplication::processEvents();
            QCOMPARE(store.rows().size(), 1); QVERIFY(store.isActive("active"));
        }
        qunsetenv("TASK3_EVENT_RETENTION_DAYS");
        { task3::AlertStore store(path); QCOMPARE(store.rows().size(), 1); QVERIFY(store.isActive("active")); }
    }
    void protocolAndRegistryLoad() {
        task3::Registry registry; auto s = sample();
        QJsonArray apps;
        for (int i = 0; i < 32; ++i) apps.append(QJsonObject{{"application_id", "app-"+QString::number(i)},
            {"pid", 0}, {"state", "stopped"}, {"service_state", "unknown"}});
        s["applications"] = apps; QString error; QJsonObject decoded;
        QElapsedTimer timer; timer.start();
        for (int round = 1; round <= 8; ++round) for (int host = 0; host < 128; ++host) {
            s["host_id"] = "load-"+QString::number(host); s["sequence"] = QString::number(round);
            QVERIFY(task3::decode(task3::encode(s),decoded,error));
            QVERIFY(registry.ingest(decoded, "test", round, round, error));
        }
        QCOMPARE(registry.rows().size(), 128);
        for (int i = 0; i < 1000; ++i) QVERIFY(!task3::decode("bad-data",decoded,error));
        qInfo() << "Load sample: 1024 telemetry snapshots, 128 synthetic hosts x32 applications; ms=" << timer.elapsed();
    }
    void boundedDdsDispatch() {
        const auto topic = QString("Task3/Test/Queue/%1").arg(QCoreApplication::applicationPid());
        task3::DdsTransport reader(false, 98, topic), writer(true, 98, topic);
        QVERIFY(reader.valid()); QVERIFY(writer.valid());
        QSignalSpy received(&reader, &task3::DdsTransport::received);
        QTimer publishing; connect(&publishing, &QTimer::timeout, [&] { writer.send("discovery"); }); publishing.start(50);
        QTRY_VERIFY_WITH_TIMEOUT(!received.isEmpty(), 12000); publishing.stop();
        // No event-loop dispatch while the native DDS listener fills the bounded queue.
        for (int i = 0; i < 2000; ++i) QVERIFY(writer.send(QByteArray(512, 'x')));
        QTRY_VERIFY_WITH_TIMEOUT(reader.droppedPackets() > 0, 3000);
        qInfo() << "DDS overload dropped full snapshots:" << reader.droppedPackets();
    }
    void configuredUnknownToOnline() {
        task3::Registry registry;
        registry.expect({"configured-host"});
        registry.expire(6000, 5000);
        QCOMPARE(registry.rows()[0].toMap()["metrics_state"].toString(), QString("unknown"));
        auto s = sample("configured-host"); QString error;
        QVERIFY(registry.ingest(s, "DDS", 7000, 10000, error));
        QVERIFY(registry.rows()[0].toMap()["online"].toBool());
    }
    void cpuSnapshotLifetimeRegression() {
        QTemporaryDir dir;
        task3::MonitoringCore core(dir.filePath("events.sqlite"));
        task3::Collectors collector({{"host_id", "cpu-regression"}, {"applications", QJsonArray{}}});
        QVERIFY(core.ingest(task3::encode(collector.collect()), "local"));
        QTest::qWait(20);
        QVERIFY(core.ingest(task3::encode(collector.collect()), "local"));
        QCOMPARE(core.hosts()[0].toMap()["cpu_history"].toList().size(), 1);
    }
    void lateSamplesRollingWindow() {
        task3::RollingSeries series(10000);
        series.refresh({{1000, 1}, {3000, 3}}, 5000);
        QCOMPARE(series.points().size(), size_t(2));
        series.refresh({{3000, 3}, {1000, 1}, {2000, 2}}, 6000); // Sample 2 arrived after original cursor.
        QCOMPARE(series.points().size(), size_t(3));
        QCOMPARE(series.points()[1].second, 2.0);
        series.refresh({{1000, 1}, {2000, 2}, {3000, 3}}, 6000);
        QCOMPARE(series.points().size(), size_t(3)); // Re-query does not append duplicates.
        series.refresh({{1000, 1}, {2000, 2}, {3000, 3}}, 13001);
        QCOMPARE(series.points().size(), size_t(0));
    }
    void chronyParsing() {
        auto synced = task3::Collectors::parseChrony("C0A80101,2,12345,0.00001,0.00002,0.00003,1,0,0.1,0.01,0.02,64,Normal", 123);
        QCOMPARE(synced["state"].toString(), QString("synced"));
        QCOMPARE(synced["system_time_seconds"].toDouble(), 0.00001);
        QCOMPARE(synced["offset_seconds"].toDouble(), 0.00002);
        QCOMPARE(synced["offset_kind"].toString(), QString("chrony_last_offset"));
        QVERIFY(synced["master_source_verified"].isNull());
        QCOMPARE(task3::Collectors::parseChrony("invalid", 0)["state"].toString(), QString("unknown"));
        QCOMPARE(task3::Collectors::parseChrony("00000000,0,0,0,0,0,0,0,0,0,0,0,Not synchronised", 0)["state"].toString(), QString("unhealthy"));
    }
    void units() {
        QCOMPARE(task3::bytesPerSecond(1, task3::ThroughputUnit::MegabitsPerSecond), 125000.0);
        QCOMPARE(task3::bytesPerSecond(239, task3::ThroughputUnit::BytesPerSecond), 239.0);
        QCOMPARE(task3::microseconds(1200), 1.2);
        QCOMPARE(task3::throughputMeasurement(239)["display_unit"].toString(), QString("B/s"));
    }
    void noDataIsNotStatisticsStale() {
        QTemporaryDir dir;
        task3::MonitoringCore core(dir.filePath("events.sqlite"));
        qputenv("TASK3_EXPECTED_TOPICS", "test-periodic");
        qputenv("TASK3_DDS_TIMEOUT_MS", "1000");
        QVariantMap writer{{"kind", "Writer"}, {"id", "1"}, {"alive", true}, {"topic", "test-periodic"},
            {"metric_fresh", false}};
        core.updateDds({writer});
        QTest::qWait(1100); core.tick();
        bool stale = false, noData = false;
        for (auto e : core.events()) {
            stale |= e.toMap()["kind"] == "statistics_stale";
            noData |= e.toMap()["kind"] == "dds_no_data";
        }
        QVERIFY(stale); QVERIFY(!noData);
        writer["alive"] = false; core.updateDds({writer});
        QTest::qWait(1100); core.tick();
        for (auto e : core.events()) noData |= e.toMap()["kind"] == "dds_no_data";
        QVERIFY(noData);
        writer["alive"] = true; writer["metric_fresh"] = true;
        writer["metric_timestamp_ms"] = QDateTime::currentMSecsSinceEpoch();
        core.updateDds({writer});
        bool recovered = false;
        for (auto e : core.events())
            recovered |= e.toMap()["kind"] == "dds_no_data" && e.toMap()["state"] == "Recovered";
        QVERIFY(recovered);
        qunsetenv("TASK3_EXPECTED_TOPICS"); qunsetenv("TASK3_DDS_TIMEOUT_MS");
    }
    void localDdsTelemetryTimeoutAndRecovery() {
        QTemporaryDir dir;
        task3::MonitoringCore core(dir.filePath("events.sqlite"), 200);
        const auto topic = QString("Task3/Test/Telemetry/%1").arg(QCoreApplication::applicationPid());
        QVERIFY(core.startTelemetry(97, topic));
        task3::DdsTransport writer(true, 97, topic);
        QVERIFY2(writer.valid(), qPrintable(writer.error()));
        task3::Collectors c({{"host_id", "dds-local-test"}, {"applications", QJsonArray{}}});
        QTimer publisher;
        connect(&publisher, &QTimer::timeout, this, [&] { QVERIFY(writer.send(task3::encode(c.collect()))); });
        publisher.start(100);
        QTRY_COMPARE_WITH_TIMEOUT(core.hosts().size(), 1, 12000);
        QVERIFY(core.hosts()[0].toMap()["online"].toBool());
        publisher.stop();
        QTRY_VERIFY_WITH_TIMEOUT(!core.hosts()[0].toMap()["online"].toBool(), 3000);
        publisher.start(100);
        QTRY_VERIFY_WITH_TIMEOUT(core.hosts()[0].toMap()["online"].toBool(), 3000);
        publisher.stop();
        bool recovered = false;
        for (const auto& event : core.events())
            recovered |= event.toMap()["kind"] == "agent_timeout" && event.toMap()["state"] == "Recovered";
        QVERIFY(recovered);
    }
    void activeProcessAndServiceAlertsAreUnverifiedWhileAgentOffline() {
        QTemporaryDir dir; task3::MonitoringCore core(dir.filePath("events.sqlite"), 120);
        auto s = sample("offline-alert-host");
        QJsonObject app{{"application_id", "app"}, {"pid", 0}, {"state", "stopped"},
            {"service_state", "failed"}, {"service", "example.service"}};
        s["applications"] = QJsonArray{app};
        QVERIFY(core.ingest(task3::encode(s), "test"));
        auto alert = [&](const QString& kind) {
            for (const auto& value : core.events())
                if (value.toMap()["kind"] == kind && value.toMap()["state"] == "Active") return value.toMap();
            return QVariantMap{};
        };
        QCOMPARE(alert("process_stopped")["verification"].toString(), QString("Verified by current Agent telemetry"));
        QCOMPARE(alert("service_stopped")["verification"].toString(), QString("Verified by current Agent telemetry"));
        QTest::qWait(160); core.tick();
        for (const auto& kind : {QString("process_stopped"), QString("service_stopped")}) {
            const auto row = alert(kind);
            QCOMPARE(row["state"].toString(), QString("Active"));
            QCOMPARE(row["verification"].toString(), QString("Not verified — Agent Offline"));
            QVERIFY(row["recovered_ms"].isNull());
        }
        app["pid"] = 1; app["state"] = "running"; app["instance_id"] = "test-run";
        app["service_state"] = "running"; s["applications"] = QJsonArray{app};
        s["sequence"] = "2"; s["timestamp_ms"] = double(QDateTime::currentMSecsSinceEpoch());
        QVERIFY(core.ingest(task3::encode(s), "test-recovery"));
        for (const auto& kind : {QString("process_stopped"), QString("service_stopped")}) {
            bool recovered = false;
            for (const auto& value : core.events()) {
                const auto row = value.toMap();
                recovered |= row["kind"] == kind && row["state"] == "Recovered" && !row["recovered_ms"].isNull();
            }
            QVERIFY(recovered);
        }
    }
    void realCollectors() {
        task3::Collectors c({{"host_id", "local"}, {"applications", QJsonArray{}}});
        auto first = c.collect();
        QString error; QVERIFY2(task3::validate(first, error), qPrintable(error));
        QVERIFY(first["metrics"].toObject()["ram_total"].toDouble() > 0);
        QTest::qWait(20);
        auto second = c.collect();
        QVERIFY(second["metrics"].toObject()["cpu_percent"].isDouble());
        QVERIFY(second["metrics"].toObject()["disk_total"].toDouble() > 0);
    }
    void protocolSchemaAndSizeValidation() {
        auto s = sample(); QJsonObject out; QString e;
        auto packet = task3::encode(s);
        QVERIFY(task3::decode(packet, out, e)); QCOMPARE(out, s);
        QVERIFY(!task3::decode("{invalid", out, e));
        QVERIFY(!task3::decode(QByteArray(task3::MaxPacket+1, 'x'), out, e));
        s["schema_version"] = 99;
        QVERIFY(!task3::decode(task3::encode(s), out, e));
    }
    void registryOrderingAndRestart() {
        task3::Registry registry; auto s = sample(); QString e;
        QVERIFY(registry.ingest(s, "local", 0, 1000, e));
        QVERIFY(!registry.ingest(s, "local", 1, 1001, e));
        s["sequence"] = "3"; QVERIFY(registry.ingest(s, "local", 2, 1002, e));
        s["sequence"] = "2"; QVERIFY(!registry.ingest(s, "local", 3, 1003, e));
        registry.expire(6000, 5000); QVERIFY(!registry.rows()[0].toMap()["online"].toBool());
        s["sequence"] = "4"; QVERIFY(registry.ingest(s, "local", 6001, 1, e)); // wall clock backward
        QVERIFY(registry.rows()[0].toMap()["online"].toBool());
        auto restart = s; restart["session_id"] = "new-session"; restart["sequence"] = "1";
        restart["timestamp_ms"] = s["timestamp_ms"].toDouble() + 1;
        QVERIFY(registry.ingest(restart, "local", 7000, 2, e));
        s["sequence"] = "999";
        QVERIFY(!registry.ingest(s, "local", 8000, 3, e));
    }
    void unknownMapping() {
        task3::Registry registry;
        QCOMPARE(registry.mapParticipant("missing", "44")["mapping_state"].toString(), QString("unknown"));
    }
    void explicitPerRunMapping() {
        task3::Registry registry; auto s = sample(); QString e;
        const QJsonObject application{{"application_id", "app-a"}, {"pid", 123}, {"state", "running"},
            {"instance_id", "boot:12345"}, {"service_state", "not_configured"},
            {"participant_guids", QJsonArray{"guid-a", "guid-b"}}};
        s["applications"] = QJsonArray{application};
        QVERIFY(registry.ingest(s, "local", 0, 0, e));
        const auto host = s["hostname"].toString();
        QCOMPARE(registry.mapParticipant(host, "123", "guid-a")["application_id"].toString(), QString("app-a"));
        QCOMPARE(registry.mapParticipant(host, "123", "guid-b")["application_id"].toString(), QString("app-a"));
        QCOMPARE(registry.mapParticipant(host, "123", "guid-old")["mapping_state"].toString(), QString("unknown"));
        registry.expire(6000, 5000);
        QCOMPARE(registry.mapParticipant(host, "123", "guid-a")["mapping_state"].toString(), QString("unknown"));
        auto restarted = application;
        restarted["pid"] = 456; restarted["instance_id"] = "boot:67890";
        restarted["participant_guids"] = QJsonArray{"guid-new"};
        s["sequence"] = "2"; s["applications"] = QJsonArray{restarted};
        QVERIFY(registry.ingest(s, "local", 7000, 7000, e));
        QCOMPARE(registry.mapParticipant(host, "123", "guid-a")["mapping_state"].toString(), QString("unknown"));
        QCOMPARE(registry.mapParticipant(host, "456", "guid-new")["instance_id"].toString(), QString("boot:67890"));
    }
    void multipleHosts() {
        task3::Registry registry; QString e;
        for (int i = 0; i < 80; ++i) QVERIFY(registry.ingest(sample("host-" + QString::number(i)), "local", 0, 0, e));
        QCOMPARE(registry.rows().size(), 80);
    }
    void alertTransitionsAndPersistence() {
        QTemporaryDir dir; const auto path = dir.filePath("events.sqlite");
        {
            task3::AlertStore store(path);
            QVERIFY(store.error().isEmpty());
            store.condition("h/agent", true, "agent_timeout", "h", "timeout", 1000);
            store.condition("h/agent", true, "agent_timeout", "h", "timeout", 1001);
            QCOMPARE(store.rows().size(), 1);
        }
        {
            task3::AlertStore store(path);
            store.condition("h/agent", true, "agent_timeout", "h", "timeout", 1002);
            QCOMPARE(store.rows().size(), 1);
            store.condition("h/agent", false, "agent_timeout", "h", "timeout", 1003);
            QCOMPARE(store.rows().size(), 1);
            QCOMPARE(store.rows()[0].toMap()["state"].toString(), QString("Recovered"));
            QVERIFY(!store.rows()[0].toMap()["recovered_ms"].isNull());
            QVERIFY(store.rows()[0].toMap()["triggered_ms"].toLongLong() > 0);
        }
    }
    void processStopped() {
        task3::Collectors c({{"host_id", "local"}, {"applications", QJsonArray{
            QJsonObject{{"application_id", "missing"}, {"process_name", "task3-not-running-xyz"}}}}});
        const auto s = c.collect();
        QCOMPARE(s["applications"].toArray()[0].toObject()["state"].toString(), QString("stopped"));
    }
};
QTEST_GUILESS_MAIN(CoreTests)
#include "CoreTests.moc"
