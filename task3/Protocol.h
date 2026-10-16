// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QString>
namespace task3 {
constexpr int MaxPacket = 48000;
QByteArray encode(const QJsonObject& snapshot, const QByteArray& key);
bool decode(const QByteArray& packet, const QByteArray& key, QJsonObject& snapshot, QString& error);
bool validate(const QJsonObject& snapshot, QString& error);
}
