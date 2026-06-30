#pragma once
#include <pp_common/_.hpp>
#include <pp_common/_error.hpp>

struct xPP_BlockAccount final : public xBinaryMessage {  // from proxy_access to relay server
public:
    void SerializeMembers() override;
    void DeserializeMembers() override;

    uint64_t            StartTimestampMS;
    uint64_t            GlobalAuthId;
    eBlockAccountReason BlockReason;  // 1 refused(not used), 2. temp banned
    uint64_t            BlockThreshold;
    uint64_t            BlockTriggerValue;
    uint32_t            BlockPeriodMS;
};
