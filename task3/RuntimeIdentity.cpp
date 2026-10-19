// SPDX-License-Identifier: GPL-3.0-or-later
#include "RuntimeIdentity.h"
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QRegularExpression>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace task3 {
QString processInstance(qint64 pid) {
#ifdef Q_OS_WIN
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, DWORD(pid));
    FILETIME created{}, exited{}, kernel{}, user{};
    QString result;
    if (process && GetProcessTimes(process, &created, &exited, &kernel, &user))
        result = QString::number((quint64(created.dwHighDateTime) << 32) | created.dwLowDateTime);
    if (process)
        CloseHandle(process);
    return result;
#else
    QFile stat(QString("/proc/%1/stat").arg(pid));
    if (!stat.open(QIODevice::ReadOnly))
        return {};
    const auto bytes = stat.readAll();
    const auto end = bytes.lastIndexOf(')');
    if (end < 0)
        return {};
    const auto fields = bytes.mid(end + 2).simplified().split(' ');
    if (fields.size() < 20 || fields[0] == "Z")
        return {};
    QFile boot("/proc/sys/kernel/random/boot_id");
    if (!boot.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(boot.readAll()).trimmed() + ":" + fields[19];
#endif
}
bool writeRuntimeIdentity(const QString& directory,
                          const QString& app,
                          const QStringList& guids,
                          const QJsonObject& counts) {
    static const QRegularExpression id("^[A-Za-z0-9_.:-]{1,128}$");
    if (!id.match(app).hasMatch() || guids.size() > 32 || directory.isEmpty())
        return false;
    const auto pid = QCoreApplication::applicationPid();
    const auto instance = processInstance(pid);
    if (instance.isEmpty() || !QDir().mkpath(directory))
        return false;
    const QJsonObject manifest{{"application_id", app},
                               {"pid", double(pid)},
                               {"instance_id", instance},
                               {"participant_guids", QJsonArray::fromStringList(guids)},
                               {"writer_counts", counts},
                               {"checked_ms", double(QDateTime::currentMSecsSinceEpoch())}};
    QSaveFile file(QDir(directory).filePath(app + "-" + QString::number(pid) + ".json"));
    if (!file.open(QIODevice::WriteOnly))
        return false;
    const auto bytes = QJsonDocument(manifest).toJson(QJsonDocument::Compact);
    if (file.write(bytes) != bytes.size())
        return false;
    return file.commit();
}
}
