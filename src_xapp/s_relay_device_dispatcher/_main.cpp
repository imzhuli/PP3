#include <lib_component/server_id_client.hpp>
#include <pp_common/service_runtime.hpp>

static auto SmallServerListServer     = xNetAddress();
static auto ProducerBindAddress       = xNetAddress();
static auto ObserverBindAddress       = xNetAddress();
static auto ExportProducerBindAddress = xNetAddress();
static auto ExportObserverBindAddress = xNetAddress();

static auto ProducerServerIdCleint = xServerIdClient();
static auto ObserverServerIdCleint = xServerIdClient();
static auto ProducerService        = xTcpService();
static auto DispatcherService      = xTcpService();

void OnProducerConnected(const xTcpServiceClientConnectionHandle & Handle) {
    DEBUG_LOG("HandleId=%" PRIx64 "", Handle.GetConnectionId());
}

void OnObserverConnected(const xTcpServiceClientConnectionHandle & Handle) {
    DEBUG_LOG("HandleId=%" PRIx64 "", Handle.GetConnectionId());
}

int main(int argc, char ** argv) {
    X_VAR xServiceEnvironmentGuard(argc, argv);
    auto  CL = ServiceEnvironment.LoadConfig();

    CL.Require(SmallServerListServer, "SmallServerListServer");
    CL.Require(ProducerBindAddress, "ProducerBindAddress");
    CL.Require(ObserverBindAddress, "ObserverBindAddress");
    CL.Require(ExportProducerBindAddress, "ExportProducerBindAddress");
    CL.Require(ExportObserverBindAddress, "ExportObserverBindAddress");

    auto ProducerServerIdCleintOptions = xServerIdClientOptions{
        .ServerGroup   = ST_DEVICE_DISPATCHER_PRODUCER_ENTRY,
        .ExportAddress = ExportProducerBindAddress,
    };
    X_RESOURCE_GUARD_ASSERTED(ProducerServerIdCleint, ServiceIoContext, ProducerServerIdCleintOptions, SmallServerListServer);
    auto ObserverServerIdCleintOptions = xServerIdClientOptions{
        .ServerGroup   = ST_DEVICE_DISPATCHER_OBSERVER_ENTRY,
        .ExportAddress = ExportObserverBindAddress,
    };
    X_RESOURCE_GUARD_ASSERTED(ObserverServerIdCleint, ServiceIoContext, ObserverServerIdCleintOptions, SmallServerListServer);

    X_RESOURCE_GUARD_ASSERTED(ProducerService, ServiceIoContext, ProducerBindAddress);
    X_RESOURCE_GUARD_ASSERTED(DispatcherService, ServiceIoContext, ObserverBindAddress);

    ProducerService.OnClientConnected   = OnProducerConnected;
    DispatcherService.OnClientConnected = OnObserverConnected;

    while (ServiceRunState) {
        ServiceUpdateOnce(
            ProducerServerIdCleint, ObserverServerIdCleint,
            ProducerService, DispatcherService
        );
    }

    return 0;
}
