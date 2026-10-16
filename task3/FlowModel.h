// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QVariantList>
namespace task3 {
QString matchingState(const QVariantMap& endpoint, qint64 now);
QVariantList topology(const QVariantList& entities, qint64 now);
}
