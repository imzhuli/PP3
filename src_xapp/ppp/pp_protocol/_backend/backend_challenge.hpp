#pragma once
#include <pp_common/_.hpp>

class xBackendChallenge : public xBinaryMessage {
public:
    void SerializeMembers() override { W(AppKey, TimestampMS, ChallengeValue); }
    void DeserializeMembers() override { R(AppKey, TimestampMS, ChallengeValue); }

public:
    static std::string Sign(const std::string & Source);
    std::string        GenerateChallengeString(const std::string & AppSecret) const;

public:
    std::string AppKey;
    uint64_t    TimestampMS;
    std::string ChallengeValue;
};

class xBackendChallengeResp : public xBinaryMessage {
public:
    void SerializeMembers() override { W(ErrorCode); }
    void DeserializeMembers() override { R(ErrorCode); }

public:
    uint32_t ErrorCode;
};
