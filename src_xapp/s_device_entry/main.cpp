#include <iostream>
#include <lib_component/address_challenge/address_challenge.hpp>
#include <lib_component/relay/relay_info_observer.hpp>
#include <lib_component/small_server_list_downloader.hpp>
#include <pp_common/service_runtime.hpp>

static auto AddressChallengeBindAddressList = std::string();
static auto ServerIdServerAddress           = xel::xNetAddress();

static auto SmallServerListDownloader = xSmallServerListDownloader();

int main(int argc, char ** argv) {

    X_VAR xServiceEnvironmentGuard(argc, argv);
    auto  CL = ServiceEnvironment.LoadConfig();
    CL.Require(AddressChallengeBindAddressList, "AddressChallengeBindAddressList");
    CL.Require(ServerIdServerAddress, "ServerIdServerAddress");

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

    auto RelayInfoObserver = std::make_unique<xRelayInfoObserver>();
    X_RUNTIME_ASSERT(xRaii::IsReady(*RelayInfoObserver));

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
