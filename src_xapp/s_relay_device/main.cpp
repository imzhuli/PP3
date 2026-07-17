#include "../ppp/lib_component/small_server_list_downloader.hpp"

#include <pp_common/service_runtime.hpp>

static auto SmallServerListAddress = xNetAddress();
static auto DeviceEntryAddress     = xNetAddress();
static auto ProxyEntryAddress      = xNetAddress();

static auto SmallServerListDownloader = xSmallServerListDownloader();
static auto RelayDispatcherMaster     = xTcpClientWrapper();

int main(int argc, char ** argv) {
    X_VAR xServiceEnvironmentGuard(argc, argv);
    auto  CL = ServiceEnvironment.LoadConfig();
    CL.Require(SmallServerListAddress, "SmallServerListAddress");
    CL.Require(DeviceEntryAddress, "DeviceEntryAddress");
    CL.Require(ProxyEntryAddress, "ProxyEntryAddress");

    X_RESOURCE_GUARD_ASSERTED(RelayDispatcherMaster, ServiceIoContext);
    X_RESOURCE_GUARD_ASSERTED(SmallServerListDownloader, SmallServerListAddress);

    SmallServerListDownloader.EnableServerGroup(ST_RELAY_REGISTER);
    SmallServerListDownloader.OnServerListUpdated = [](xServerGroup ServerGroup, const xServerInfo * ServerList, size_t ServerListSize, uint64_t VersionTimestampMS) {
        if (ServerListSize != 1) {
            AuditLogger->E("Invalid ST_RELAY_REGISTER count, it should always be ONE !!!");
            return;
        }
        RelayDispatcherMaster.UpdateTarget(ServerList[0].Address);
    };

    while (ServiceRunState) {
        ServiceUpdateOnce(SmallServerListDownloader, RelayDispatcherMaster);
    }

    return 0;
}
