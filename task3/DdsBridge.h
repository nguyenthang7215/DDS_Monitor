// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QFutureWatcher>
#include <QTimer>
#include <QVariantList>
namespace task3 {
class DdsBridge : public QObject {
    Q_OBJECT
  public:
    explicit DdsBridge(QObject* parent = nullptr);
    ~DdsBridge() override;
    void poll();
    void stop();
    static QVariantList snapshot();
  signals:
    void updated(QVariantList);

  private:
    QTimer timer_;
    QFutureWatcher<QVariantList> watcher_;
};
}
