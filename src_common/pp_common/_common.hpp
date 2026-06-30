#pragma once
#include "./_.hpp"

/* constant section */

static constexpr const uint32_t MAX_DEVICE_STATE_RELAY_SERVER_COUNT = 100;

enum struct eRelayServerType : uint8_t {
    UNSPECIFIED = 0,
    DEVICE      = 1,
    STATIC      = 2,
    THIRD       = 3,
    RELAY_TYPE_COUNT,
};

inline bool IsUnspecified(eRelayServerType Type) {
    return Type == eRelayServerType::UNSPECIFIED;
}

struct xAbstractRelayServerInfo {
    uint64_t    RelayServerId;
    xNetAddress ExportDeviceSideAddress;
    xNetAddress ExportProxySideAddrfess;
};

struct xAbstractDeviceInfo {
    uint64_t    RelayServerId;
    uint64_t    DeviceId;
    xNetAddress DeviceAddress4;
    xNetAddress DeviceAddress6;
};
