// SPDX-License-Identifier: GPL-3.0-or-later
// Owned test process demonstrating opt-in runtime identity; not a production business application.
#include "DdsTransport.h"
#include "RuntimeIdentity.h"
#include "Shutdown.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QTimer>
#include <QDebug>
#include <memory>
#include <vector>
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    task3::installShutdownHandler();
    QCommandLineParser cli; cli.addHelpOption();
    cli.addOptions({{"identity-dir", "Runtime identity directory", "path"}, {"app", "Application ID", "id", "probe"},
        {"mode", "writer or reader", "mode", "writer"}, {"domain", "Application Domain", "id", "0"},
        {"participants", "Participants per process", "count", "2"},
        {"silent-after-ms", "Stop writing but keep process and identity alive", "ms", "0"}});
    cli.process(app);
    const bool writer = cli.value("mode") == "writer";
    std::vector<std::unique_ptr<task3::DdsTransport>> transports;
    QStringList guids;
    for (int i = 0; i < qBound(1, cli.value("participants").toInt(), 4); ++i) {
        auto transport = std::make_unique<task3::DdsTransport>(writer, cli.value("domain").toInt(),
            "Task3/RuntimeProbe", nullptr, true);
        if (!transport->valid()) { qCritical() << transport->error(); return 2; }
        guids.append(transport->participantGuid()); transports.push_back(std::move(transport));
    }
    QElapsedTimer elapsed; elapsed.start(); quint64 count = 0;
    auto publish = [&] {
        QJsonObject counts;
        const int silent = cli.value("silent-after-ms").toInt();
        const bool writing = writer && (!silent || elapsed.elapsed() < silent);
        if (writing) ++count;
        for (const auto& transport : transports) {
            if (writing && !transport->send(QByteArray("real business fixture sample ") + QByteArray::number(count))) return;
            if (writer) counts[transport->writerGuid()] = QString::number(count);
        }
        if (!task3::writeRuntimeIdentity(cli.value("identity-dir"), cli.value("app"), guids, counts))
            qWarning() << "Cannot publish runtime identity";
    };
    publish(); QTimer timer; QObject::connect(&timer, &QTimer::timeout, &app, publish); timer.start(200);
    return app.exec();
}
