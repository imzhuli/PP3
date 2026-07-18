#pragma once
#include "../../abstract/relay_abstract.hpp"

class xRelayInfoRegister final : xRaii {
public:
    xRelayInfoRegister(const xNetAddress & MasterDispatcherAddress, const xRelayServerInfo & RelayInfo);
    ~xRelayInfoRegister();

public:
    void Tick(uint64_t NowMS);

private:
    xTcpClient DispatcherClient;
};
