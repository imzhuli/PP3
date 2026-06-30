#pragma once
#include "./rdkafka_producer.hpp"

#include <librdkafka/rdkafkacpp.h>

#include <map>
#include <pp_common/_.hpp>

class xRdKfkOneTopicProducer final {
public:
    bool Init(const std::string & Topic, const std::map<std::string, std::string> & KafkaParams);
    void Clean();

    bool Post(const std::string & Key, const void * DataPtr, const size_t Size) { return Producer.Post(DefuaultTopicId, Key, DataPtr, Size); }
    bool Post(const std::string & Key, std::string_view Data) { return Post(Key, Data.data(), Data.size()); }
    bool Post(std::string_view Data) { return Post({}, Data.data(), Data.size()); }

    std::string GetAuditOutput() const { return Producer.GetAuditOutput(); }

private:
    xRdKfkProducer Producer;
    uint64_t       DefuaultTopicId = {};
};