// SPDX-License-Identifier: GPL-3.0-or-later
#include "Dashboard.h"
#include "MonitoringCore.h"
#include "DdsBridge.h"
#include <dds_monitor/Engine.h>
#include <QQmlContext>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSaveFile>
#include <QCoreApplication>
#include <QQuickWindow>
namespace task3 {
void installDashboard(Engine& engine) {
    const auto dataDir = qEnvironmentVariable("TASK3_DATA_DIR",
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/task3");
    if (!QDir().mkpath(dataDir)) qWarning() << "Cannot create event directory:" << dataDir;
    auto* core = new MonitoringCore(dataDir + "/events.sqlite", qgetenv("TASK3_KEY"),
        qEnvironmentVariableIntValue("TASK3_TIMEOUT_MS") > 0 ? qEnvironmentVariableIntValue("TASK3_TIMEOUT_MS") : 5000, &engine);
    bool domainOk = false;
    const auto telemetryDomain = qEnvironmentVariable("TASK3_TELEMETRY_DOMAIN", "42").toInt(&domainOk);
    if (!domainOk || !core->startTelemetry(telemetryDomain, qEnvironmentVariable("TASK3_TELEMETRY_TOPIC", "Task3/HostTelemetry/v1")))
        qWarning() << "DDS telemetry startup:" << core->status();
    auto* bridge = new DdsBridge(&engine);
    QObject::connect(bridge, &DdsBridge::updated, core, &MonitoringCore::updateDds);
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, core,
        [core, bridge] { bridge->stop(); core->stop(); });
    engine.rootContext()->setContextProperty("task3Core", core);
    engine.rootContext()->setContextProperty("task3StartupTab", qBound(0, qEnvironmentVariableIntValue("TASK3_UI_TAB"), 3));
    for (auto* root : engine.rootObjects()) {
        root->setProperty("task3Ready", true);
        root->setProperty("title", "Task 3 · DDS Monitor");
    }
    if (qEnvironmentVariableIsSet("TASK3_UI_EXERCISE")) {
        auto* exercise = new QTimer(&engine);
        QObject::connect(exercise, &QTimer::timeout, &engine, [&engine, step = 0]() mutable {
            for (auto* root : engine.rootObjects()) QMetaObject::invokeMethod(root, "task3Exercise", Q_ARG(QVariant, ++step));
        }); exercise->start(750);
    }
    const auto snapshotPath = qEnvironmentVariable("TASK3_SNAPSHOT_PATH");
    if (!snapshotPath.isEmpty()) {
        auto* exportTimer = new QTimer(&engine);
        QObject::connect(exportTimer, &QTimer::timeout, core, [core, snapshotPath, &engine] {
            int windows = 0, steps = 0;
            for (auto* root : engine.rootObjects()) {
                if (qobject_cast<QQuickWindow*>(root)) ++windows;
                steps = root->property("task3UiSteps").toInt();
            }
            const QJsonObject snapshot{{"master_pid", double(QCoreApplication::applicationPid())}, {"window_count", windows}, {"ui_steps", steps},
                {"hosts", QJsonArray::fromVariantList(core->hosts())},
                {"entities", QJsonArray::fromVariantList(core->entities())},
                {"flows", QJsonArray::fromVariantList(core->flows())},
                {"events", QJsonArray::fromVariantList(core->events())}, {"status", core->status()}};
            QSaveFile file(snapshotPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(QJsonDocument(snapshot).toJson()); file.commit();
            } else qWarning() << "Snapshot export:" << file.errorString();
            const auto png = qEnvironmentVariable("TASK3_SCREENSHOT_PATH");
            if (!png.isEmpty() && !core->hosts().isEmpty()) {
                for (auto* root : engine.rootObjects()) {
                    auto* window = qobject_cast<QQuickWindow*>(root);
                    if (window && window->title().startsWith("Task 3")) {
                        const auto capture = window->grabWindow(); capture.save(png);
                        if (qEnvironmentVariableIsSet("TASK3_UI_EXERCISE"))
                            capture.save(png + ".page-" + QString::number(root->property("task3Page").toBool() ? steps % 4 : 4) + ".png");
                    }
                }
            }
        });
        exportTimer->start(1000);
    }
    const int exitAfter = qEnvironmentVariableIntValue("TASK3_EXIT_AFTER_MS");
    if (exitAfter > 0) QTimer::singleShot(exitAfter, QCoreApplication::instance(), &QCoreApplication::quit);
    // Original DDS engine owns monitor creation and callbacks; bridge reads the same backend database.
    if (qEnvironmentVariable("TASK3_AUTO_DOMAIN", "0") != "off") {
        bool ok = false; const auto domain = qEnvironmentVariable("TASK3_AUTO_DOMAIN", "0").toInt(&ok);
        if (ok && domain >= 0 && domain <= 232)
            QTimer::singleShot(0, &engine, [&engine, domain] { engine.init_monitor(domain); });
    }
}
}
