#pragma once
#include <pp_common/_.hpp>
#include <random>
#include <xel/object/object.hpp>

class xSmallServerListDownloader final {
    using xOnServerListUpdated = std::function<void(xServerGroup ServerGroup, const xServerInfo * ServerList, size_t ServerListSize, uint64_t VersionTimestampMS)>;

public:
    bool Init(const xNetAddress & ServerListServerMasterAddress);
    bool Init(const xNetAddress & ServerListServerMasterAddress, const xNetAddress & LocalBindAddress);
    void Clean();
    void Tick(uint64_t NowMS);

    void EnableServerGroup(xServerGroup Type);
    void DisableServerGroup(xServerGroup Type);

    struct xServerListView {
        const xServerInfo * ServerList     = nullptr;
        size_t              ServerListSize = 0;
    };
    xServerListView      GetServerListView(xServerGroup Type);
    xOnServerListUpdated OnServerListUpdated = Noop<>;

private:
    const xNetAddress & SelectServerListServer();

    void UpdateServerListSlaveList();
    void UpdateOneEnabledServerList();
    void OnUdpPacket(const xUdpServiceChannelHandle &, xPacketCommandId, xPacketRequestId, ubyte *, size_t);

private:
    using xSmallServerList = std::array<xServerInfo, MAX_SMALL_SERVER_LIST_SIZE>;
    struct xEnabledServerGroupNode : xListNode {
        bool               Enabled               = false;
        xServerGroup       ServerGroup           = 0;
        uint64_t           VersionTimestampMS    = 0;
        uint64_t           LastUpdateTimestampMS = 0;
        size_t             ServerListSize        = 0;
        xSmallServerList * ServerList            = nullptr;
    };
    using xEnabledServerGroupList = xList<xEnabledServerGroupNode>;
    using xEnabledServerGroupMap  = std::array<xEnabledServerGroupNode, MAX_SMALL_SERVER_LIST_SIZE>;

    xel::xTicker                  LocalTicker;
    xel::xNetAddress              ServerListServerMasterAddress;
    xel::xUdpService              DownloadService;
    std::vector<xel::xNetAddress> ServerListServerList;
    //
    xEnabledServerGroupList       EnabledServerGroupList;
    xEnabledServerGroupMap        EnabledServerGroupMap;
    //
    xHolder<std::mt19937>         RandomGeneratorHolder;
    uint64_t                      LastUpdateServerListSlaveListTimestampMS = 0;
    xSmallServerList              TempSmallServerListForResponse;
    //
};
