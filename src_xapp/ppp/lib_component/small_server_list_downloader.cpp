#include "./small_server_list_downloader.hpp"

#include "../pp_protocol/command.hpp"
#include "../pp_protocol/p_small_server_list.hpp"

#include <pp_common/service_runtime.hpp>

#ifndef NDEBUG
static constexpr const uint64_t UPDATE_SERVER_LIST_TIMEOUT_MS = 60'000;
#else
static constexpr const uint64_t UPDATE_SERVER_LIST_TIMEOUT_MS = 5 * 60'000;
#endif

bool xSmallServerListDownloader::Init(const xNetAddress & ServerListServerAddress) {
    return Init(ServerListServerAddress, ServerListServerAddress.Decay());
}

bool xSmallServerListDownloader::Init(const xNetAddress & ServerListServerAddress, const xNetAddress & LocalBindAddress) {
    std::random_device rd;
    RandomGeneratorHolder.CreateValue(rd());

    ServerListServerList.reserve(MAX_SMALL_SERVER_LIST_SIZE);
    ServerListServerMasterAddress = ServerListServerAddress;
    if (!DownloadService.Init(ServiceIoContext, LocalBindAddress)) {
        Reset(ServerListServerMasterAddress);
        return false;
    }
    DownloadService.OnPacket = Delegate(&xSmallServerListDownloader::OnUdpPacket, this);
    return true;
}

void xSmallServerListDownloader::Clean() {
    for (size_t I = 0; I < MAX_SMALL_SERVER_LIST_SIZE; ++I) {
        auto & Node = EnabledServerGroupMap[I];
        if (Node.Enabled) {
            DisableServerGroup(xServerGroup(I));
        }
    }

    DownloadService.Clean();
    Reset(ServerListServerMasterAddress);
    RandomGeneratorHolder.Destroy();
}

void xSmallServerListDownloader::Tick(uint64_t NowMS) {
    LocalTicker.Update(NowMS);
    UpdateServerListSlaveList();
    UpdateOneEnabledServerList();
}

void xSmallServerListDownloader::EnableServerGroup(xServerGroup Type) {
    assert(Type != ST_SERVER_LIST);
    auto & Node = EnabledServerGroupMap[Type];
    assert(!Node.Enabled && !xListNode::IsLinked(Node) && !Node.ServerList && !Node.ServerGroup);
    Node.Enabled     = true;
    Node.ServerGroup = Type;
    Node.ServerList  = new xSmallServerList();
    EnabledServerGroupList.AddTail(Node);
}

void xSmallServerListDownloader::DisableServerGroup(xServerGroup Type) {
    auto & Node = EnabledServerGroupMap[Type];
    assert(Node.Enabled && xListNode::IsLinked(Node) && Node.ServerList && Node.ServerGroup);
    Node.Enabled               = false;
    Node.ServerGroup           = 0;
    Node.LastUpdateTimestampMS = 0;
    Node.ServerListSize        = 0;
    delete Steal(Node.ServerList);
    xEnabledServerGroupList::Remove(Node);
}

xSmallServerListDownloader::xServerListView xSmallServerListDownloader::GetServerListView(xServerGroup Type) {
    auto & Node = EnabledServerGroupMap[Type];
    if (!Node.Enabled) {
        return {};
    }
    return { Node.ServerList->data(), Node.ServerListSize };
}

void xSmallServerListDownloader::UpdateServerListSlaveList() {
    auto NowMS = LocalTicker();
    if (LastUpdateServerListSlaveListTimestampMS > NowMS - UPDATE_SERVER_LIST_TIMEOUT_MS) {
        return;
    }
    LastUpdateServerListSlaveListTimestampMS = NowMS;

    auto Req        = xPP_GetSmallServerList();
    Req.ServerGroup = ST_SERVER_LIST;
    DownloadService.PostMessage(ServerListServerMasterAddress, Cmd_DownloadSmallServerList, xPacketRequestId(Req.ServerGroup), Req);
}

void xSmallServerListDownloader::UpdateOneEnabledServerList() {
    auto NowMS = LocalTicker();
    auto Cond  = [Timepoint = NowMS - UPDATE_SERVER_LIST_TIMEOUT_MS](const xEnabledServerGroupNode & Node) {
        return Node.LastUpdateTimestampMS <= Timepoint;
    };
    // update only one server list each time.
    auto PNode = EnabledServerGroupList.PopHead(Cond);
    if (!PNode) {
        return;
    }

    PNode->LastUpdateTimestampMS = NowMS;
    EnabledServerGroupList.AddTail(*PNode);

    auto Req                     = xPP_GetSmallServerList();
    Req.ServerGroup              = PNode->ServerGroup;
    auto ServerListServerAddress = SelectServerListServer();
    DownloadService.PostMessage(ServerListServerAddress, Cmd_DownloadSmallServerList, xPacketRequestId(Req.ServerGroup), Req);
}

void xSmallServerListDownloader::OnUdpPacket(const xUdpServiceChannelHandle &, xPacketCommandId CommandId, xPacketRequestId RequestId, ubyte * Payload, size_t PayloadSize) {
    if (CommandId != Cmd_DownloadSmallServerListResp || RequestId > MAX_SMALL_SERVER_LIST_SIZE) {
        return;
    }
    auto Resp       = xPP_GetSmallServerListResp();
    Resp.ServerList = &TempSmallServerListForResponse;
    if (!Resp.Deserialize(Payload, PayloadSize)) {
        return;
    }
    if (RequestId > MAX_SMALL_SERVER_LIST_SIZE) {
        return;
    }
    auto ServerGroup = xServerGroup(RequestId);
    auto PNode       = &EnabledServerGroupMap[ServerGroup];
    if (!PNode->Enabled) {
        return;
    }

    bool VersionChange = (PNode->VersionTimestampMS != Resp.VersionTimestampMS);
    if (VersionChange) {
        for (size_t I = 0; I < Resp.ServerListSize; ++I) {
            PNode->ServerList[I] = Resp.ServerList[I];
        }
        PNode->ServerListSize     = Resp.ServerListSize;
        PNode->VersionTimestampMS = Resp.VersionTimestampMS;
        OnServerListUpdated(ServerGroup, PNode->ServerList->data(), PNode->ServerListSize, PNode->VersionTimestampMS);
    }
    return;
}

const xNetAddress & xSmallServerListDownloader::SelectServerListServer() {
    if (auto SlaveServerListSize = ServerListServerList.size()) {
        auto RandomDistribution = std::uniform_int_distribution<uint32_t>(0, SlaveServerListSize - 1);
        auto Random             = RandomDistribution(*RandomGeneratorHolder);
        return ServerListServerList[Random];
    }
    return ServerListServerMasterAddress;
}