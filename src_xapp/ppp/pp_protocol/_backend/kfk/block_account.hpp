#pragma once
#include <pp_common/_.hpp>

struct xPPB_BlockAccount final : public xBinaryMessage {  // from proxy_access to relay server
public:
    void SerializeMembers() override;
    void DeserializeMembers() override;

    uint64_t StartTimestampMS;
    uint64_t AuditId;
    uint16_t BlockType;  // 1. connection count, 2 bandwith
    uint64_t BlockThresholdValue;
    uint32_t BlockPeriodMS;
    uint16_t Action;  // 1 refused(not used), 2. temp banned
};
