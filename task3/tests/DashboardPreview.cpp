// SPDX-License-Identifier: GPL-3.0-or-later
// Renders the Task 3 dashboard (Qt Quick or Qt WebEngine page) from a recorded MonitoringCore
// snapshot, so both implementations can be compared pixel-for-pixel without DDS or the full app.
#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>
#include "WebFeed.h"

namespace {
class FixtureCore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList hosts MEMBER hosts NOTIFY changed)
    Q_PROPERTY(QVariantList events MEMBER events NOTIFY changed)
    Q_PROPERTY(QVariantList entities MEMBER entities NOTIFY changed)
    Q_PROPERTY(QVariantList flows MEMBER flows NOTIFY changed)
    Q_PROPERTY(QString status MEMBER status NOTIFY changed)
    Q_PROPERTY(bool hasMoreEvents MEMBER hasMoreEvents NOTIFY changed)

  public:
    QVariantList hosts, events, entities, flows;
    QString status;
    bool hasMoreEvents = false;
    Q_INVOKABLE void loadOlderEvents() {
        printf("loadOlderEvents\n");
    }
  public slots:
    void opened(const QString& entityId) {
        printf("openDdsWorkspace \"%s\"\n", qPrintable(entityId));
    }
  signals:
    void changed();
};
}

int main(int argc, char** argv) {
    QtWebEngineQuick::initialize();
    QApplication app(argc, argv);
    QCommandLineParser cli;
    cli.addOptions({{"page", "web or qml", "page", "web"},
                    {"fixture", "MonitoringCore snapshot JSON (hosts/events/entities/flows)", "path"},
                    {"output", "PNG written after rendering", "path"},
                    {"imports", "Repository root (Theme and eProsimaBrand style)", "path", "."},
                    {"width", "Window width", "px", "1600"},
                    {"height", "Window height", "px", "806"},
                    {"host", "Selected host_id", "id"},
                    {"app", "Selected application_id", "id"},
                    {"tab", "Host detail tab 0-3", "index", "0"},
                    {"resource-tab", "Resource tab 0-1", "index", "0"},
                    {"flow", "Select the first flow"},
                    {"events", "Show the events page"},
                    {"wait", "Render delay", "ms", "2500"},
                    {"click", "CSS selector clicked in the web page (repeatable, 700 ms apart)", "selector"}});
    cli.process(app);

    FixtureCore core;
    QFile file(cli.value("fixture"));
    if (!file.open(QIODevice::ReadOnly))
        qFatal("Cannot open fixture %s", qPrintable(cli.value("fixture")));
    const auto snapshot = QJsonDocument::fromJson(file.readAll()).object();
    core.hosts = snapshot["hosts"].toArray().toVariantList();
    core.events = snapshot["events"].toArray().toVariantList();
    core.entities = snapshot["entities"].toArray().toVariantList();
    core.flows = snapshot["flows"].toArray().toVariantList();
    core.status = snapshot["status"].toString();
    // Same trimmed, change-only feed as the Master; declared before the engine so it outlives the page.
    task3::WebFeed feed(&core);

    QQuickStyle::setStyle("eProsimaBrand");
    QQuickStyle::setFallbackStyle("Universal");
    qputenv("QT_QUICK_CONTROLS_UNIVERSAL_ACCENT", "#0895cd");
    QQmlApplicationEngine engine;
    const auto root = cli.value("imports");
    engine.addImportPath(root + "/imports");
    engine.addImportPath(root + "/qml/QtQuick/Controls");
    engine.rootContext()->setContextProperty("task3Core", &core);
    engine.rootContext()->setContextProperty("task3WebFeed", &feed);
    engine.rootContext()->setContextProperty("task3StartupTab", cli.value("tab").toInt());
    const auto page = cli.value("page") == "qml" ? "qrc:/task3/Dashboard.qml" : "qrc:/task3/WebDashboard.qml";
    engine.loadData(QStringLiteral("import QtQuick\nimport QtQuick.Controls\nApplicationWindow {\n"
                                   "  width: %1; height: %2; visible: true; color: \"#f5f5f5\"\n"
                                   "  property alias page: loader.item\n"
                                   "  Loader { id: loader; anchors.fill: parent; source: \"%3\" }\n}\n")
                        .arg(cli.value("width"), cli.value("height"), page)
                        .toUtf8());
    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().value(0));
    if (!window)
        return 2;
    auto* item = window->property("page").value<QObject*>();
    if (!item)
        return 3;
    QObject::connect(item, SIGNAL(openDdsWorkspace(QString)), &core, SLOT(opened(QString)));
    if (cli.isSet("host"))
        QMetaObject::invokeMethod(item, "chooseHost", Q_ARG(QVariant, cli.value("host")));
    if (cli.isSet("app"))
        QMetaObject::invokeMethod(item, "chooseApplication", Q_ARG(QVariant, cli.value("host")),
                                  Q_ARG(QVariant, cli.value("app")));
    item->setProperty("detailTab", cli.value("tab").toInt());
    item->setProperty("resourceTab", cli.value("resource-tab").toInt());
    item->setProperty("eventsPage", cli.isSet("events"));
    if (cli.isSet("flow") && !core.flows.isEmpty()) {
        const auto flow = core.flows.first().toMap();
        item->setProperty("selectedFlowKey",
                          flow["writer_id"].toString() + "/" + flow["reader_id"].toString());
    }
    // Live updates: re-publish the same data like MonitoringCore does every telemetry tick.
    QTimer tick;
    QObject::connect(&tick, &QTimer::timeout, &core, &FixtureCore::changed);
    tick.start(1000);
    // Interaction check: click web elements and report the selection state seen by QML.
    const auto clicks = cli.values("click");
    auto* view = window->findChild<QObject*>("task3WebView");
    for (int i = 0; i < clicks.size(); ++i)
        QTimer::singleShot(cli.value("wait").toInt() + 700 * i, &app, [view, selector = clicks[i]] {
            if (view)
                QMetaObject::invokeMethod(view, "runJavaScript",
                                          Q_ARG(QString, QStringLiteral("document.querySelector(%1).click()")
                                                             .arg(QString::fromUtf8(QJsonDocument(QJsonArray{selector}).toJson(QJsonDocument::Compact)).mid(1).chopped(1))));
        });
    QTimer::singleShot(cli.value("wait").toInt() + 700 * clicks.size(), &app, [&] {
        QJsonObject result;
        for (const auto* name : {"selectedHostId", "selectedApplicationId", "selectedFlowKey", "detailTab", "resourceTab", "eventsPage"})
            result[name] = QJsonValue::fromVariant(item->property(name));
        if (auto* network = window->findChild<QObject*>("hostNetworkMetrics"))
            result["network_view"] = QJsonObject{{"width", network->property("width").toDouble()},
                                                 {"height", network->property("height").toDouble()},
                                                 {"count", network->property("count").toInt()},
                                                 {"content_height", network->property("contentHeight").toDouble()}};
        printf("%s\n", QJsonDocument(result).toJson(QJsonDocument::Compact).constData());
        const auto image = window->grabWindow();
        const bool ok = !cli.isSet("output") || image.save(cli.value("output"));
        app.exit(ok ? 0 : 4);
    });
    return app.exec();
}
#include "DashboardPreview.moc"
