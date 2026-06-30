#include "./rdkafka_producer.hpp"

#include <pp_common/service_runtime.hpp>

static constexpr const size_t   NATIVE_TOPIC_POOL_SIZE           = 500;
static constexpr const uint64_t NATIVE_PRODUCER_POLL_TIMEOUT_MS  = 50;
static constexpr const uint64_t NATIVE_PRODUCER_FLUSH_TIMEOUT_MS = 60'000;

class xKfkEventLoggerCb : public RdKafka::EventCb {
    void event_cb(RdKafka::Event & event) override;
};

class xKfkDeliveryReportCb : public RdKafka::DeliveryReportCb {
public:
    static constexpr const uint64_t AuditTimeoutMS = 10 * 60'000;
    struct xAudit {
        uint64_t AuditLastOffset            = 0;
        uint64_t AuditLastSegAverageLatency = 0;  // micro->milli
    };

    void   Tick(uint64_t NowMS);
    void   Reset();
    xAudit GetAudit() const;

private:
    void dr_cb(RdKafka::Message & message) override;

private:
    size_t TotalSuccess = 0;
    size_t TotalFailure = 0;

    // audit
    xTicker LocalTicker;
    xAudit  Audit = {};

    xel::xSpinlock AuditLock            = {};
    uint64_t       LastAuditTimestampMS = 0;
    uint64_t       LastSegMessageCount  = 0;
    uint64_t       LastSegTotalLatency  = 0;
};

void xKfkEventLoggerCb::event_cb(RdKafka::Event & event) {
    switch (event.type()) {
        case RdKafka::Event::EVENT_ERROR: {
            Logger->E("Kafka error: %s", RdKafka::err2str(event.err()).c_str());
        } break;

        case RdKafka::Event::EVENT_LOG: {
            // This is where librdkafka's log messages come through
            auto severity = event.severity();
            auto fac      = event.fac();
            auto msg      = event.str();
            if (severity <= RdKafka::Event::EVENT_SEVERITY_ERROR) {
                Logger->E("[%s]", msg.c_str());
            } else {
                Logger->I("[%s]", msg.c_str());
            }
        } break;

        default: {
        } break;
    }
}

void xKfkDeliveryReportCb::dr_cb(RdKafka::Message & message) {
    auto G = xel::xSpinlockGuard(AuditLock);
    if (message.err()) {
        DEBUG_LOG("error=%s", message.errstr().c_str());
        ++TotalFailure;
    } else {
        DEBUG_LOG("post=%zi", (size_t)message.offset());
        ++TotalSuccess;
        ++LastSegMessageCount;
        LastSegTotalLatency  += message.latency();
        Audit.AuditLastOffset = message.offset();
    }
}

void xKfkDeliveryReportCb::Reset() {
    auto G = xel::xSpinlockGuard(AuditLock);
    xel::Reset(TotalSuccess);
    xel::Reset(TotalFailure);
    xel::Reset(Audit.AuditLastOffset);
    xel::Reset(Audit.AuditLastSegAverageLatency);
    xel::Reset(LastSegMessageCount);
    xel::Reset(LastSegTotalLatency);
}

void xKfkDeliveryReportCb::Tick(uint64_t NowMS) {
    LocalTicker.Update(NowMS);
    if (NowMS - LastAuditTimestampMS < AuditTimeoutMS) {
        return;
    }
    do {
        auto G = xel::xSpinlockGuard(AuditLock);
        if (!LastSegMessageCount) {
            Audit.AuditLastSegAverageLatency = 0;
        } else {
            Audit.AuditLastSegAverageLatency = LastSegTotalLatency / LastSegMessageCount / 1000;  // micro -> milli
        }
        xel::Reset(LastSegMessageCount);
        xel::Reset(LastSegTotalLatency);
    } while (false);
    LastAuditTimestampMS = LocalTicker();
}

xKfkDeliveryReportCb::xAudit xKfkDeliveryReportCb::GetAudit() const {
    auto G = xel::xSpinlockGuard(AuditLock);
    return Audit;
}

////////////// producer

bool xRdKfkProducer::Init(const xRdKfkProducerOptions & Options) {
    if (!NativeTopicPool.Init(NATIVE_TOPIC_POOL_SIZE)) {
        Logger->E("failed to init native topic pool");
        return false;
    }
    auto NTPCleaner = xScopeGuard([this] { NativeTopicPool.Clean(); });

    KfkEventCB      = new xKfkEventLoggerCb();
    auto ECBCleaner = xScopeGuard([this] { delete Steal(KfkEventCB); });

    KfkDeliveryReportCB = new xKfkDeliveryReportCb();
    auto DRCBCleaner    = xScopeGuard([this] { delete Steal(KfkDeliveryReportCB); });

    KfkConf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    if (!KfkConf) {
        Logger->E("Failed to create kafka config");
        return false;
    }
    auto ConfCleaner = xScopeGuard([this] { delete Steal(KfkConf); });

    // set up configs
    auto errstr      = std::string();
    auto KafkaParams = std::map<std::string, std::string>{
        { "security.protocol", Options.SecurityProtocol },
        { "sasl.mechanism", Options.SaslMechanism },
        { "sasl.username", Options.SaslUsername },
        { "sasl.password", Options.SaslPassword },
        { "bootstrap.servers", Options.BootstrapServerList },
    };
    for (auto & [K, V] : KafkaParams) {
        if (RdKafka::Conf::CONF_OK != KfkConf->set(K, V, errstr)) {
            Logger->E("failed to set kafka producer param: %s -> %s, error=%s", K.c_str(), V.c_str(), errstr.c_str());
            return false;
        }
    }
    if (RdKafka::Conf::CONF_OK != KfkConf->set("event_cb", KfkEventCB, errstr)) {
        Logger->E("failed to set kafka producer param: %s, error=%s", "event_cb", errstr.c_str());
        return false;
    }
    if (RdKafka::Conf::CONF_OK != KfkConf->set("dr_cb", KfkDeliveryReportCB, errstr)) {
        Logger->E("failed to set kafka producer param: %s, error=%s", "dr_cb", errstr.c_str());
        return false;
    }

    // create native producer:
    NativeProducer = RdKafka::Producer::create(KfkConf, errstr);
    if (!NativeProducer) {
        Logger->E("failed to create native producer: error=%s", errstr.c_str());
        return false;
    }
    auto ProducerCleaner = xScopeGuard([&] { delete Steal(NativeProducer); });

    NTPCleaner.Dismiss();
    ECBCleaner.Dismiss();
    DRCBCleaner.Dismiss();
    ConfCleaner.Dismiss();
    ProducerCleaner.Dismiss();

    RuntimeAssert(RunState.Start());
    PollThread = std::thread{ [this] {
        while (RunState) {
            Poll(NATIVE_PRODUCER_POLL_TIMEOUT_MS);
        }
    } };
    return true;
}

void xRdKfkProducer::Clean() {
    NativeProducer->flush(NATIVE_PRODUCER_FLUSH_TIMEOUT_MS);

    RunState.Stop();
    PollThread.join();
    Reset(PollThread);
    RunState.Finish();

    RemoveAllTopics();
    auto NTPCleaner      = xScopeGuard([this] { NativeTopicPool.Clean(); });
    auto ECBCleaner      = xScopeGuard([this] { delete Steal(KfkEventCB); });
    auto DRCBCleaner     = xScopeGuard([this] { delete Steal(KfkDeliveryReportCB); });
    auto ConfCleaner     = xScopeGuard([this] { delete Steal(KfkConf); });
    auto ProducerCleaner = xScopeGuard([&] { delete Steal(NativeProducer); });
}

void xRdKfkProducer::Poll(uint64_t TimeoutMS) {
    NativeProducer->poll((int)TimeoutMS);
    KfkDeliveryReportCB->Tick(xel::GetTimestampMS());
}

uint64_t xRdKfkProducer::AddTopic(const std::string & TopicName) {
    auto TopicId = NativeTopicPool.Acquire();
    if (!TopicId) {
        return {};
    }
    auto TopicIdCleaner = xScopeGuard([=, this] {
        NativeTopicPool.Release(TopicId);
    });

    auto & Node  = NativeTopicPool[TopicId];
    Node.TopicId = TopicId;

    auto errstr = std::string();
    if (!(Node.Native = RdKafka::Topic::create(NativeProducer, TopicName, nullptr, errstr))) {
        Logger->E("error=%s", errstr.c_str());
        return {};
    }

    TopicIdCleaner.Dismiss();
    return TopicId;
}

void xRdKfkProducer::RemoveTopic(uint64_t TopicId) {
    auto NP = NativeTopicPool.CheckAndGet(TopicId);
    if (!NP) {
        return;
    }
    assert(NP->TopicId == TopicId);
    delete NP->Native;
    NativeTopicPool.Release(NP->TopicId);
}

bool xRdKfkProducer::Post(uint64_t TopicId, const std::string & Key, const void * DataPtr, const size_t Size) {
    auto NP = NativeTopicPool.CheckAndGet(TopicId);
    assert(NP);

    RdKafka::ErrorCode resp = NativeProducer->produce(
        NP->Native,                      // 主题对象
        RdKafka::Topic::PARTITION_UA,    // 分区（PARTITION_UA 表示自动分配）
        RdKafka::Producer::RK_MSG_COPY,  // 消息复制策略
        (char *)(DataPtr),               // 消息内容
        Size,                            // 消息长度
        (Key.empty() ? nullptr : &Key),  // 键（可选）
        NULL                             // 消息头（可选）
    );
    if (resp != RdKafka::ERR_NO_ERROR) {
        DEBUG_LOG("failed to post payload");
        return false;
    }
    return true;
}

void xRdKfkProducer::RemoveAllTopics() {
    while (auto NP = ActiveTopicList.PopHead()) {
        delete NP->Native;
        NativeTopicPool.Release(NP->TopicId);
    }
}

std::string xRdKfkProducer::GetAuditOutput() const {
    auto Audit = KfkDeliveryReportCB->GetAudit();
    auto OS    = std::ostringstream();
    OS << "AuditLastOffset:" << Audit.AuditLastOffset << endl;
    OS << "AuditLastSegAverageLatency:" << Audit.AuditLastSegAverageLatency << endl;
    return OS.str();
}
