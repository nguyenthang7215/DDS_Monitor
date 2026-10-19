// SPDX-License-Identifier: GPL-3.0-or-later
#include "DdsTransport.h"
#include "Protocol.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/topic/TopicDataType.hpp>
#include <fastcdr/Cdr.h>
#include <fastcdr/FastBuffer.h>
#include <atomic>
#include <QMutex>
#include <QMutexLocker>
#include <QQueue>
#include <sstream>
namespace task3 {
namespace dds = eprosima::fastdds::dds;
namespace rtps = eprosima::fastdds::rtps;
class TelemetryType final : public dds::TopicDataType {
  public:
    TelemetryType() {
        set_name("task3::TelemetryEnvelopeV1");
        max_serialized_type_size = MaxPacket + 12;
        is_compute_key_provided = false;
    }
    bool serialize(const void* const data,
                   rtps::SerializedPayload_t& payload,
                   dds::DataRepresentationId_t) override {
        const auto& bytes = *static_cast<const std::string*>(data);
        if (bytes.size() > MaxPacket)
            return false;
        try {
            eprosima::fastcdr::FastBuffer buffer(reinterpret_cast<char*>(payload.data),
                                                 payload.max_size);
            eprosima::fastcdr::Cdr cdr(buffer,
                                       eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
                                       eprosima::fastcdr::CdrVersion::XCDRv1);
            cdr.serialize_encapsulation();
            cdr << bytes;
            payload.encapsulation =
                cdr.endianness() == eprosima::fastcdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;
            payload.length = uint32_t(cdr.get_serialized_data_length());
            return true;
        } catch (const eprosima::fastcdr::exception::Exception&) {
            return false;
        }
    }
    bool deserialize(rtps::SerializedPayload_t& payload, void* data) override {
        if (payload.length > MaxPacket + 12)
            return false;
        try {
            eprosima::fastcdr::FastBuffer buffer(reinterpret_cast<char*>(payload.data),
                                                 payload.length);
            eprosima::fastcdr::Cdr cdr(buffer);
            cdr.read_encapsulation();
            cdr >> *static_cast<std::string*>(data);
            return static_cast<std::string*>(data)->size() <= MaxPacket;
        } catch (const eprosima::fastcdr::exception::Exception&) {
            return false;
        }
    }
    uint32_t calculate_serialized_size(const void* const data,
                                       dds::DataRepresentationId_t) override {
        return uint32_t(static_cast<const std::string*>(data)->size()) + 9;
    }
    bool compute_key(rtps::SerializedPayload_t&, rtps::InstanceHandle_t&, bool) override {
        return false;
    }
    bool compute_key(const void* const, rtps::InstanceHandle_t&, bool) override {
        return false;
    }
    void* create_data() override {
        return new std::string;
    }
    void delete_data(void* data) override {
        delete static_cast<std::string*>(data);
    }
};
struct DdsTransport::Impl : dds::DataReaderListener {
    DdsTransport* owner;
    dds::TypeSupport type{new TelemetryType};
    dds::DomainParticipant* participant = nullptr;
    dds::DataReader* reader = nullptr;
    dds::DataWriter* writer = nullptr;
    QString error;
    std::atomic<bool> closing{false};
    std::atomic<quint64> dropped{0};
    QMutex queueMutex;
    QQueue<QByteArray> pending;
    bool scheduled = false;
    explicit Impl(DdsTransport* o) : owner(o) {
    }
    void dispatch() {
        for (int i = 0; i < 32 && !closing; ++i) {
            QByteArray packet;
            {
                QMutexLocker lock(&queueMutex);
                if (pending.isEmpty()) {
                    scheduled = false;
                    return;
                }
                packet = pending.dequeue();
            }
            emit owner->received(packet, "DDS telemetry");
        }
        if (!closing)
            QMetaObject::invokeMethod(owner, [this] { dispatch(); }, Qt::QueuedConnection);
    }
    ~Impl() override {
        closing = true;
        if (participant) {
            if (reader)
                reader->set_listener(nullptr);
            participant->delete_contained_entities();
            dds::DomainParticipantFactory::get_instance()->delete_participant(participant);
        }
    }
    void on_data_available(dds::DataReader* r) override {
        std::string data;
        dds::SampleInfo info;
        int budget = 256;
        while (!closing && budget-- > 0 && r->take_next_sample(&data, &info) == dds::RETCODE_OK) {
            if (info.valid_data && data.size() <= MaxPacket) {
                QMutexLocker lock(&queueMutex);
                if (pending.size() >= 256) {
                    pending.dequeue();
                    ++dropped;
                }
                pending.enqueue(QByteArray::fromStdString(data));
                if (!scheduled) {
                    scheduled = true;
                    QMetaObject::invokeMethod(owner, [this] { dispatch(); }, Qt::QueuedConnection);
                }
            }
        }
    }
};
DdsTransport::DdsTransport(bool writer,
                           int domain,
                           const QString& name,
                           QObject* parent,
                           bool statistics)
    : QObject(parent), impl_(new Impl(this)) {
    if (domain < 0 || domain > 232 || name.isEmpty() || name.size() > 200) {
        impl_->error = "invalid DDS domain/topic";
        return;
    }
    auto* factory = dds::DomainParticipantFactory::get_instance();
    dds::DomainParticipantQos pq;
    pq.name(statistics ? (writer ? "Task3 Runtime Probe Writer" : "Task3 Runtime Probe Reader")
                       : (writer ? "Task3 Slave Agent" : "Task3 Master Telemetry"));
    // Keep telemetry separate from application statistics and avoid monitoring its own monitoring stream.
    pq.properties().properties().emplace_back(
        "fastdds.statistics",
        statistics
            ? "PHYSICAL_DATA_TOPIC;DISCOVERY_TOPIC;PUBLICATION_THROUGHPUT_TOPIC;SUBSCRIPTION_THROUGHPUT_TOPIC;HISTORY_LATENCY_TOPIC;MONITOR_SERVICE_TOPIC"
            : "");
    impl_->participant = factory->create_participant(domain, pq);
    if (!impl_->participant) {
        impl_->error = "DDS participant creation failed";
        return;
    }
    auto* p = impl_->participant;
    if (impl_->type.register_type(p) != dds::RETCODE_OK) {
        impl_->error = "type registration failed";
        return;
    }
    auto* topic =
        p->create_topic(name.toStdString(), impl_->type.get_type_name(), dds::TOPIC_QOS_DEFAULT);
    if (!topic) {
        impl_->error = "DDS topic creation failed";
        return;
    }
    if (writer) {
        auto* pub = p->create_publisher(dds::PUBLISHER_QOS_DEFAULT);
        dds::DataWriterQos qos;
        qos.reliability().kind = dds::RELIABLE_RELIABILITY_QOS;
        qos.reliability().max_blocking_time = {0, 100000000};
        qos.durability().kind = dds::VOLATILE_DURABILITY_QOS;
        qos.history().kind = dds::KEEP_LAST_HISTORY_QOS;
        qos.history().depth = 32;
        qos.resource_limits().max_samples = 64;
        qos.resource_limits().allocated_samples = 32;
        if (pub)
            impl_->writer = pub->create_datawriter(topic, qos);
        if (!impl_->writer)
            impl_->error = "DDS writer creation failed";
    } else {
        auto* sub = p->create_subscriber(dds::SUBSCRIBER_QOS_DEFAULT);
        dds::DataReaderQos qos;
        qos.reliability().kind = dds::RELIABLE_RELIABILITY_QOS;
        qos.durability().kind = dds::VOLATILE_DURABILITY_QOS;
        qos.history().kind = dds::KEEP_LAST_HISTORY_QOS;
        qos.history().depth = 256;
        qos.resource_limits().max_samples = 512;
        qos.resource_limits().allocated_samples = 32;
        if (sub)
            impl_->reader = sub->create_datareader(topic, qos, impl_.get());
        if (!impl_->reader)
            impl_->error = "DDS reader creation failed";
    }
}
DdsTransport::~DdsTransport() = default;
bool DdsTransport::valid() const {
    return impl_->writer || impl_->reader;
}
QString DdsTransport::error() const {
    return impl_->error;
}
QString DdsTransport::participantGuid() const {
    if (!impl_->participant)
        return {};
    std::ostringstream out;
    out << impl_->participant->guid();
    return QString::fromStdString(out.str());
}
QString DdsTransport::writerGuid() const {
    if (!impl_->writer)
        return {};
    std::ostringstream out;
    out << impl_->writer->guid();
    return QString::fromStdString(out.str());
}
quint64 DdsTransport::droppedPackets() const {
    return impl_->dropped.load();
}
bool DdsTransport::send(const QByteArray& packet) {
    if (!impl_->writer || packet.size() > MaxPacket)
        return false;
    auto data = packet.toStdString();
    return impl_->writer->write(&data) == dds::RETCODE_OK;
}
}
