// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QJsonObject>
#include <QStringList>
namespace task3 {
QString processInstance(qint64 pid);
// Opt-in instrumentation; directory must be restricted to trusted local applications.
bool writeRuntimeIdentity(const QString& directory, const QString& applicationId,
                          const QStringList& participantGuids, const QJsonObject& writerCounts = {});
}
