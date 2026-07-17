#include "../ppp/lib_component/server_id_client.hpp"

#include <pp_common/service_runtime.hpp>

// config
static auto SmallServerListServer      = xNetAddress();
static auto RelayEntryBindAddress      = xNetAddress();
static auto DispatcherEntryBindAddress = xNetAddress();

// service
static auto RelayEntry      = xTcpService();
static auto DispatcherEntry = xTcpService();

static auto RelayEntryServerIdCleint      = xServerIdClient();
static auto DispatcherEntryServerIdCleint = xServerIdClient();

int main(int argc, char ** argv) {
    X_VAR xServiceEnvironmentGuard(argc, argv);
    auto  CL = ServiceEnvironment.LoadConfig();

    CL.Require(SmallServerListServer, "SmallServerListServer");
    CL.Require(RelayEntryBindAddress, "RelayEntryBindAddress");
    CL.Require(DispatcherEntryBindAddress, "DispatcherEntryBindAddress");

    X_RESOURCE_GUARD_ASSERTED(RelayEntry, ServiceIoContext, RelayEntryBindAddress);
    X_RESOURCE_GUARD_ASSERTED(DispatcherEntry, ServiceIoContext, DispatcherEntryBindAddress);

    auto RelayEntryServerIdClientOptions = xServerIdClientOptions{
        .ServerGroup   = ST_RELAY_REGISTER,
        .ExportAddress = RelayEntryBindAddress,
    };
    X_RESOURCE_GUARD_ASSERTED(RelayEntryServerIdCleint, ServiceIoContext, RelayEntryServerIdClientOptions, SmallServerListServer, std::string{});
    RelayEntryServerIdCleint.OnServerIdUpdated = [](uint64_t NewId) { Logger->I("Update RelayEntryServerIdCleint: new Id=%" PRIu64 "", NewId); };

    auto DispatcherEntryServerIdClientOptions = xServerIdClientOptions{
        .ServerGroup   = ST_RELAY_DISPATCHER_MASTER,
        .ExportAddress = RelayEntryBindAddress,
    };
    X_RESOURCE_GUARD_ASSERTED(DispatcherEntryServerIdCleint, ServiceIoContext, DispatcherEntryServerIdClientOptions, SmallServerListServer, std::string{});
    DispatcherEntryServerIdCleint.OnServerIdUpdated = [](uint64_t NewId) { Logger->I("Update DispatcherEntryServerIdCleint: new Id=%" PRIu64 "", NewId); };

    while (ServiceRunState) {
        ServiceUpdateOnce(RelayEntry, DispatcherEntry, RelayEntryServerIdCleint, DispatcherEntryServerIdCleint);
    }

    return 0;
}
