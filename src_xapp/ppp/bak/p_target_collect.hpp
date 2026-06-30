#pragma once
#include <pp_common/_.hpp>

struct xPP_TargetCollect final : public xBinaryMessage {  // from proxy_access to relay server
public:
    void SerializeMembers() override;
    void DeserializeMembers() override;

    uint64_t         GlobalAuthId;  // global authid / AuthId
    xNetAddress      TargetAddress;
    std::string_view TargetHostView;
    //
    uint64_t         StartTimestampMS;
    uint64_t         PeriodMS;
    //
    uint64_t         Count = 0;
    //
    uint32_t         Hash;
};
