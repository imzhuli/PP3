#include "./rdm.hpp"

#include <lib_component/server_id_client.hpp>
#include <pp_common/service_runtime.hpp>

// config
static auto SmallServerListServer      = xNetAddress();
static auto RelayEntryBindAddress      = xNetAddress();
static auto DispatcherEntryBindAddress = xNetAddress();

// service
static auto RelayEntryServerIdCleint      = xServerIdClient();
static auto DispatcherEntryServerIdCleint = xServerIdClient();

int main(int argc, char ** argv) {
    X_VAR xServiceEnvironmentGuard(argc, argv);
    auto  CL = ServiceEnvironment.LoadConfig();

    CL.Require(SmallServerListServer, "SmallServerListServer");
    CL.Require(RelayEntryBindAddress, "RelayEntryBindAddress");
    CL.Require(DispatcherEntryBindAddress, "DispatcherEntryBindAddress");

    auto RDM = std::make_unique<xRelayDispatcherMaster>(RelayEntryBindAddress, DispatcherEntryBindAddress);
    X_RUNTIME_ASSERT(xRaii::IsReady(*RDM));

    auto RelayEntryServerIdClientOptions = xServerIdClientOptions{
        .ServerGroup   = ST_RELAY_REGISTER,
        .ExportAddress = RelayEntryBindAddress,
    };
    X_RESOURCE_GUARD_ASSERTED(RelayEntryServerIdCleint, ServiceIoContext, RelayEntryServerIdClientOptions, SmallServerListServer, std::string{});
    RelayEntryServerIdCleint.OnServerIdUpdated = [](uint64_t NewId) { Logger->I("Update RelayEntryServerIdCleint: new Id=%" PRIx64 "", NewId); };

    auto DispatcherEntryServerIdClientOptions = xServerIdClientOptions{
        .ServerGroup   = ST_RELAY_DISPATCHER_MASTER,
        .ExportAddress = DispatcherEntryBindAddress,
    };
    X_RESOURCE_GUARD_ASSERTED(DispatcherEntryServerIdCleint, ServiceIoContext, DispatcherEntryServerIdClientOptions, SmallServerListServer, std::string{});
    DispatcherEntryServerIdCleint.OnServerIdUpdated = [](uint64_t NewId) { Logger->I("Update DispatcherEntryServerIdCleint: new Id=%" PRIx64 "", NewId); };

    while (ServiceRunState) {
        ServiceUpdateOnce(*RDM, RelayEntryServerIdCleint, DispatcherEntryServerIdCleint);
    }

    return 0;
}
