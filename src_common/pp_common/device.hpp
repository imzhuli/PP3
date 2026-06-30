#pragma once
#include "./_.hpp"

struct xDeviceLocator final {
    uint64_t             RelayServerRuntimeId;
    uint64_t             RelayServerSideDeviceId;
    std::strong_ordering operator<=>(const xDeviceLocator &) const = default;
};

using xDeviceFlag              = uint32_t;
using xDevicePoolId            = uint32_t;
using xDeviceSelectionStrategy = uint16_t;

constexpr const xDeviceFlag DF_NONE        = 0x00;
constexpr const xDeviceFlag DF_ENABLE_TCP4 = 0x01 << 0;
constexpr const xDeviceFlag DF_ENABLE_TCP6 = 0x01 << 1;
constexpr const xDeviceFlag DF_ENABLE_UDP4 = 0x01 << 2;
constexpr const xDeviceFlag DF_ENABLE_UDP6 = 0x01 << 3;

constexpr const xDeviceSelectionStrategy DSS_CLEAR                 = 0;
constexpr const xDeviceSelectionStrategy DSS_IPV4                  = 0x01u << 0;
constexpr const xDeviceSelectionStrategy DSS_IPV6                  = 0x01u << 1;
constexpr const xDeviceSelectionStrategy DSS_UDP                   = 0x01u << 2;
constexpr const xDeviceSelectionStrategy DSS_DEVICE_PERSISTENT     = 0x01u << 3;
constexpr const xDeviceSelectionStrategy DSS_DEVICE_VOLATILE       = 0x01u << 4;
constexpr const xDeviceSelectionStrategy DSS_REGION_DOWNGRADE      = 0x01u << 5;
constexpr const xDeviceSelectionStrategy DSS_STATIC_EXPORT_ADDRESS = 0x01u << 6;