#pragma once
#include <librdkafka/rdkafkacpp.h>

#include <map>
#include <pp_common/_.hpp>

struct xRdKfkProducerOptions {
    std::string SecurityProtocol;
    std::string SaslMechanism;
    std::string SaslUsername;
    std::string SaslPassword;
    std::string BootstrapServerList;
};

class xRdKfkProducer final {
public:
    bool Init(const xRdKfkProducerOptions & Options);
    void Clean();
    auto GetAuditOutput() const -> std::string;

    uint64_t AddTopic(const std::string & TopicName);
    void     RemoveTopic(uint64_t TopicId);

    bool Post(uint64_t TopicId, const std::string & Key, const void * DataPtr, const size_t Size);
    bool Post(uint64_t TopicId, const std::string & Key, std::string_view Data) { return Post(TopicId, Key, Data.data(), Data.size()); }
    bool Post(uint64_t TopicId, std::string_view Data) { return Post(TopicId, {}, Data.data(), Data.size()); }

private:
    void Poll(uint64_t NowMS);
    void RemoveAllTopics();

private:
    struct xNativeTopic : xListNode {
        uint64_t         TopicId = 0;
        RdKafka::Topic * Native  = nullptr;
    };
    using xNativeTopicList = xList<xNativeTopic>;

private:
    RdKafka::Conf * KfkConf = nullptr;
    xRunState       RunState;
    std::thread     PollThread;

    RdKafka::Producer *          NativeProducer      = nullptr;
    class xKfkEventLoggerCb *    KfkEventCB          = nullptr;
    class xKfkDeliveryReportCb * KfkDeliveryReportCB = nullptr;

    xIndexedStorage<xNativeTopic> NativeTopicPool;
    xNativeTopicList              ActiveTopicList;
};
