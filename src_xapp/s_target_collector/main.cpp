#include "./reporter.hpp"

#include <lib_component/server_id_client.hpp>
#include <lib_component/small_server_list_downloader.hpp>
#include <pp_common/service_runtime.hpp>
#include <pp_protocol/command.hpp>
#include <pp_protocol/p_target_collect.hpp>

#ifndef NDEBUG
static constexpr const auto OUTPUT_AUDIT_TIMEOUT_MS = 1min;
#else
static constexpr const auto OUTPUT_AUDIT_TIMEOUT_MS = 15min;
#endif

// config
static auto BindAddress           = xNetAddress();
static auto ExportAddress         = xNetAddress();
static auto ServerIdServerAddress = xNetAddress();

// service
static auto CollectorService = xUdpService();
static auto ServerIdClient   = xServerIdClient();
static auto Reporter         = xTargetCollectReporter();

void OnUdpPacket(const xUdpServiceChannelHandle & Handle, xPacketCommandId CmdId, xPacketRequestId RequestId, ubyte * Payload, size_t PayloadSize) {
    if (CmdId == Cmd_TargetReport) {
        auto Req = xPP_TargetCollect();
        if (!Req.Deserialize(Payload, PayloadSize)) {
            DEBUG_LOG("invalid protocol of xPP_TargetCollect");
            return;
        }
        DEBUG_LOG("PostCollect:%" PRIu64 ", %s, %s, %zi", Req.GlobalAuthId, Req.TargetAddress.ToString().c_str(), std::string(Req.TargetHostView).c_str(), (size_t)Req.Count);
        Reporter.PostTargetCollect(Req.GlobalAuthId, Req.TargetAddress, Req.TargetHostView, Req.Count);
    }
}

int main(int argc, char ** argv) {
    X_VAR xServiceEnvironmentGuard(argc, argv);
    auto  CL = ServiceEnvironment.LoadConfig();
    CL.Require(BindAddress, "BindAddress");
    CL.Require(ExportAddress, "ExportAddress");
    CL.Require(ServerIdServerAddress, "ServerIdServerAddress");
    X_RESOURCE_GUARD_ASSERTED(Reporter, ServiceEnvironment.DefaultConfigFilePath);

    auto ServerIdClientOptions = xServerIdClientOptions{
        .ServerGroup      = ST_TARGET_COLLECTOR,
        .PreviousServerId = 0,
        .ExportAddress    = ExportAddress,
    };
    auto ServerIdFilename = ServiceEnvironment.DefaultLocalServerIdFilePath;
    X_RESOURCE_GUARD_ASSERTED(ServerIdClient, ServiceIoContext, ServerIdClientOptions, ServerIdServerAddress, ServerIdFilename);
    X_RESOURCE_GUARD_ASSERTED(CollectorService, ServiceIoContext, BindAddress);

    CollectorService.OnPacket = OnUdpPacket;

    auto AuditOutputTimer = xTimer();
    while (ServiceRunState) {
        ServiceUpdateOnce(ServerIdClient, Reporter);
        if (AuditOutputTimer.TestAndTag(OUTPUT_AUDIT_TIMEOUT_MS)) {
            AuditLogger->I("%s", Reporter.GetAuditOutput().c_str());
        }
    };

    return 0;
}
