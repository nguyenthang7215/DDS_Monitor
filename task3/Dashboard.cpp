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
    auto* core = new MonitoringCore(dataDir + "/events.sqlite",
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
        root->setProperty("title", "DDS Monitor");
        root->setProperty("task3ResizeExercise", qEnvironmentVariable("TASK3_UI_EXERCISE") != "desktop");
        // Test-only viewport controls; do not change a normal user's window settings.
        if (qEnvironmentVariableIsSet("TASK3_UI_EXERCISE")) {
            const int width = qEnvironmentVariableIntValue("TASK3_UI_WIDTH");
            const int height = qEnvironmentVariableIntValue("TASK3_UI_HEIGHT");
            if (width >= 900 && height >= 650) {
                root->setProperty("width", width);
                root->setProperty("height", height);
            }
        }
    }
    if (qEnvironmentVariableIsSet("TASK3_UI_EXERCISE")) {
        auto* exercise = new QTimer(&engine);
        QObject::connect(exercise, &QTimer::timeout, &engine, [&engine, step = 0]() mutable {
            for (auto* root : engine.rootObjects()) QMetaObject::invokeMethod(root, "task3Exercise", Q_ARG(QVariant, ++step));
        }); exercise->start(qEnvironmentVariable("TASK3_UI_EXERCISE") == "desktop" ? 1400 : 750);
    }
    const auto snapshotPath = qEnvironmentVariable("TASK3_SNAPSHOT_PATH");
    if (!snapshotPath.isEmpty()) {
        auto* exportTimer = new QTimer(&engine);
        QObject::connect(exportTimer, &QTimer::timeout, core, [core, snapshotPath, &engine] {
            int windows = 0, steps = 0;
            QVariantMap ui;
            for (auto* root : engine.rootObjects()) {
                if (qobject_cast<QQuickWindow*>(root)) ++windows;
                steps = root->property("task3UiSteps").toInt();
                ui = {{"workspace", root->property("workspaceIndex")},
                    {"host_id", root->property("task3SelectedHost")},
                    {"application_id", root->property("task3SelectedApplication")},
                    {"detail_tab", root->property("task3DetailTab")},
                    {"resource_tab", root->property("task3ResourceTab")},
                    {"dashboard_loaded", root->property("task3DashboardLoaded")},
                    {"dds_started", root->property("legacyStarted")},
                    {"width", root->property("width")}, {"height", root->property("height")},
                    {"selection_retained", root->property("task3SelectionRetained")},
                    {"dds_workspace_count", root->findChildren<QObject*>("ddsOriginalWorkspace").size()},
                    {"title", root->property("title")}};
                if (auto* network = root->findChild<QObject*>("hostNetworkMetrics")) {
                    ui.insert("network_view", QVariantMap{{"width", network->property("width")},
                        {"height", network->property("height")}, {"count", network->property("count")},
                        {"content_height", network->property("contentHeight")}});
                }
            }
            const QJsonObject snapshot{{"master_pid", double(QCoreApplication::applicationPid())}, {"window_count", windows}, {"ui_steps", steps},
                {"hosts", QJsonArray::fromVariantList(core->hosts())},
                {"entities", QJsonArray::fromVariantList(core->entities())},
                {"flows", QJsonArray::fromVariantList(core->flows())},
                {"events", QJsonArray::fromVariantList(core->events())}, {"status", core->status()},
                {"ui", QJsonObject::fromVariantMap(ui)}};
            QSaveFile file(snapshotPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(QJsonDocument(snapshot).toJson()); file.commit();
            } else qWarning() << "Snapshot export:" << file.errorString();
            const auto png = qEnvironmentVariable("TASK3_SCREENSHOT_PATH");
            if (!png.isEmpty() && !core->hosts().isEmpty()) {
                for (auto* root : engine.rootObjects()) {
                    auto* window = qobject_cast<QQuickWindow*>(root);
                    if (window && window->objectName() == "ddsMonitorWindow") {
                        const auto capture = window->grabWindow();
                        // Readers must never copy a partially written screenshot.
                        const auto saveCapture = [&capture](const QString& target) {
                            QSaveFile imageFile(target);
                            if (imageFile.open(QIODevice::WriteOnly) && capture.save(&imageFile, "PNG"))
                                imageFile.commit();
                        };
                        saveCapture(png);
                        const int workspace = root->property("workspaceIndex").toInt();
                        const QString page = workspace == 1 ? "dds" : workspace == 2 ? "events" : "system";
                        saveCapture(png + "." + page + ".png");
                        if (qEnvironmentVariableIsSet("TASK3_UI_EXERCISE")) {
                            saveCapture(png + ".page-" + QString::number(workspace) + ".png");
                            if (workspace == 0)
                                saveCapture(png + ".system-" + QString::number(root->property("task3DetailTab").toInt()) + ".png");
                            if (workspace == 0 && root->property("task3DetailTab").toInt() == 0)
                                saveCapture(png + (root->property("task3ResourceTab").toInt() == 1 ? ".network.png" : ".resources.png"));
                        }
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
