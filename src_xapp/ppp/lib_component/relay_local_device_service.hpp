#pragma once
#include "../abstract/device_locator_abstract.hpp"
#include "../abstract/dns_abstract.hpp"
#include "../abstract/relay_abstract.hpp"
#include "./pa_future.hpp"

#include <map>

struct xRelayDnsResultFuture : xDnsReultFuture {
    uint64_t      RelayServerId;
    uint64_t      RelaySideDeviceId;
    uint64_t      ProxySideConnectionId;
    xFutureHandle CreateConnectionFutureHandle;  // create connection future handle;
    uint16_t      TargetPort;
};

struct xRelayLocalDevice {
    uint64_t    DeviceId;
    xNetAddress BindAddress;
    xNetAddress ExportAddress;
    bool        EnableTcp;
    bool        EnableUdp;

    std::string ToString() const;
};

struct xRelayLocalDeviceConnectionTimeoutNode : xListNode {
    uint64_t TimestampMS = 0;
};
using xRelayLocalDeviceConnectionTimeoutList = xList<xRelayLocalDeviceConnectionTimeoutNode>;

struct xRelayLocalDeviceConnection final
    : xRelayLocalDeviceConnectionTimeoutNode
    , xTcpConnection {
    uint64_t      ConnectionId          = 0;
    uint64_t      ProxySideConnectionId = 0;
    xFutureHandle CreateConnectionFutureHandle;
    bool          DeleteMark = false;

    void ClearFutures() { Reset(CreateConnectionFutureHandle); }
};

struct xRelayLocalDeviceUdpChannelTimeoutNode : xListNode {
    uint64_t TimestampMS = 0;
};
using xRelayLocalDeviceUdpChannelTimeoutList = xList<xRelayLocalDeviceUdpChannelTimeoutNode>;

struct xRelayLocalDeviceUdpChannel final
    : xRelayLocalDeviceUdpChannelTimeoutNode
    , xUdpChannel {
    uint64_t DeviceId              = 0;
    uint64_t UdpChannelId          = 0;
    uint64_t ProxySideUdpChannelId = 0;
    bool     DeleteMark            = false;
};

struct xRelayLocalBindingOption {
    xNetAddress BindAddress;
    xNetAddress ExportAddress;
    bool        EnableTcp = false;
    bool        EnableUdp = false;

    std::string ToString() const;
};

class xRelayLocalBindingService final
    : public xRelayServiceAbstract
    , public xDeviceLocatorServiceAbstract
    , xTcpConnection::iListener
    , xUdpChannel::iListener {

public:
    bool Init(uint64_t ServerId, const std::string & AddressPairFile);
    bool Init(uint64_t ServerId, const std::vector<xRelayLocalBindingOption> & BindAddressPairList);
    void Clean();
    void BindProxyService(xProxyServiceAbstract * ProxyService);
    void BindDnsService(xDnsServiceAbstract * DnsService);
    void SetDeviceBufferSize(size_t Size);
    void Tick(uint64_t NowMS);

    auto OutputAudit() const -> std::string;

    void AcquireDevice(const xDeviceRequest & Request, xAcquireDeviceFuture & Future) override;
    void CreateConnection(uint64_t RelayServerId, uint64_t DeviceId, uint64_t ProxySideConnectionId, const std::string_view & TargetHostnameView, uint16_t TargetPort, xRelayCreateConnectionFuture & Future) override;
    void CreateConnection(uint64_t RelayServerId, uint64_t DeviceId, uint64_t ProxySideConnectionId, const xNetAddress & TargetAddress, xRelayCreateConnectionFuture & Future) override;
    void CreateUdpChannel(uint64_t RelayServerId, uint64_t DeviceId, uint64_t ProxySideUdpChannelId, xRelayCreateUdpChannelFuture & Future) override;
    void DestroyConnection(uint64_t RelayServerId, uint64_t ConnectionId) override;
    void DestroyUdpChannel(uint64_t RelayServerId, uint64_t UdpChannelId) override;
    void PostData(uint64_t RelayServerId, uint64_t ConnectionId, const void * Payload, size_t PayloadSize) override;
    void PostData(uint64_t RelayServerId, uint64_t UdpChannelId, const xel::xNetAddress & TargetAddress, const void * Payload, size_t PayloadSize) override;
    void KeepUdpChannelAlive(uint64_t RelayServerId, uint64_t UdpChannelId) override;

private:
    bool CreateLocalDeviceList(const std::vector<xRelayLocalBindingOption> & OptionList);
    void DestroyLocalDeviceList();
    auto GetDevice(uint64_t DeviceId) const -> const xRelayLocalDevice *;
    bool KeepAlive(xRelayLocalDeviceConnection * Connection);
    bool KeepAlive(xRelayLocalDeviceUdpChannel * UdpChannel);
    void DeferDestroyConnection(xRelayLocalDeviceConnection * Connection);
    void DeferDestroyUdpChannel(xRelayLocalDeviceUdpChannel * UdpChannel);
    void DeferDestroyIdleConnections();
    void DeferDestroyIdleUdpChannels();
    void CleanDyingConnections();
    void CleanDyingUdpChannels();
    void CleanAllConnections();
    void CleanAllUdpChannels();

    void CreateConnection(uint64_t ProxySideConnectionId, const xNetAddress & DeviceBindAddress, const xNetAddress & TargetAddress, xRelayCreateConnectionFuture & Future);
    void CreateConnectionWithDnsResult(xRelayDnsResultFuture * DnsFuture);
    void ProcessDnsResults();

    const xRelayLocalDevice * FindDeviceByExportAddress(const xNetAddress & ExportAddress);

private:  // listener
    void   OnConnected(xTcpConnection * TcpConnectionPtr) override;
    void   OnPeerClose(xTcpConnection * TcpConnectionPtr) override;
    void   OnFlush(xTcpConnection * TcpConnectionPtr) override {}
    size_t OnData(xTcpConnection * TcpConnectionPtr, ubyte * DataPtr, size_t DataSize) override;
    void   OnData(xUdpChannel * ChannelPtr, ubyte * DataPtr, size_t DataSize, const xNetAddress & RemoteAddress) override;

private:
    using xLocalDeviceList = std::vector<xRelayLocalDevice>;

    xTicker LocalTicker;
    size_t  DefaultBufferSize = 0;

    uint64_t                                          LocalRelayServerId = 0;
    xLocalDeviceList                                  LocalDeviceList;
    std::map<xNetAddress, uint64_t>                   LocalDeviceExportAddressMap;
    xel::xIndexedStorage<xRelayLocalDeviceConnection> LocalConnectionPool;
    xel::xIndexedStorage<xRelayLocalDeviceUdpChannel> LocalUdpChannelPool;

    xProxyServiceAbstract *                   ProxyService = nullptr;
    xDnsServiceAbstract *                     DnsService   = nullptr;
    xFuturePoolManager<xRelayDnsResultFuture> DnsFutureManager;

    xRelayLocalDeviceConnectionTimeoutList ConnectionEstablishTimeoutList;
    xRelayLocalDeviceConnectionTimeoutList ConnectionIdleTimeoutList;
    xRelayLocalDeviceConnectionTimeoutList ConnectionKillList;

    xRelayLocalDeviceUdpChannelTimeoutList UdpChannelIdleTimeoutList;
    xRelayLocalDeviceUdpChannelTimeoutList UdpChannelKillList;

    struct {
        size_t ConnectionCount = 0;
        size_t UdpChannelCount = 0;
    } Audit;
};
