#pragma once
#include "../../abstract/relay_abstract.hpp"
#include "../small_server_list_tcp_client.hpp"

class xRelayInfoObserver final : public xSmallServerListTcpClient {
private:
    bool OnPacket(const xTcpClientPoolConnectionHandle & Handle, xPacketCommandId CmdId, xPacketRequestId ReqId, ubyte * Payload, size_t PayloadSize) override;

private:
    struct xLocalRelayInfo : xListNode {
        xNetAddress ExportDeviceSideAddress;
        xNetAddress ExportProxySideAddrfess;
    };
    std::array<xLocalRelayInfo, MAX_RELAY_SERVER_LIST_SIZE> RelayServerList;
};
