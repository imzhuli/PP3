#include "./rdkafka_wrapper.hpp"

#include <pp_common/service_runtime.hpp>
#include <typeinfo>

/* Callback object */

bool xRdKfkOneTopicProducer::Init(const std::string & Topic, const std::map<std::string, std::string> & KafkaParams) {
    auto Options = xRdKfkProducerOptions{
        .SecurityProtocol    = KafkaParams.at("security.protocol"),
        .SaslMechanism       = KafkaParams.at("sasl.mechanism"),
        .SaslUsername        = KafkaParams.at("sasl.username"),
        .SaslPassword        = KafkaParams.at("sasl.password"),
        .BootstrapServerList = KafkaParams.at("bootstrap.servers"),
    };
    if (!Producer.Init(Options)) {
        return false;
    }
    if (!(DefuaultTopicId = Producer.AddTopic(Topic))) {
        Producer.Clean();
        return false;
    }
    return true;
}

void xRdKfkOneTopicProducer::Clean() {
    Producer.Clean();
}
