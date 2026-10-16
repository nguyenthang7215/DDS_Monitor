// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QStringList>
#include <QMap>
namespace task3 {
class Collectors {
public:
    explicit Collectors(QJsonObject config);
    QJsonObject collect();
    static QJsonObject timeStatus();
    static QJsonObject parseChrony(const QString& csv, qint64 checkedMs);
    static QString parseService(const QString& output, bool success);
    static QJsonObject parseWindowsTime(const QJsonObject& evidence, qint64 checkedMs);
private:
    QJsonObject config_;
    QString session_;
    quint64 sequence_ = 0, previousIdle_ = 0, previousTotal_ = 0, rx_ = 0, tx_ = 0;
    QElapsedTimer elapsed_;
    qint64 previousMs_ = 0;
    QJsonObject timeCache_;
    qint64 timeCheckedMs_ = -10000;
    QMap<QString, QPair<quint64, quint64>> previousInterfaces_;
    QJsonArray applications();
};
}
