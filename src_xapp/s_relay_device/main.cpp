#include <lib_component/small_server_list_downloader.hpp>
#include <pp_common/service_runtime.hpp>
#include <pp_protocol/command.hpp>
#include <pp_protocol/p_relay_register.hpp>

// config
static auto SmallServerListAddress   = xNetAddress();
static auto DeviceEntryAddress       = xNetAddress();
static auto ProxyEntryAddress        = xNetAddress();
// export values
static auto ExportDeviceEntryAddress = xNetAddress();
static auto ExportProxyEntryAddress  = xNetAddress();

// global objects
static auto SmallServerListDownloader = xSmallServerListDownloader();
static auto RelayDispatcherMaster     = xTcpClientWrapper();

int main(int argc, char ** argv) {
    X_VAR xServiceEnvironmentGuard(argc, argv);
    auto  CL = ServiceEnvironment.LoadConfig();
    CL.Require(SmallServerListAddress, "SmallServerListAddress");
    CL.Require(DeviceEntryAddress, "DeviceEntryAddress");
    CL.Require(ProxyEntryAddress, "ProxyEntryAddress");
    CL.Require(ExportDeviceEntryAddress, "ExportDeviceEntryAddress");
    CL.Require(ExportProxyEntryAddress, "ExportProxyEntryAddress");

    X_RUNTIME_ASSERT(SmallServerListAddress && SmallServerListAddress.Port);
    X_RUNTIME_ASSERT(DeviceEntryAddress && DeviceEntryAddress.Port);
    X_RUNTIME_ASSERT(ProxyEntryAddress && ProxyEntryAddress.Port);
    X_RUNTIME_ASSERT(ExportDeviceEntryAddress && ExportDeviceEntryAddress.Port);
    X_RUNTIME_ASSERT(ExportProxyEntryAddress && ExportProxyEntryAddress.Port);

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

    RelayDispatcherMaster.OnServerConnected = [] {
        auto Register                    = xPP_RelayRegister();
        Register.ExportDeviceSideAddress = DeviceEntryAddress;
        Register.ExportProxySideAddrfess = ProxyEntryAddress;
        RelayDispatcherMaster.PostMessage(Cmd_RelayInfoRegister, 0, Register);
    };
    RelayDispatcherMaster.OnServerPacket = [](xPacketCommandId CommandId, xPacketRequestId RequestId, ubyte * PayloadPtr, size_t PayloadSize) -> bool {
        if (CommandId != Cmd_RelayInfoRegisterResp) {
            return false;
        }
        auto Result = xPP_RelayRegisterResp();
        if (!Result.Deserialize(PayloadPtr, PayloadSize)) {
            Logger->E("invalid protocol");
            return false;
        }
        AuditLogger->I("Update server id: %" PRIx64 "", Result.ServerId);
        if (!Result.ServerId) {
            Logger->E("new server id refused");
            return false;
        }
        return true;
    };

    while (ServiceRunState) {
        ServiceUpdateOnce(SmallServerListDownloader, RelayDispatcherMaster);
    }

    return 0;
}
