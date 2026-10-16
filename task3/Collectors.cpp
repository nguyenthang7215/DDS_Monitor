// SPDX-License-Identifier: GPL-3.0-or-later
#include "Collectors.h"
#include "RuntimeIdentity.h"
#include <QJsonDocument>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QHostInfo>
#include <QJsonArray>
#include <QProcess>
#include <QStorageInfo>
#include <QSysInfo>
#include <QUuid>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QtConcurrent/QtConcurrentRun>
#include <cmath>
#include <algorithm>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#else
#include <unistd.h>
#include <signal.h>
#include <cerrno>
#endif
namespace task3 {
static QByteArray read(const QString& path) { QFile f(path); return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray(); }
static QJsonObject command(const QString& exe, const QStringList& args, int timeoutMs = 700) {
    QProcess p;
    auto environment = QProcessEnvironment::systemEnvironment(); environment.insert("LC_ALL", "C");
    p.setProcessEnvironment(environment); p.start(exe, args);
    if (!p.waitForStarted(300)) return {{"ok", false}, {"detail", p.errorString()}};
    if (!p.waitForFinished(timeoutMs)) { p.kill(); p.waitForFinished(); return {{"ok", false}, {"detail", "command timed out"}}; }
    return {{"ok", p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0},
            {"output", QString::fromUtf8(p.readAllStandardOutput()).trimmed()},
            {"detail", QString::fromUtf8(p.readAllStandardError()).trimmed()}};
}
static QJsonObject unknownTimeStatus(qint64 checkedMs, const QString& detail) {
    return {{"state", "unknown"}, {"source", ""}, {"service_running", QJsonValue(QJsonValue::Null)},
            {"clock_synced", QJsonValue(QJsonValue::Null)}, {"source_valid", QJsonValue(QJsonValue::Null)},
            {"source_reachable", QJsonValue(QJsonValue::Null)}, {"master_source_verified", QJsonValue(QJsonValue::Null)},
            {"offset_seconds", QJsonValue(QJsonValue::Null)}, {"system_time_seconds", QJsonValue(QJsonValue::Null)},
            {"checked_ms", checkedMs}, {"detail", detail}};
}
Collectors::Collectors(QJsonObject config, TimeStatusProvider timeStatusProvider, qint64 timeRefreshMs)
    : config_(std::move(config)), session_(QUuid::createUuid().toString(QUuid::WithoutBraces)),
      timeStatusProvider_(timeStatusProvider ? std::move(timeStatusProvider) : TimeStatusProvider(&Collectors::timeStatus)),
      timeRefreshMs_(qMax<qint64>(1000, timeRefreshMs)) {
    elapsed_.start();
    timeCache_ = unknownTimeStatus(QDateTime::currentMSecsSinceEpoch(), "Time status query pending");
    startTimeStatusQuery();
}
void Collectors::startTimeStatusQuery() {
    const auto provider = timeStatusProvider_;
    timeStatusFuture_ = QtConcurrent::run([provider] {
        try {
            const auto result = provider();
            const auto state = result.value("state").toString();
            if (state == "synced" || state == "unhealthy" || state == "unknown") return result;
            return unknownTimeStatus(QDateTime::currentMSecsSinceEpoch(), "Invalid time status result");
        } catch (...) {
            return unknownTimeStatus(QDateTime::currentMSecsSinceEpoch(), "Time status query failed");
        }
    });
    timeStatusInFlight_ = true;
    timeCheckedMs_ = elapsed_.elapsed();
}
QJsonObject Collectors::timeStatus() {
    const auto now = QDateTime::currentMSecsSinceEpoch();
#ifdef Q_OS_WIN
    const auto script = qEnvironmentVariable("TASK3_WINDOWS_TIME_SCRIPT",
        QCoreApplication::applicationDirPath() + "/windows-time-status.ps1");
    const auto result = command("powershell.exe", {"-NoProfile", "-NonInteractive", "-File", script}, 5000);
    const auto evidence = result["ok"].toBool() ?
        QJsonDocument::fromJson(result["output"].toString().toUtf8()).object() : QJsonObject{};
    auto status = parseWindowsTime(evidence, now);
    if (evidence.isEmpty()) status["detail"] = "Structured Windows Time evidence unavailable: " + result["detail"].toString();
    return status;
#else
    const auto result = command("chronyc", {"-c", "tracking"});
    if (!result["ok"].toBool())
        return {{"state", "unknown"}, {"source", ""}, {"offset_seconds", QJsonValue::Null},
                {"checked_ms", now}, {"detail", result["detail"].toString().isEmpty() ?
                 "chronyc unavailable or daemon not reachable" : result["detail"].toString()}};
    return parseChrony(result["output"].toString(), now);
#endif
}
QJsonObject Collectors::parseChrony(const QString& csv, qint64 now) {
    const auto fields = csv.split(',');
    bool systemTimeValid = false, lastOffsetValid = false;
    const double systemTime = fields.value(3).toDouble(&systemTimeValid);
    const double lastOffset = fields.value(4).toDouble(&lastOffsetValid);
    bool stratumValid = false; const int stratum = fields.value(1).toInt(&stratumValid);
    if (fields.size() < 13 || !systemTimeValid || !std::isfinite(systemTime) || !lastOffsetValid ||
        !std::isfinite(lastOffset) || !stratumValid || stratum < 0 || stratum > 16)
        return unknownTimeStatus(now, "unrecognized chronyc CSV");
    const bool sourceValid = stratum > 0 && stratum < 16 && !fields.value(0).trimmed().isEmpty() && fields.value(0) != "00000000";
    const bool synced = sourceValid && fields.last().trimmed() == "Normal";
    return {{"state", synced ? "synced" : "unhealthy"}, {"source", fields.value(0)},
            {"service_running", true}, {"source_reachable", QJsonValue::Null}, {"clock_synced", synced},
            {"source_valid", sourceValid}, {"master_source_verified", QJsonValue(QJsonValue::Null)}, {"stratum", stratum},
            {"offset_seconds", synced ? QJsonValue(lastOffset) : QJsonValue(QJsonValue::Null)},
            {"offset_kind", "chrony_last_offset"},
            {"system_time_seconds", synced ? QJsonValue(systemTime) : QJsonValue(QJsonValue::Null)},
            {"checked_ms", now}, {"detail", synced ? "Chrony synchronized to its selected source; Master identity not verified" : fields.last().trimmed()}};
}
QJsonObject Collectors::parseWindowsTime(const QJsonObject& evidence, qint64 now) {
    const auto service = evidence["service_running"];
    const auto count = evidence["time_sources"];
    const auto offset = evidence["offset_us"];
    const bool available = count.isDouble() && count.toDouble() > 0 && offset.isDouble() && std::isfinite(offset.toDouble());
    return {{"state", service.isBool() && !service.toBool() ? "unhealthy" : "unknown"},
        {"service_running", service.isBool() ? service : QJsonValue(QJsonValue::Null)},
        {"source", evidence["source"].toString()}, {"source_valid", QJsonValue::Null},
        {"source_reachable", QJsonValue::Null}, {"master_source_verified", QJsonValue::Null}, {"clock_synced", QJsonValue::Null},
        {"offset_seconds", available ? QJsonValue(offset.toDouble() / 1000000.0) : QJsonValue(QJsonValue::Null)},
        {"checked_ms", now}, {"offset_kind", "absolute_w32time_estimate"}, {"detail", "Structured service/source/performance evidence; clock synchronization not independently established"}};
}
QJsonArray Collectors::applications() {
    QJsonArray apps;
    for (const auto& value : config_["applications"].toArray()) {
        const auto c = value.toObject();
        const auto name = c["process_name"].toString();
        const auto service = c["service"].toString();
        QString state = name.isEmpty() ? "unknown" : "stopped", instance, serviceState = "not_configured";
        qint64 pid = 0;
        QJsonArray instances;
#ifdef Q_OS_WIN
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) state = "unknown";
        else {
            PROCESSENTRY32W e{}; e.dwSize = sizeof(e);
            if (Process32FirstW(snapshot, &e)) do {
                if (QString::fromWCharArray(e.szExeFile).compare(name, Qt::CaseInsensitive) != 0) continue;
                if (!c["argument_token"].toString().isEmpty()) { state = "unknown"; continue; }
                pid = e.th32ProcessID; state = "running";
                instance.clear();
                HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, DWORD(pid));
                FILETIME create{}, exit{}, kernel{}, user{};
                if (h && GetProcessTimes(h, &create, &exit, &kernel, &user))
                    instance = QString::number((quint64(create.dwHighDateTime) << 32) | create.dwLowDateTime);
                if (h) CloseHandle(h);
                if (!instance.isEmpty() && instances.size() < 128)
                    instances.append(QJsonObject{{"pid", double(pid)}, {"instance_id", instance}, {"state", "running"}});
            } while (Process32NextW(snapshot, &e));
            CloseHandle(snapshot);
        }
        if (!service.isEmpty()) {
            serviceState = "unknown";
            SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
            if (scm) {
                const auto wide = service.toStdWString();
                SC_HANDLE svc = OpenServiceW(scm, wide.c_str(), SERVICE_QUERY_STATUS);
                if (svc) {
                    SERVICE_STATUS_PROCESS status{}; DWORD bytes{};
                    if (QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&status), sizeof(status), &bytes))
                        serviceState = status.dwCurrentState == SERVICE_RUNNING ? "running" :
                            (status.dwCurrentState == SERVICE_STOPPED ? (status.dwWin32ExitCode ? "failed" : "stopped") : "unknown");
                    CloseServiceHandle(svc);
                }
                CloseServiceHandle(scm);
            }
        }
#else
        // Scan names only for explicitly configured applications; never publish the full process list.
        const auto entries = QDir("/proc").entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto& entry : entries) {
            if (name.isEmpty()) break;
            bool numeric = false; const auto candidate = entry.toLongLong(&numeric);
            if (!numeric) continue;
            const auto comm = QString::fromUtf8(read("/proc/" + entry + "/comm")).trimmed();
            // Linux comm is limited to 15 bytes; use the actual executable basename as well.
            if (comm != name && QFileInfo(QFileInfo("/proc/" + entry + "/exe").symLinkTarget()).fileName() != name) continue;
            const auto token = c["argument_token"].toString();
            if (!token.isEmpty()) {
                const auto args = read("/proc/" + entry + "/cmdline");
                if (args.isEmpty()) { state = "unknown"; continue; }
                if (!args.split('\0').contains(token.toUtf8())) continue;
            }
            const auto stat = read("/proc/" + entry + "/stat");
            const auto close = stat.lastIndexOf(')');
            const auto fields = stat.mid(close + 2).simplified().split(' ');
            if (fields.size() < 20) { state = "unknown"; continue; }
            if (fields[0] == "Z") continue;
            pid = candidate; state = "running";
            instance = QString::fromUtf8(read("/proc/sys/kernel/random/boot_id")).trimmed() + ":" + fields[19];
            if (instances.size() < 128)
                instances.append(QJsonObject{{"pid", double(pid)}, {"instance_id", instance}, {"state", "running"}});
        }
        if (!service.isEmpty()) {
            const auto result = command("systemctl", {"show", "--property=ActiveState", "--value", service});
            const auto output = result["output"].toString();
            serviceState = parseService(output, result["ok"].toBool());
        }
#endif
        const QDir identities(config_["identity_directory"].toString());
        if (!config_["identity_directory"].toString().isEmpty()) {
            const auto files = identities.entryList({"*.json"}, QDir::Files | QDir::NoSymLinks);
            for (const auto& filename : files.mid(0, 512)) {
                QFile file(identities.filePath(filename));
                if (!file.open(QIODevice::ReadOnly) || file.size() > 16384) continue;
                const auto m = QJsonDocument::fromJson(file.readAll()).object();
                if (m["application_id"] != c["application_id"]) continue;
                const auto age = QDateTime::currentMSecsSinceEpoch() - qint64(m["checked_ms"].toDouble());
                if (age < -2000 || age > 10000) continue;
                for (auto item : instances) {
                    auto process = item.toObject();
                    if (m["pid"] != process["pid"] || m["instance_id"] != process["instance_id"]) continue;
                    if (process.contains("binding_source")) process["binding_conflict"] = true;
                    else {
                        process["participant_guids"] = m["participant_guids"];
                        process["writer_counts"] = m["writer_counts"];
                        process["binding_source"] = "runtime_manifest";
                    }
                    item = process;
                }
            }
        }
        if (c["identity_required"].toBool()) {
            QJsonArray verified;
            for (const auto& item : instances) if (item.toObject()["binding_source"] == "runtime_manifest") verified.append(item);
            if (!instances.isEmpty() && verified.isEmpty()) state = "unknown";
            instances = verified;
        }
        // Preserve a real representative PID for older schema-1 consumers; instances[] is authoritative.
        if (!instances.isEmpty()) { pid = qint64(instances[0].toObject()["pid"].toDouble()); instance = instances[0].toObject()["instance_id"].toString(); }
        if (instances.size() > 1) state = "running";
        else if (state == "running" && instance.isEmpty()) state = "unknown";
        apps.append(QJsonObject{{"application_id", c["application_id"]}, {"process_name", name},
            {"service", service}, {"pid", double(pid)}, {"instance_id", instance},
            {"state", state}, {"service_state", serviceState}, {"participant_guids", c["participant_guids"].toArray()},
            {"instances", instances}, {"process_count", instances.size()},
            {"checked_ms", double(QDateTime::currentMSecsSinceEpoch())}});
    }
    return apps;
}
QString Collectors::parseService(const QString& output, bool success) {
    if (!success) return "unknown";
    const auto state = output.trimmed();
    if (state == "active" || state == "reloading") return "running";
    if (state == "inactive") return "stopped";
    if (state == "failed") return "failed";
    return "unknown";
}
QJsonObject Collectors::collect() {
    const qint64 nowMono = elapsed_.elapsed();
    if (timeStatusInFlight_ && timeStatusFuture_.isFinished()) {
        try { timeCache_ = timeStatusFuture_.result(); }
        catch (...) { timeCache_ = unknownTimeStatus(QDateTime::currentMSecsSinceEpoch(), "Time status query failed"); }
        timeStatusInFlight_ = false;
    }
    if (!timeStatusInFlight_ && nowMono - timeCheckedMs_ >= timeRefreshMs_) startTimeStatusQuery();
    QJsonObject metrics{{"ram_total", 0.0}, {"ram_available", 0.0}, {"ram_used", 0.0}};
    quint64 idle = 0, total = 0, rx = 0, tx = 0;
    QMap<QString, QPair<quint64, quint64>> interfaces;
#ifdef Q_OS_WIN
    FILETIME fi{}, fk{}, fu{};
    if (GetSystemTimes(&fi, &fk, &fu)) {
        auto n = [](FILETIME t) { return (quint64(t.dwHighDateTime) << 32) | t.dwLowDateTime; };
        idle = n(fi); total = n(fk) + n(fu);
    }
    MEMORYSTATUSEX mem{}; mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        metrics["ram_total"] = double(mem.ullTotalPhys); metrics["ram_available"] = double(mem.ullAvailPhys);
        metrics["ram_used"] = double(mem.ullTotalPhys - mem.ullAvailPhys);
    }
    PMIB_IF_TABLE2 table = nullptr;
    if (GetIfTable2(&table) == NO_ERROR) {
        for (ULONG i = 0; i < table->NumEntries; ++i) {
            if (table->Table[i].Type == IF_TYPE_SOFTWARE_LOOPBACK) continue;
            rx += table->Table[i].InOctets; tx += table->Table[i].OutOctets;
            interfaces[QString::number(table->Table[i].InterfaceIndex)] =
                qMakePair(quint64(table->Table[i].InOctets), quint64(table->Table[i].OutOctets));
        }
        FreeMibTable(table);
    }
#else
    const auto cpu = read("/proc/stat").split('\n').value(0).simplified().split(' ');
    for (int i = 1; i < qMin(9, cpu.size()); ++i) total += cpu[i].toULongLong();
    idle = cpu.value(4).toULongLong() + cpu.value(5).toULongLong();
    QMap<QByteArray, quint64> mem;
    for (const auto& line : read("/proc/meminfo").split('\n')) {
        const auto fields = line.simplified().split(' ');
        if (fields.size() > 1) mem[fields[0]] = fields[1].toULongLong() * 1024;
    }
    metrics["ram_total"] = double(mem["MemTotal:"]); metrics["ram_available"] = double(mem["MemAvailable:"]);
    metrics["ram_used"] = double(mem["MemTotal:"] - mem["MemAvailable:"]);
    for (const auto& line : read("/proc/net/dev").split('\n')) {
        const int colon = line.indexOf(':');
        if (colon < 0 || line.left(colon).trimmed() == "lo") continue;
        const auto counters = line.mid(colon + 1).simplified().split(' ');
        rx += counters.value(0).toULongLong(); tx += counters.value(8).toULongLong();
        interfaces[QString::fromUtf8(line.left(colon).trimmed())] =
            qMakePair(counters.value(0).toULongLong(), counters.value(8).toULongLong());
    }
#endif
    const bool validCpu = previousTotal_ && total > previousTotal_ && idle >= previousIdle_;
    metrics["cpu_percent"] = validCpu ? QJsonValue(qBound(0.0, 100.0 * (1.0 - double(idle - previousIdle_) / double(total - previousTotal_)), 100.0))
                                      : QJsonValue(QJsonValue::Null);
    previousIdle_ = idle; previousTotal_ = total;
    const QStorageInfo disk(config_["disk_path"].toString(QDir::rootPath()));
    metrics["disk_total"] = double(qMax<qint64>(0, disk.bytesTotal()));
    metrics["disk_free"] = double(qMax<qint64>(0, disk.bytesFree()));
    metrics["disk_used"] = double(qMax<qint64>(0, disk.bytesTotal() - disk.bytesFree()));
    metrics["disk_path"] = disk.rootPath();
    QJsonArray disks;
    auto diskPaths = config_["disk_paths"].toArray();
    if (diskPaths.isEmpty()) diskPaths.append(config_["disk_path"].toString(QDir::rootPath()));
    for (const auto& path : diskPaths) {
        QStorageInfo item(path.toString());
        disks.append(QJsonObject{{"path", path}, {"valid", item.isValid() && item.isReady()},
            {"total", double(qMax<qint64>(0, item.bytesTotal()))}, {"free", double(qMax<qint64>(0, item.bytesFree()))},
            {"used", double(qMax<qint64>(0, item.bytesTotal() - item.bytesFree()))}});
    }
    metrics["disks"] = disks;
    metrics["ram_percent"] = metrics["ram_total"].toDouble() > 0 ?
        QJsonValue(100.0 * metrics["ram_used"].toDouble() / metrics["ram_total"].toDouble()) : QJsonValue(QJsonValue::Null);
    metrics["network_rx_bytes"] = double(rx); metrics["network_tx_bytes"] = double(tx);
    const auto deltaMs = nowMono - previousMs_;
    QJsonArray networkInterfaces;
    double rxRate = 0, txRate = 0;
    for (auto it = interfaces.cbegin(); it != interfaces.cend(); ++it) {
        const auto prev = previousInterfaces_.value(it.key());
        const bool comparable = previousInterfaces_.contains(it.key()) && deltaMs > 0 &&
            it->first >= prev.first && it->second >= prev.second;
        const double receiveRate = comparable ? double(it->first - prev.first) * 1000.0 / deltaMs : 0;
        const double sendRate = comparable ? double(it->second - prev.second) * 1000.0 / deltaMs : 0;
        networkInterfaces.append(QJsonObject{{"name", it.key()}, {"rx_bytes", double(it->first)},
            {"tx_bytes", double(it->second)}, {"rx_bps", receiveRate}, {"tx_bps", sendRate}, {"rate_valid", comparable}});
        rxRate += receiveRate; txRate += sendRate;
    }
    previousInterfaces_ = interfaces;
    metrics["interfaces"] = networkInterfaces;
    metrics["network_rx_bps"] = rxRate;
    metrics["network_tx_bps"] = txRate;
    metrics["network_scope"] = "aggregate non-loopback interfaces";
    rx_ = rx; tx_ = tx; previousMs_ = nowMono;
    return {{"schema_version", 1}, {"type", "telemetry"}, {"host_id", config_["host_id"]},
        {"hostname", QHostInfo::localHostName()}, {"os", QSysInfo::prettyProductName()},
        {"dds_host_identifier", config_["dds_host_identifier"].toString(QString::fromLatin1(QSysInfo::machineUniqueId()))},
        {"session_id", session_}, {"sequence", QString::number(++sequence_)},
        {"timestamp_ms", double(QDateTime::currentMSecsSinceEpoch())}, {"metrics", metrics},
        {"applications", applications()}, {"time_sync", timeCache_},
        {"collector_health", QJsonObject{{"cpu", total > 0 ? "available" : "unavailable"},
            {"ram", metrics["ram_total"].toDouble() > 0 ? "available" : "unavailable"},
            {"disk", disk.isValid() && disk.isReady() ? "available" : "unavailable"},
            {"network", interfaces.isEmpty() ? "unavailable" : "available"}}}};
}
}
