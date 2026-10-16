// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QString>
#include <QVariantMap>
namespace task3 {
enum class ThroughputUnit { BytesPerSecond, MegabitsPerSecond };
inline double bytesPerSecond(double raw, ThroughputUnit unit) {
    return unit == ThroughputUnit::MegabitsPerSecond ? raw * 125000.0 : raw;
}
inline double microseconds(double rawNs) { return rawNs / 1000.0; }
inline QVariantMap throughputMeasurement(double raw) {
    // Verified for installed Fast DDS 3.6.1 + Backend 2.5.0 SOURCE, despite stale Mb/s header comment.
    return {{"raw_value", raw}, {"raw_unit", "B/s"}, {"normalized_value", raw},
            {"normalized_unit", "B/s"}, {"display_value", raw}, {"display_unit", "B/s"}};
}
}
