#pragma once
#include <pp_common/_.hpp>

struct xPP_AuditCollect final : public xBinaryMessage {  // from proxy_access to relay server
public:
    void SerializeMembers() override;
    void DeserializeMembers() override;

    uint64_t GlobalAuthId;
    uint64_t StartTimestampMS;
    uint64_t PeriodMS;
    uint64_t TotalTcpConnections;
    uint64_t TotalTcpBytesFromClient;
    uint64_t TotalTcpBytesToClient;
    uint64_t TotalUdpChannels;
    uint64_t TotalUdpBytesFromClient;
    uint64_t TotalUdpBytesToClient;
};
