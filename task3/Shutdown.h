// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QCoreApplication>
#include <QTimer>
#include <csignal>
namespace task3 {
inline volatile std::sig_atomic_t stopRequested = 0;
inline void requestStop(int) {
    stopRequested = 1;
}
inline void installShutdownHandler() {
    std::signal(SIGINT, requestStop);
    std::signal(SIGTERM, requestStop);
    auto* timer = new QTimer(QCoreApplication::instance());
    QObject::connect(timer, &QTimer::timeout, [] {
        if (stopRequested)
            QCoreApplication::quit();
    });
    timer->start(50);
}
}
