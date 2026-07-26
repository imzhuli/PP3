#include "./relay_info_observer.hpp"

#include "../../pp_protocol/command.hpp"
#include "../../pp_protocol/p_relay_register.hpp"

#include <pp_common/service_runtime.hpp>

bool xRelayInfoObserver::OnPacket(const xTcpClientPoolConnectionHandle & Handle, xPacketCommandId CmdId, xPacketRequestId ReqId, ubyte * Payload, size_t PayloadSize) {
    if (CmdId == Cmd_RelayHeartbeatBroadcast) {
        auto B = xPP_RelayInfoBroadcast();
        if (!B.Deserialize(Payload, PayloadSize)) {
            DEBUG_LOG("invalid protocol");
            return false;
        }
        DEBUG_LOG("update relay server: %" PRIx64 ", Type=%u DeviceSideAddress=%s, ProxySideAddress=%s", B.ServerId, (unsigned)B.Type, B.ExportDeviceSideAddress.ToString().c_str(), B.ExportProxySideAddrfess.ToString().c_str());
        return true;
    }
    if (CmdId == Cmd_RelayHeartbeatLost) {
        auto B = xPP_RelayInfoLost();
        if (!B.Deserialize(Payload, PayloadSize)) {
            DEBUG_LOG("invalid protocol");
            return false;
        }
        DEBUG_LOG("Lost relay server: %" PRIx64 "", B.ServerId);
        return true;
    }
    return true;
}
