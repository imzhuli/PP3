#include "./address_challenge.hpp"

#include "../../pp_protocol/command.hpp"
#include "../../pp_protocol/p_address_challenge.hpp"

#include <pp_common/service_runtime.hpp>

static void OnUdpPacket(const xUdpServiceChannelHandle & Handle, xPacketCommandId CommandId, xPacketRequestId RequestId, ubyte * Payload, size_t DataSize) {
    DEBUG_LOG("CommandId=%" PRIx32 "", CommandId);

    if (CommandId == Cmd_DV_RL_AddressChallenge) {
        auto Req = xPP_AddressChallenge();
        if (!Req.Deserialize(Payload, DataSize)) {
            DEBUG_LOG("invalid protocol:\n%s", HexShow(Payload, DataSize).c_str());
            return;
        }
        auto RA = Handle.GetRemoteAddress();
        DEBUG_LOG("AddressChallenge from %s, data=%s", RA.IpToString().c_str(), std::string(Req.Message).c_str());

        auto Resp       = xPP_AddressChallengeResp();
        Resp.AddressKey = "World!~";
        Handle.PostMessage(Cmd_DV_RL_AddressChallengeResp, 0, Resp);
    }
}

xAddressChallengeService::xAddressChallengeService(const std::vector<xel::xNetAddress> & BindAddressList) {
    X_RUNTIME_ASSERT(ServiceRunState);
    X_RUNTIME_ASSERT(BindAddressList.size());
    bool HasError = false;
    for (auto & BA : BindAddressList) {
        auto S = new xUdpService();
        if (!S->Init(ServiceIoContext, BA)) {
            delete S;
            HasError = true;
            break;
        }
        S->OnPacket = &OnUdpPacket;
        UdpServiceList.push_back(S);
    }
    if (HasError) {
        for (auto S : UdpServiceList) {
            S->Clean();
            delete S;
        }
        Reset(UdpServiceList);
        return;
    }

    SetRaiiReady();
}

xAddressChallengeService::~xAddressChallengeService() {
    if (!IsRaiiReady()) {
        return;
    }
    for (auto S : UdpServiceList) {
        S->Clean();
        delete S;
    }
    Reset(UdpServiceList);
}
