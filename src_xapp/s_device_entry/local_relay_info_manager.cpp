#include "./local_relay_info_manager.hpp"

#include <pp_common/service_runtime.hpp>

xDE_LocalRelayInfoManager::xDE_LocalRelayInfoManager() {
    SetRaiiReady();
}

xDE_LocalRelayInfoManager::~xDE_LocalRelayInfoManager() {
    if (!IsRaiiReady()) {
        return;
    }
}

void xDE_LocalRelayInfoManager::UpdateRelayServerInfo(const xDE_LocalRelayInfoBase & ServerInfo) {
    auto Index = ExtractIndexFromRelayServerId(ServerInfo.ServerId);
    if (Index >= MAX_RELAY_SERVER_LIST_SIZE) {
        return;
    }
    auto & Node = FullRelayInfoList[Index];
    if (Node.BaseInfo == ServerInfo) {
        DEBUG_LOG("ServerInfo remain unchanged");
        return;
    }
    // remove
    xLocalRelayInfoList::Remove(Node);
    Reset(Node.BaseInfo, ServerInfo);

    if (!RegionService) {
        return;
    }
    auto Future = xRegionFuture();
    RegionService->GetRegion(ServerInfo.ExportDeviceEntryAddress, Future);
    assert(Future.IsReady);

    if (!Future.CountryId) {
        DEBUG_LOG("invalid country id for relay info is found");
        return;
    }

    auto & ListEntry = RelayInfoListByCountry[*Future.CountryId];
    if (ServerInfo.ExportDeviceEntryAddress.Is4()) {
        ListEntry.V4.AddTail(Node);
    } else if (ServerInfo.ExportDeviceEntryAddress.Is6()) {
        ListEntry.V4.AddTail(Node);
    } else {
        ++Audit.InvalidRelayServerAddress;
    }
    DEBUG_LOG("Update RelayServer: ServerId=%" PRIx64 ", Region=%x", ServerInfo.ServerId, (unsigned)*Future.CountryId);
}

void xDE_LocalRelayInfoManager::RemoveRelayServerInfo(uint64_t ServerId) {
    auto Index = ExtractIndexFromRelayServerId(ServerId);
    if (Index >= MAX_RELAY_SERVER_LIST_SIZE) {
        return;
    }
    auto & Node = FullRelayInfoList[Index];
    if (Node.BaseInfo.ServerId != ServerId) {
        return;
    }
    DEBUG_LOG("RemoveRelayServer: ServerId=%" PRIx64 "", ServerId);
    xLocalRelayInfoList::Remove(Node);
    Reset(Node.BaseInfo);
}
