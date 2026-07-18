#include "./rdm.hpp"

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

    RelayEntry.OnClientKeepAlive = Delegate(&xRelayDispatcherMaster::OnRelayEntryKeepAlive, this);
    RelayEntry.OnClientClean     = Delegate(&xRelayDispatcherMaster::OnRelayEntryClean, this);
    RelayEntry.OnClientPacket    = Delegate(&xRelayDispatcherMaster::OnRelayEntryPacket, this);

    SetRaiiReady();
}

xRelayDispatcherMaster::~xRelayDispatcherMaster() {
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
    // TODO: broadcast heartbeat.
}

void xRelayDispatcherMaster::OnRelayEntryClean(const xTcpServiceClientConnectionHandle & Handle) {
    auto Context = static_cast<xRelayEntryContext *>(Handle->UserContext.P);
    if (!Context) {
        return;
    }
    assert(Context->ServerId);
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
        return;
    }
    Handle.Kill();
    return;
}

void xRelayDispatcherMaster::OnDispatcherEntryClean(const xTcpServiceClientConnectionHandle & Handle) {
    auto Context = static_cast<xDispatcherEntryContext *>(Handle->UserContext.P);
    if (!Context) {
        return;
    }
    assert(Context->ContextId);
    DispatcherEntryContextPool.Release(Context->ContextId);
}

bool xRelayDispatcherMaster::OnDispatcherEntryPacket(const xTcpServiceClientConnectionHandle & Handle, xPacketCommandId CmdId, xPacketRequestId ReqId, ubyte * Payload, size_t PayloadSize) {
    auto OldContext = static_cast<xDispatcherEntryContext *>(Handle->UserContext.P);
    if (OldContext) {
        DEBUG_LOG("duplicated request");
        return false;
    }

    return false;
}
