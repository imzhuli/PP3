#pragma once
#include "../abstract/relay_abstract.hpp"
#include "./small_server_list_tcp_client.hpp"

class xRemoteRelayService : xRelayServiceStub {
public:
private:
    // relay server list:
    std::unique_ptr<xSmallServerListTcpClient> ServerListClient;
};
