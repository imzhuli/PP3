#include "./relay_info_observer.hpp"

#include "../../pp_protocol/command.hpp"
#include "../../pp_protocol/p_relay_register.hpp"

#include <pp_common/service_runtime.hpp>

xRelayInfoObserver::xRelayInfoObserver() {
    if (!ClientPool.Init(ServiceIoContext, MAX_SMALL_SERVER_LIST_SIZE * 5)) {
        return;
    }
    ClientPool.OnTargetPacket = Delegate(&xRelayInfoObserver::OnPacket, this);
    SetRaiiReady();
}

xRelayInfoObserver::~xRelayInfoObserver() {
    if (!IsRaiiReady()) {
        return;
    }
    ClientPool.Clean();
}

void xRelayInfoObserver::Tick(uint64_t NowMS) {
    ClientPool.Tick(NowMS);
}

void xRelayInfoObserver::UpdateDispatcher(const xServerInfo * ServerInfoList, size_t ServerInfoListSize) {
    auto NewServerArray = std::array<xServerInfo, MAX_SMALL_SERVER_LIST_SIZE>();
    for (auto I = size_t(0); I < ServerInfoListSize; ++I) {
        auto & SI = ServerInfoList[I];
        assert(SI.ServerId);
        auto Index            = ExtractIndexFromServerId(SI.ServerId);
        NewServerArray[Index] = SI;
    }
    for (auto I = size_t(0); I < MAX_SMALL_SERVER_LIST_SIZE; ++I) {
        auto & Old = DispatcherInfoContainer[I];
        auto & New = NewServerArray[I];

        if (Old.ConnectionId) {
            if (X_LIKELY(New == Old.ServerInfo)) {
                DEBUG_LOG("KeepServer: ServerId=%" PRIx64 ", Address=%s", New.ServerId, New.Address.ToString().c_str());
                continue;
            }
            DEBUG_LOG("RemoveServer: ServerId=%" PRIx64 ", Address=%s, ConnectionId=%" PRIx64 "", Old.ServerInfo.ServerId, Old.ServerInfo.Address.ToString().c_str(), Old.ConnectionId);
            ClientPool.RemoveServer(Steal(Old.ConnectionId));
            if (New.ServerId) {
                Old.ConnectionId = ClientPool.AddServer(New.Address);
                Old.ServerInfo   = New;
                DEBUG_LOG("AddServer: ServerId=%" PRIx64 ", Address=%s", New.ServerId, New.Address.ToString().c_str());
            }
        } else {
            if (New.ServerId) {
                X_RUNTIME_ASSERT(Old.ConnectionId = ClientPool.AddServer(New.Address));
                Old.ServerInfo = New;
                DEBUG_LOG("AddServer: ServerId=%" PRIx64 ", Address=%s", New.ServerId, New.Address.ToString().c_str());
            }
        }
    }
}

bool xRelayInfoObserver::OnPacket(const xTcpClientPoolConnectionHandle & Handle, xPacketCommandId CmdId, xPacketRequestId ReqId, ubyte * Payload, size_t PayloadSize) {
    if (CmdId == Cmd_RelayHeartbeatBroadcast) {
        auto B = xPP_RelayInfoBroadcast();
        if (!B.Deserialize(Payload, PayloadSize)) {
            DEBUG_LOG("invalid protocol");
            return false;
        }
        DEBUG_LOG("update relay server: %" PRIx64 ", DeviceSideAddress=%s, ProxySideAddress=%s", B.ServerId, B.ExportDeviceSideAddress.ToString().c_str(), B.ExportProxySideAddrfess.ToString().c_str());
    }

    return true;
}
