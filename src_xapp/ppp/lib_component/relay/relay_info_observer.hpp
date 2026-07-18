#pragma once
#include "../../abstract/relay_abstract.hpp"

class xRelayInfoObserver final : xRaii {
public:
    void OnRelayServerInfo(uint64_t ServerId, const xRelayServerInfo & RelayInfo);

private:
    xTcpClientPool ClientPool;
};
