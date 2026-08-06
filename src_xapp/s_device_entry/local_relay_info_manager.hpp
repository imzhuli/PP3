#pragma once

#include <abstract/region_abstract.hpp>
#include <compare>
#include <const/ppp_const.hpp>
#include <map>
#include <pp_common/_.hpp>
#include <pp_common/_region.hpp>

struct xDE_LocalRelayInfoBase {
    uint64_t    ServerId;
    xNetAddress ExportDeviceEntryAddress;

    std::strong_ordering operator<=>(const xDE_LocalRelayInfoBase &) const = default;
};

struct xDE_LocalRelayInfo : xListNode {
    xDE_LocalRelayInfoBase BaseInfo;
};
using xLocalRelayInfoList = xList<xDE_LocalRelayInfo>;

struct xDE_LocalRelayInfListEntry {
    xLocalRelayInfoList V4;
    xLocalRelayInfoList V6;
};

class xDE_LocalRelayInfoManager final : public xRaii {
public:
    xDE_LocalRelayInfoManager();
    ~xDE_LocalRelayInfoManager();

public:
    void SetRegionService(xRegionServiceAbstract * Service) { RegionService = Service; }
    void UpdateRelayServerInfo(const xDE_LocalRelayInfoBase & ServerInfo);
    void RemoveRelayServerInfo(uint64_t ServerId);
    auto GetRelayServerByDeviceIp(const xNetAddress & DeviceAddress) -> xNetAddress;

private:
    std::array<xDE_LocalRelayInfo, MAX_RELAY_SERVER_LIST_SIZE> FullRelayInfoList      = {};
    std::map<xCountryId, xDE_LocalRelayInfListEntry>           RelayInfoListByCountry = {};
    xRegionServiceAbstract *                                   RegionService          = nullptr;
    //
    struct xAudit {
        size_t InvalidRelayServerAddress = 0;
    } Audit = {};
};
