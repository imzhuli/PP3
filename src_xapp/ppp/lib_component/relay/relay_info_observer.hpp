#pragma once
#include "../../abstract/relay_abstract.hpp"

class xRelayInfoObserver final : public xRaii {
public:
    xRelayInfoObserver();
    ~xRelayInfoObserver();
    void Tick(uint64_t NowMS);

    void OnRelayDispatcherUpdated(const xServerInfo * ServerInfoList, size_t ServerInfoListSize);

private:
    void OnRelayServerRemoved(uint64_t ServerId);
    void OnRelayServerInfo(uint64_t ServerId, const xRelayServerInfo & RelayInfo);

private:
    struct xDispatcherLocalInfo {
        uint64_t    ConnectionId = 0;
        xServerInfo ServerInfo   = {};
    };
    using xDispatcherInfoContainer = std::array<xDispatcherLocalInfo, MAX_SMALL_SERVER_LIST_SIZE>;

    xDispatcherInfoContainer DispatcherInfoContainer = {};
    xTcpClientPool           ClientPool              = {};
};
