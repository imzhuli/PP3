#include "./local_relay_info_manager.hpp"

#include <const/ppp_const.hpp>
#include <iostream>
#include <lib_component/address_challenge/address_challenge.hpp>
#include <lib_component/region/region_mmdb.hpp>
#include <lib_component/relay/relay_info_observer.hpp>
#include <lib_component/small_server_list_downloader.hpp>
#include <pp_common/network.hpp>
#include <pp_common/service_runtime.hpp>
#include <pp_protocol/command.hpp>
#include <pp_protocol/p_device_challenge.hpp>

static auto AddressChallengeBindAddressList = std::string();
static auto ServerIdServerAddress           = xel::xNetAddress();
static auto DeviceChallengeBindAddress      = xel::xNetAddress();
static auto MmdbFilename                    = std::string();
static auto StartServiceDelay               = 6 * 60'000;

static auto SmallServerListDownloader = xSmallServerListDownloader();
static auto DeviceChallengeService    = xUdpService();

int main(int argc, char ** argv) {
    X_VAR xServiceEnvironmentGuard(argc, argv);
    auto  CL = ServiceEnvironment.LoadConfig();
    CL.Require(AddressChallengeBindAddressList, "AddressChallengeBindAddressList");
    CL.Require(ServerIdServerAddress, "ServerIdServerAddress");
    CL.Require(DeviceChallengeBindAddress, "DeviceChallengeBindAddress");
    CL.Require(MmdbFilename, "MmdbFilename");
    CL.Optional(StartServiceDelay, "StartServiceDelay", DEVICE_ENTRY_DEFAULT_INIT_DELAY_MS);

    auto Lines     = xel::Split(AddressChallengeBindAddressList, ",");
    auto Addresses = std::vector<xNetAddress>();
    for (auto & L : Lines) {
        L = Trim(L);
        if (L.empty()) {
            continue;
        }
        auto A = xel::xNetAddress::Parse(Trim(L));
        if (!A || !A.Port) {
            Logger->F("InvalidAddressLine: %s", L.c_str());
        }
        Logger->I("Add challenge address:%s", A.ToString().c_str());
        Addresses.push_back(A);
    }
    auto AddressChallengeService = xAddressChallengeService(Steal(Addresses));

    auto RegionService = std::make_unique<xRegionServiceMmdb>(MmdbFilename.c_str());
    SERVICE_RUNTIME_ASSERT(xRaii::IsReady(*RegionService));

    auto LocalRelayInfoManager = std::make_unique<xDE_LocalRelayInfoManager>();
    SERVICE_RUNTIME_ASSERT(xRaii::IsReady(*LocalRelayInfoManager));
    LocalRelayInfoManager->SetRegionService(RegionService.get());

    auto RelayInfoObserver = std::make_unique<xRelayInfoObserver>();
    SERVICE_RUNTIME_ASSERT(xRaii::IsReady(*RelayInfoObserver));

    X_RESOURCE_GUARD_ASSERTED(DeviceChallengeService, ServiceIoContext, DeviceChallengeBindAddress);
    DeviceChallengeService.OnPacket = [&](const xUdpServiceChannelHandle & Handle, xPacketCommandId CmdId, xPacketRequestId ReqId, ubyte * Payload, size_t PayloadSize) {
        if (CmdId == Cmd_DV_CC_Challenge) {

            DEBUG_LOG("Cmd_DV_CC_Challenge");
            auto Req = xPP_DeviceChallenge();
            if (!Req.Deserialize(Payload, PayloadSize)) {
                DEBUG_LOG("invalid protocol");
                return;
            }
            auto DeviceAddress = xNetAddress();
            if (Req.AddressKey4.size()) {
                DeviceAddress = DecryptAddressKey(Req.AddressKey4);
                DEBUG_LOG("DeviceAddress4: %s", DeviceAddress.ToString().c_str());
            } else if (Req.AddressKey6.size()) {
                DeviceAddress = DecryptAddressKey(Req.AddressKey6);
                DEBUG_LOG("DeviceAddress6: %s", DeviceAddress.ToString().c_str());
            }
            if (!DeviceAddress) {
                DEBUG_LOG("invalid device key");
                return;
            }
            auto RelayAddress = LocalRelayInfoManager->GetRelayServerByDeviceIp(DeviceAddress);
            auto Resp         = xPP_DeviceChallengeResp();
            if (RelayAddress) {
                Resp.Accepted     = true;
                Resp.RelayAddress = RelayAddress;
                DEBUG_LOG("device accepted: relay address=%s", RelayAddress.ToString().c_str());
            } else {
                Resp.BanVersionTimeMS = CLIENT_CHALLENGE_RETRY_TIMEOUT_MS;
                DEBUG_LOG("device refused: timeout = %zi", size_t(Resp.BanVersionTimeMS));
            }
            Handle.PostMessage(Cmd_DV_CC_ChallengeResp, 0, Resp);
            return;
        }
    };

    RelayInfoObserver->OnRelayUpdated = [&](const auto & RelayInfo) {
        DEBUG_LOG(
            "NewRelay: %" PRIx64 ", DeviceAddress=%s, ProxyAddress=%s",
            RelayInfo.ServerId, RelayInfo.ExportDeviceEntryAddress.ToString().c_str(), RelayInfo.ExportProxyEntryAddress.ToString().c_str()
        );
        auto LocalRelayServerInfo = xDE_LocalRelayInfoBase{
            .ServerId                 = RelayInfo.ServerId,
            .ExportDeviceEntryAddress = RelayInfo.ExportDeviceEntryAddress,
        };
        LocalRelayInfoManager->UpdateRelayServerInfo(LocalRelayServerInfo);
    };
    RelayInfoObserver->OnRelayDropped = [&](const auto & RelayInfo) {
        DEBUG_LOG(
            "DropRelay: %" PRIx64 ", DeviceAddress=%s, ProxyAddress=%s",
            RelayInfo.ServerId, RelayInfo.ExportDeviceEntryAddress.ToString().c_str(), RelayInfo.ExportProxyEntryAddress.ToString().c_str()
        );
        LocalRelayInfoManager->RemoveRelayServerInfo(RelayInfo.ServerId);
    };
    LocalRelayInfoManager->SetRegionService(RegionService.get());

    X_RESOURCE_GUARD_ASSERTED(SmallServerListDownloader, ServerIdServerAddress);
    SmallServerListDownloader.EnableServerGroup(ST_RELAY_DISPATCHER_SLAVE);

    SmallServerListDownloader.OnServerListUpdated = [&](xServerGroup ServerGroup, const xServerInfo * ServerList, size_t ServerListSize, uint64_t VersionTimestampMS) {
        if (ServerGroup == ST_RELAY_DISPATCHER_SLAVE) {
            RelayInfoObserver->UpdateServerList(ServerList, ServerListSize);
        }
    };

    while (ServiceRunState) {
        ServiceUpdateOnce(SmallServerListDownloader, *RelayInfoObserver);
    }

    return 0;
}
