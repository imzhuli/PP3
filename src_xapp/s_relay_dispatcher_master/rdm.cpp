#include "./rdm.hpp"

#include <cmath>
#include <pp_common/service_runtime.hpp>
#include <pp_protocol/command.hpp>
#include <pp_protocol/p_relay_register.hpp>

xRelayDispatcherMaster::xRelayDispatcherMaster(const xel::xNetAddress & RelayEntrydAddress, const xNetAddress & DispatcherEntryAddress) {
    auto ICP = ServiceIoContext;

    if (!RelayEntryContextPool.Init(20'0000)) {
        return;
    }
    if (!DispatcherEntryContextPool.Init(MAX_SMALL_SERVER_LIST_SIZE)) {
        RelayEntryContextPool.Clean();
        return;
    }
    if (!RelayEntry.Init(ICP, RelayEntrydAddress)) {
        DispatcherEntryContextPool.Clean();
        RelayEntryContextPool.Clean();
        return;
    }
    if (!DispatcherEntry.Init(ICP, DispatcherEntryAddress)) {
        RelayEntry.Clean();
        DispatcherEntryContextPool.Clean();
        RelayEntryContextPool.Clean();
        return;
    }

    RelayEntry.SetMaxConnectionIdleTimeoutMS(3 * 60'000);
    RelayEntry.OnClientKeepAlive = Delegate(&xRelayDispatcherMaster::OnRelayEntryKeepAlive, this);
    RelayEntry.OnClientClean     = Delegate(&xRelayDispatcherMaster::OnRelayEntryClean, this);
    RelayEntry.OnClientPacket    = Delegate(&xRelayDispatcherMaster::OnRelayEntryPacket, this);

    DispatcherEntry.OnClientKeepAlive = Delegate(&xRelayDispatcherMaster::OnDispatcherEntryKeepAlive, this);
    DispatcherEntry.OnClientClean     = Delegate(&xRelayDispatcherMaster::OnDispatcherEntryClean, this);
    DispatcherEntry.OnClientPacket    = Delegate(&xRelayDispatcherMaster::OnDispatcherEntryPacket, this);

    std::random_device rd;
    RandomGeneratorHolder.CreateValue(rd());

    SetRaiiReady();
}

xRelayDispatcherMaster::~xRelayDispatcherMaster() {
    RandomGeneratorHolder.Destroy();
    DispatcherEntry.Clean();
    RelayEntry.Clean();
    DispatcherEntryContextPool.Clean();
    RelayEntryContextPool.Clean();
}

void xRelayDispatcherMaster::Tick(uint64_t NowMS) {
    TickAll(NowMS, RelayEntry, DispatcherEntry);
}

//////////////////////

void xRelayDispatcherMaster::OnRelayEntryKeepAlive(const xTcpServiceClientConnectionHandle & Handle) {
    auto Context = static_cast<xRelayEntryContext *>(Handle->UserContext.P);
    if (!Context) {
        DEBUG_LOG("invalid client relay entry");
        Handle.Kill();
        return;
    }
    DispatchRelayHeartbeat(*Context);
}

void xRelayDispatcherMaster::OnRelayEntryClean(const xTcpServiceClientConnectionHandle & Handle) {
    auto Context = static_cast<xRelayEntryContext *>(Handle->UserContext.P);
    if (!Context) {
        return;
    }
    assert(Context->ServerId);
    DEBUG_LOG("Release relay, serverId=%" PRIx64 "", Context->ServerId);
    RelayEntryContextPool.Release(Context->ServerId);
}

bool xRelayDispatcherMaster::OnRelayEntryPacket(const xTcpServiceClientConnectionHandle & Handle, xPacketCommandId CmdId, xPacketRequestId ReqId, ubyte * Payload, size_t PayloadSize) {
    auto OldContext = static_cast<xRelayEntryContext *>(Handle->UserContext.P);
    if (OldContext) {
        DEBUG_LOG("duplicated request");
        return false;
    }
    if (CmdId != Cmd_RelayInfoRegister) {
        DEBUG_LOG("invalid command");
        return false;
    }
    auto Register = xPP_RelayRegister();
    if (!Register.Deserialize(Payload, PayloadSize)) {
        DEBUG_LOG("invalid protocol");
        return false;
    }

    auto NewContextId = RelayEntryContextPool.Acquire();
    if (NewContextId) {
        auto & Context             = RelayEntryContextPool[NewContextId];
        Context.ServerId           = NewContextId;
        Context.RelayServerType    = Register.RelayServerType;
        Context.DeviceEntryAddress = Register.ExportDeviceSideAddress;
        Context.ProxyEntryAddress  = Register.ExportProxySideAddrfess;
        Handle->UserContext.P      = &Context;
        DEBUG_LOG("new Relay connection, ServerId=%" PRIx64 ", Type=%u DeviceEntry=%s, ProxyEntry=%s",  //
                  Context.ServerId, (unsigned)Context.RelayServerType, Context.DeviceEntryAddress.ToString().c_str(), Context.ProxyEntryAddress.ToString().c_str());
        DispatchRelayHeartbeat(Context);
    }
    auto Resp     = xPP_RelayRegisterResp();
    Resp.ServerId = NewContextId;
    Handle.PostMessage(Cmd_RelayInfoRegisterResp, 0, Resp);

    return true;
}

///////

void xRelayDispatcherMaster::OnDispatcherEntryKeepAlive(const xTcpServiceClientConnectionHandle & Handle) {
    auto Context = static_cast<xDispatcherEntryContext *>(Handle->UserContext.P);
    if (!Context) {
        DEBUG_LOG("invalid diaptcher connection");
        Handle.Kill();
        return;
    }
    return;
}

void xRelayDispatcherMaster::OnDispatcherEntryClean(const xTcpServiceClientConnectionHandle & Handle) {
    auto Context = static_cast<xDispatcherEntryContext *>(Handle->UserContext.P);
    if (!Context) {
        return;
    }
    assert(Context->ContextId);
    DEBUG_LOG("ReleaseDispatcherSlave: %" PRIx64 "", Context->ContextId);
    DispatcherEntryContextPool.Release(Context->ContextId);
}

bool xRelayDispatcherMaster::OnDispatcherEntryPacket(const xTcpServiceClientConnectionHandle & Handle, xPacketCommandId CmdId, xPacketRequestId ReqId, ubyte * Payload, size_t PayloadSize) {
    auto OldContext = static_cast<xDispatcherEntryContext *>(Handle->UserContext.P);
    if (OldContext) {
        DEBUG_LOG("duplicated request");
        return false;
    }

    if (CmdId != Cmd_RelayDispatcherSlaveRegister) {
        DEBUG_LOG("invalid command id");
        return false;
    }

    auto Register = xPP_RelayDispatcherSlaveRegister();
    if (!Register.Deserialize(Payload, PayloadSize)) {
        DEBUG_LOG("invalid protocol");
        return false;
    }

    auto NowMS = xel::GetTimestampMS();
    if (std::abs(SignedDiff(NowMS, Register.TimestampMS)) > 10 * 60'000) {
        DEBUG_LOG("invalid timestamp");
        return false;
    }

    auto NewContextId = DispatcherEntryContextPool.Acquire();
    if (NewContextId) {
        auto & Context           = DispatcherEntryContextPool[NewContextId];
        Context.ContextId        = NewContextId;
        Context.ConnectionHandle = Handle;

        DispatcherEntryList.AddTail(Context);
        Handle->UserContext.P = &Context;
        DEBUG_LOG("AcceptDispatcherSlave: %" PRIx64 "", NewContextId);
    }
    auto Resp     = xPP_RelayDispatcherSlaveRegisterResp();
    Resp.Accepted = NewContextId;
    Handle.PostMessage(Cmd_RelayDispatcherSlaveRegisterResp, 0, Resp);

    return true;
}

void xRelayDispatcherMaster::DispatchRelayHeartbeat(xRelayEntryContext & RelayContext) {
    if (!RelayContext.HeartbeatDataSize) {  // build heartbeat packet buffer
        auto RHB                       = xPP_RelayInfoBroadcast();
        RHB.Type                       = RelayContext.RelayServerType;
        RHB.ServerId                   = RelayContext.ServerId;
        RHB.ExportDeviceSideAddress    = RelayContext.DeviceEntryAddress;
        RHB.ExportProxySideAddrfess    = RelayContext.ProxyEntryAddress;
        RelayContext.HeartbeatDataSize = WriteMessage(RelayContext.HeartbeatBuffer, Cmd_RelayHeartbeatBroadcast, RHB);

        DEBUG_LOG("Build RelayHeartbeat buffer: result_size=%zi", RelayContext.HeartbeatDataSize);
    }
    DispatcherEntryList.ForEach([&](const xDispatcherEntryContext & DEC) {
        DEC.ConnectionHandle.PostData(RelayContext.HeartbeatBuffer, RelayContext.HeartbeatDataSize);
    });
}
