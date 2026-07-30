#pragma once
#include "../../abstract/relay_abstract.hpp"
#include "../../const/ppp_const.hpp"
#include "../small_server_list_tcp_client.hpp"

class xRelayInfoObserver final : public xRaii {
public:
    xRelayInfoObserver();
    ~xRelayInfoObserver();
    void Tick(uint64_t NowMS) {
        LocalTicker.Update(NowMS);
        TcpClient.Tick(NowMS);
        RemoveTimeoutRelayInfo();
    }
    void UpdateServerList(const xServerInfo * ServerInfoList, size_t ServerInfoListSize) { TcpClient.UpdateServerList(ServerInfoList, ServerInfoListSize); }

    struct xLocalRelayInfo {
        uint64_t    ServerId                 = {};
        xNetAddress ExportDeviceEntryAddress = {};
        xNetAddress ExportProxyEntryAddress  = {};
    };

    using xOnRelayUpdated = std::function<void(const xLocalRelayInfo &)>;
    using xOnRelayDropped = std::function<void(const xLocalRelayInfo &)>;

    xOnRelayUpdated OnRelayUpdated = Noop<>;
    xOnRelayDropped OnRelayDropped = Noop<>;

private:
    struct xInternalLocalRelayInfo
        : xListNode
        , xLocalRelayInfo {
        uint64_t LastKeepAliveTimestampMS = 0;
    };

    bool OnPacket(const xTcpClientPoolConnectionHandle & Handle, xPacketCommandId CmdId, xPacketRequestId ReqId, ubyte * Payload, size_t PayloadSize);
    void KeepAlive(xInternalLocalRelayInfo & RelayInfo);
    void Kill(xInternalLocalRelayInfo & RelayInfo);
    void RemoveTimeoutRelayInfo();

private:
    xTicker                                                         LocalTicker          = {};
    std::array<xInternalLocalRelayInfo, MAX_RELAY_SERVER_LIST_SIZE> RelayServerList      = {};
    xList<xInternalLocalRelayInfo>                                  RelayInfoTimeoutList = {};
    xSmallServerListTcpClient                                       TcpClient            = {};
};
