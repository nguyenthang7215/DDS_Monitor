// SPDX-License-Identifier: GPL-3.0-or-later
#include "Collectors.h"
#include "Protocol.h"
#include "Shutdown.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include "DdsTransport.h"
#include <QDebug>
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv); QCoreApplication::setApplicationName("task3_agent");
    task3::installShutdownHandler();
    QCommandLineParser cli; cli.addHelpOption();
    cli.addOption({"config", "Agent JSON configuration", "path"});
    cli.addOption({"once", "Collect once and print authenticated envelope to stdout"});
    cli.process(app);
    QFile file(cli.value("config"));
    if (!file.open(QIODevice::ReadOnly)) { qCritical() << "Cannot open config" << file.errorString(); return 2; }
    QJsonParseError parse; const auto doc = QJsonDocument::fromJson(file.readAll(), &parse);
    if (parse.error != QJsonParseError::NoError || !doc.isObject()) { qCritical() << "Invalid configuration JSON"; return 2; }
    const auto config = doc.object(); const QByteArray key = qgetenv("TASK3_KEY");
    const int domain = config["telemetry_domain"].toInt(42), interval = config["interval_ms"].toInt(1000);
    const auto topic = config["telemetry_topic"].toString("Task3/HostTelemetry/v1");
    if (key.size() < 16 || domain < 0 || domain > 232 || interval < 100 || interval > 60000 ||
        config["applications"].toArray().size() > 128) { qCritical() << "Invalid key/domain/interval/application limit"; return 2; }
    task3::Collectors collectors(config);
    std::unique_ptr<task3::DdsTransport> transport;
    if (!cli.isSet("once")) {
        transport.reset(new task3::DdsTransport(true, domain, topic));
        if (!transport->valid()) { qCritical() << transport->error(); return 2; }
    }
    int failures = 0;
    auto send = [&] {
        const auto snapshot = collectors.collect(); QString error;
        if (!task3::validate(snapshot, error)) { qCritical() << "Invalid sample" << error; ++failures; return; }
        const auto packet = task3::encode(snapshot, key);
        if (packet.size() > task3::MaxPacket) { qCritical() << "Telemetry exceeds packet limit"; ++failures; return; }
        if (cli.isSet("once")) { fprintf(stdout, "%s\n", packet.constData()); return; }
        if (!transport->send(packet)) {
            qWarning() << "DDS telemetry write failed"; ++failures;
        }
    };
    send();
    if (cli.isSet("once")) return failures ? 1 : 0;
    QTimer timer; QObject::connect(&timer, &QTimer::timeout, &app, send); timer.start(interval);
    qInfo() << "Real collectors active; DDS domain" << domain << "topic" << topic;
    return app.exec();
}
