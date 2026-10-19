// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QStringList>
#include <QMap>
#include <QFuture>
#include <functional>
namespace task3 {
class Collectors {
  public:
    using TimeStatusProvider = std::function<QJsonObject()>;
    explicit Collectors(QJsonObject config,
                        TimeStatusProvider timeStatusProvider = {},
                        qint64 timeRefreshMs = 10000);
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
    TimeStatusProvider timeStatusProvider_;
    QFuture<QJsonObject> timeStatusFuture_;
    qint64 timeCheckedMs_ = -1, timeRefreshMs_ = 10000;
    bool timeStatusInFlight_ = false;
    QMap<QString, QPair<quint64, quint64>> previousInterfaces_;
    void startTimeStatusQuery();
    QJsonArray applications();
};
}
