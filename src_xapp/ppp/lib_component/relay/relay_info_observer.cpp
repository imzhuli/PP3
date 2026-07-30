#include "./relay_info_observer.hpp"

#include "../../pp_protocol/command.hpp"
#include "../../pp_protocol/p_relay_register.hpp"

#include <pp_common/service_runtime.hpp>

xRelayInfoObserver::xRelayInfoObserver()
    : TcpClient() {
    if (!xRaii::IsReady(TcpClient)) {
        return;
    }
    TcpClient.OnServerPacket = Delegate(&xRelayInfoObserver::OnPacket, this);
    SetRaiiReady();
}

xRelayInfoObserver::~xRelayInfoObserver() {
    if (!IsRaiiReady()) {
        return;
    }
}

bool xRelayInfoObserver::OnPacket(const xTcpClientPoolConnectionHandle & Handle, xPacketCommandId CmdId, xPacketRequestId ReqId, ubyte * Payload, size_t PayloadSize) {
    if (CmdId == Cmd_RelayHeartbeatBroadcast) {
        auto B = xPP_RelayInfoBroadcast();
        if (!B.Deserialize(Payload, PayloadSize) || !B.ServerId) {
            DEBUG_LOG("invalid protocol");
            return false;
        }
        auto ServerIndex = ExtractIndexFromRelayServerId(B.ServerId);
        if (ServerIndex >= MAX_RELAY_SERVER_LIST_SIZE) {
            AuditLogger->E("ServerIndex overflow: %" PRIx32 "", ServerIndex);
            return false;
        }
        auto & RelayInfo = RelayServerList[ServerIndex];
        if (RelayInfo.ServerId != B.ServerId ||
            RelayInfo.ExportDeviceEntryAddress != B.ExportDeviceEntryAddress ||
            RelayInfo.ExportProxyEntryAddress != B.ExportProxyEntryAddress) {
            if (RelayInfo.ServerId) {
                Kill(RelayInfo);
            }
            RelayInfo.ServerId                 = B.ServerId;
            RelayInfo.ExportProxyEntryAddress  = B.ExportProxyEntryAddress;
            RelayInfo.ExportDeviceEntryAddress = B.ExportDeviceEntryAddress;
            // Add new server info node:

            DEBUG_LOG("New RelayServerId: %" PRIx64 "", RelayInfo.ServerId);
            OnRelayUpdated(RelayInfo);
            RelayInfo.LastKeepAliveTimestampMS = LocalTicker();
            RelayInfoTimeoutList.AddTail(RelayInfo);
        } else {
            KeepAlive(RelayInfo);
        }
        return true;
    }
    if (CmdId == Cmd_RelayHeartbeatLost) {
        auto B = xPP_RelayInfoLost();
        if (!B.Deserialize(Payload, PayloadSize) || !B.ServerId) {
            DEBUG_LOG("invalid protocol");
            return false;
        }
        auto ServerIndex = ExtractIndexFromRelayServerId(B.ServerId);
        if (ServerIndex >= MAX_RELAY_SERVER_LIST_SIZE) {
            AuditLogger->E("ServerIndex overflow: %" PRIx32 "", ServerIndex);
            return false;
        }

        auto & ServerInfoNode = RelayServerList[ServerIndex];
        if (ServerInfoNode.ServerId == B.ServerId) {
            Kill(ServerInfoNode);
        }
        return true;
    }
    return true;
}

void xRelayInfoObserver::KeepAlive(xInternalLocalRelayInfo & RelayInfo) {
    assert(RelayInfo.ServerId);
    assert(xListNode::IsLinked(RelayInfo));
    DEBUG_LOG("RelayServerId: %" PRIx64 "", RelayInfo.ServerId);

    RelayInfo.LastKeepAliveTimestampMS = LocalTicker();
    RelayInfoTimeoutList.GrabTail(RelayInfo);
}

void xRelayInfoObserver::Kill(xInternalLocalRelayInfo & RelayInfo) {
    assert(RelayInfo.ServerId);
    DEBUG_LOG("RelayServerId: %" PRIx64 "", RelayInfo.ServerId);

    RelayInfoTimeoutList.Remove(RelayInfo);
    OnRelayDropped(RelayInfo);
    Reset(RelayInfo.ServerId);
}

void xRelayInfoObserver::RemoveTimeoutRelayInfo() {
    auto NowMS = LocalTicker();
    auto Cond  = [KillTimepoint = NowMS - RELAY_HEARTBEAT_TIMEOUT_MS](const xInternalLocalRelayInfo & Info) {
        return Info.LastKeepAliveTimestampMS <= KillTimepoint;
    };
    while (auto P = RelayInfoTimeoutList.PopHead(Cond)) {
        Kill(*P);
    }
}
