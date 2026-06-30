#pragma once
#include <pp_common/_.hpp>

struct xAudit_DeviceSelector {

    const char * AuditName = "xAudit_DeviceSelector";

    std::string ServerUuid;
    uint64_t    ServerLocalTimestampMS;
    uint64_t    AuditDuration;

    uint64_t TotalDeviceCount;
    uint64_t NewDeviceCount;
    uint64_t RemovedDeviceCount;
    uint64_t TimeoutDeviceCount;
    uint64_t EnabledDeviceCount;

    uint64_t DeviceUuidConflict;
    uint64_t DeviceSelectionCount;

    //
};
