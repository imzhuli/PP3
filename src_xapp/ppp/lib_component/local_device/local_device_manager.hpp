#pragma once
#include "../../abstract/device_locator_abstract.hpp"
#include "../../abstract/device_manager_abstract.hpp"
#include "./local_device.hpp"

#include <absl/container/flat_hash_map.h>

#include <xel/core/view.hpp>

class xLocalDeviceManager;

class xLocalDeviceConnection
    : private xUdpChannel
    , public xListNode {
    friend class xLocalDeviceManager;
    uint64_t LocalId            = 0;
    uint64_t KeepAliveTimestamp = 0;
    bool     KillMark           = false;
};

class xLocalDeviceUdpChannel
    : private xUdpChannel
    , public xListNode {
    friend class xLocalDeviceManager;
    uint64_t LocalId            = 0;
    uint64_t KeepAliveTimestamp = 0;
    bool     KillMark           = false;
};

class xLocalDeviceManager
    : public xDeviceManagerAbstract {
public:
    bool Init(const std::vector<xLocalDeviceBinding> & LocalDeviceBindingList);
    void Clean();
    void Tick(uint64_t NowMS);

public:
    /**
     * @brief
     *
     * @param Address
     * @return uint64_t DeviceId (aka device index)
     */
    uint64_t FindDeviceByExportAddress(const xNetAddress & Address);

private:
    void KeepAlive(xLocalDeviceConnection & Connection);
    void KeepAlive(xLocalDeviceUdpChannel & UdpChannel);
    void DeferKill(xLocalDeviceConnection & Connection);
    void DeferKill(xLocalDeviceUdpChannel & UdpChannel);

    void ProcessIdleConnections();
    void ProcessIdleUdpChannelConnections();
    void PerformKillConnections();
    void PerformKillUdpChannels();

public:  // overrides
    void SetDeviceEventCallback(const xEventCallback & Callbacks) override { EventCallback = Callbacks; }
    void CreateConnection(uint64_t SourceContextId, uint64_t DeviceId, const xel::xNetAddress & TargetAddress, xDeviceCreateConnectionFuture & Future) override;
    void CreateConnection(uint64_t SourceContextId, uint64_t DeviceId, const std::string & TargetHost, xDeviceCreateConnectionFuture & Future) override;
    void DestroyConnection(uint64_t ConnectionId) override;
    void CreateUdpChannel(uint64_t SourceContextId, uint64_t DeviceId, xDeviceCreateUdpChannelFuture & Future) override;
    void DestroyUdpChannel(uint64_t UdpChannelId) override;
    void PostData(uint64_t ConnectionId, const void * DataPtr, size_t DataSize) override;
    void PostData(uint64_t UdpChannelId, const xNetAddress & TargetAddress, const void * DataPtr, size_t DataSize) override;

private:
    struct xLocalDevice {
        uint64_t            Index;  // as DeviceId
        xLocalDeviceBinding BindingInfo;
    };

    xTicker                                              LocalTicker;
    //
    std::vector<xLocalDevice>                            LocalDeviceList;
    absl::flat_hash_map<std::array<ubyte, 4>, uint64_t>  Ipv4ToDeviceIndex;
    absl::flat_hash_map<std::array<ubyte, 16>, uint64_t> Ipv6ToDeviceIndex;
    //
    xel::xIndexedStorage<xLocalDeviceConnection>         ConnectionPool;
    xList<xLocalDeviceConnection>                        IdleConnectionList;
    xList<xLocalDeviceConnection>                        KillConnectionList;
    //
    xel::xIndexedStorage<xLocalDeviceUdpChannel>         UdpChannelPool;
    xList<xLocalDeviceUdpChannel>                        IdleUdpChannelList;
    xList<xLocalDeviceUdpChannel>                        KillUdpChannelList;
    //
    xEventCallback                                       EventCallback;
};
