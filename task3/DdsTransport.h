// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QByteArray>
#include <QString>
#include <memory>
namespace task3 {
class DdsTransport : public QObject {
    Q_OBJECT
  public:
    DdsTransport(bool writer,
                 int domain,
                 const QString& topic,
                 QObject* parent = nullptr,
                 bool statistics = false);
    ~DdsTransport() override;
    bool valid() const;
    bool send(const QByteArray&);
    QString error() const;
    QString participantGuid() const;
    QString writerGuid() const;
    quint64 droppedPackets() const;
  signals:
    void received(QByteArray payload, QString source);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
