#pragma once
#include "../../abstract/relay_abstract.hpp"
#include "../small_server_list_tcp_client.hpp"

class xRelayInfoObserver final : public xRaii {
public:
    xRelayInfoObserver();
    ~xRelayInfoObserver();
    void Tick(uint64_t NowMS) { TcpClient.Tick(NowMS); }
    void UpdateServerList(const xServerInfo * ServerInfoList, size_t ServerInfoListSize) { TcpClient.UpdateServerList(ServerInfoList, ServerInfoListSize); }

private:
    bool OnPacket(const xTcpClientPoolConnectionHandle & Handle, xPacketCommandId CmdId, xPacketRequestId ReqId, ubyte * Payload, size_t PayloadSize);

private:
    struct xLocalRelayInfo : xListNode {
        xNetAddress ExportDeviceSideAddress;
        xNetAddress ExportProxySideAddrfess;
    };
    std::array<xLocalRelayInfo, MAX_RELAY_SERVER_LIST_SIZE> RelayServerList;

    xSmallServerListTcpClient TcpClient;
};
