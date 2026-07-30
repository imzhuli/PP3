#pragma once
#include <abstract/relay_abstract.hpp>

struct xRelayDeviceConnection;

struct xRelayDevice : xListNode {
    xRelayDeviceConnection * Connection    = nullptr;
    uint64_t                 DeviceId      = {};
    size_t                   HeartbeatSize = 0;
    ubyte                    HeartbeatBuffer[72];
};

struct xRelayDeviceConnection : xListNode {
    uint64_t ConnectionId = 0;
    uint64_t Ipv4DeviceId = 0;
    uint64_t Ipv6DeviceId = 0;
};

struct xRelayDeviceSideVirtualConnection : xListNode {
    uint64_t DeviceId            = 0;
    uint64_t VirtualConnectionId = 0;
};

class xRelayDeviceSideService final
    : public xRaii {
public:
    static constexpr const size_t MAX_DEVICE_COUNT            = 50'0000;
    static constexpr const size_t MAX_DEVICE_CONNECTION_COUNT = MAX_DEVICE_COUNT / 2;

public:
    xRelayDeviceSideService(const xNetAddress & BindAddress);
    ~xRelayDeviceSideService();

    void Tick(uint64_t NowMS);

private:
    using xDevicePool           = xIndexedStorageStatic<xRelayDevice, MAX_DEVICE_COUNT>;
    using xDeviceConnectionPool = xIndexedStorageStatic<xRelayDeviceConnection, MAX_DEVICE_CONNECTION_COUNT>;

    xTcpService           TcpService;
    xDevicePool           DevicePool;
    xDeviceConnectionPool DeviceConnectionPool;
};
