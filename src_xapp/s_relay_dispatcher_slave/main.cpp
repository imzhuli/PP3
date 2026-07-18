#include <lib_component/server_id_client.hpp>
#include <lib_component/small_server_list_downloader.hpp>
#include <pp_common/service_runtime.hpp>
#include <pp_protocol/command.hpp>
#include <pp_protocol/p_relay_register.hpp>

static auto SmallServerListServer       = xNetAddress();
static auto DispatcherBindAddress       = xNetAddress();
static auto ExportDispatcherBindAddress = xNetAddress();

static auto Master                    = xTcpClientWrapper();
static auto ServerIdCleint            = xServerIdClient();
static auto SmallServerListDownloader = xSmallServerListDownloader();

int main(int argc, char ** argv) {
    X_VAR xServiceEnvironmentGuard(argc, argv);
    auto  CL = ServiceEnvironment.LoadConfig();
    CL.Require(SmallServerListServer, "SmallServerListServer");
    CL.Require(DispatcherBindAddress, "DispatcherBindAddress");
    CL.Require(ExportDispatcherBindAddress, "ExportDispatcherBindAddress");

    X_RESOURCE_GUARD_ASSERTED(Master, ServiceIoContext);
    auto ServerIdClientOptions = xServerIdClientOptions{
        .ServerGroup   = ST_RELAY_DISPATCHER_SLAVE,
        .ExportAddress = ExportDispatcherBindAddress,
    };
    X_RESOURCE_GUARD_ASSERTED(ServerIdCleint, ServiceIoContext, ServerIdClientOptions, SmallServerListServer);
    X_RESOURCE_GUARD_ASSERTED(SmallServerListDownloader, SmallServerListServer);

    SmallServerListDownloader.EnableServerGroup(ST_RELAY_DISPATCHER_MASTER);
    SmallServerListDownloader.OnServerListUpdated = [](xServerGroup ServerGroup, const xServerInfo * ServerList, size_t ServerListSize, uint64_t VersionTimestampMS) {
        if (ServerListSize != 1) {
            Logger->E("Invalid ST_RELAY_REGISTER count, it should always be ONE !!!");
            return;
        }
        auto & MasterAddress = ServerList[0].Address;
        if (Master.GetTarget() != MasterAddress) {
            Master.UpdateTarget(MasterAddress);
            Logger->I("Update ST_RELAY_DISPATCHER_MASTER: %s", MasterAddress.ToString().c_str());
        }
    };

    Master.OnServerConnected = [] {
        Logger->I("Master Connected");
        auto Register        = xPP_RelayDispatcherSlaveRegister();
        Register.TimestampMS = xel::GetTimestampMS();
        Master.PostMessage(Cmd_RelayDispatcherSlaveRegister, 0, Register);
    };
    Master.OnServerPacket = [](xPacketCommandId CmdId, xPacketRequestId ReqId, ubyte * Payload, size_t PayloadSize) -> bool {
        if (CmdId != Cmd_RelayDispatcherSlaveRegisterResp) {
            DEBUG_LOG("invalid command id");
            return false;
        }
        auto Resp = xPP_RelayDispatcherSlaveRegisterResp();
        if (!Resp.Deserialize(Payload, PayloadSize)) {
            DEBUG_LOG("invlaid protocol");
            return false;
        }
        if (!Resp.Accepted) {
            DEBUG_LOG("not accepted by master");
            return false;
        }
        DEBUG_LOG("Accepted by master");
        return true;
    };

    while (ServiceRunState) {
        ServiceUpdateOnce(Master, ServerIdCleint, SmallServerListDownloader);
    }

    return 0;
}
