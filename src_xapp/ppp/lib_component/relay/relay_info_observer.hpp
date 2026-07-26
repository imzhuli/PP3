#pragma once
#include "../small_server_list_tcp_client.hpp"

class xRelayInfoObserver final : public xSmallServerListTcpClient {
private:
    bool OnPacket(const xTcpClientPoolConnectionHandle & Handle, xPacketCommandId CmdId, xPacketRequestId ReqId, ubyte * Payload, size_t PayloadSize) override;
};
