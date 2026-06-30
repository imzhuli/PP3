#pragma once
#include <pp_common/_.hpp>

struct xAudit_DeviceRelay {

    const char * AuditName = "xAudit_DeviceRelay";

    std::string ServerUuid;
    uint64_t    ServerLocalTimestampMS;
    uint64_t    AuditDuration;

    //
    uint64_t UnauthorizedConnectionCount;
    uint64_t DeviceCount;
    uint64_t EnabledDeviceCount;
    uint64_t NewDeviceCount;
    uint64_t LostDeviceCount;

    //
    uint64_t RelayConnectionCount;
    uint64_t RelayConnectionEstablishedCount;
    uint64_t RelayConnectionMissingDeviceCount;
    uint64_t RelayConnectionTimeoutCount;
    uint64_t RelayConnectionDeviceCloseCount;
    uint64_t RelayConnectionProxyCloseCount;

    //
};
