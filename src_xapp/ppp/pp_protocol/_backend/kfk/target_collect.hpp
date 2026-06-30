#pragma once
#include <pp_common/_.hpp>

// commandId： 0x4301（17153）

class xPPB_TargetCollect final : public xBinaryMessage {
public:
    void SerializeMembers() override;
    void DeserializeMembers() override;

public:
    uint64_t         TimeMs;
    uint64_t         AuditId;
    std::string_view Domain;
    xNetAddress      IpAddress;
    uint32_t         Port;
    uint64_t         TimePeriodMs;
    uint64_t         TotalRequestCount;
};
