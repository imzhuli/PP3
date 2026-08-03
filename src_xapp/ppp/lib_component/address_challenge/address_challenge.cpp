#include "./address_challenge.hpp"

#include "../../pp_protocol/command.hpp"
#include "../../pp_protocol/p_address_challenge.hpp"

#include <pp_common/service_runtime.hpp>
#include <span>

static ubyte EC = 0x00;
static void  EncryptAddressKey(std::span<ubyte> & Output, const xel::xNetAddress & Address) {
    assert(Address && !Address.Port);
    auto B = Output.data();
    auto S = Output.size();
    Reset(Output);
    auto LocalEC = ++EC | 0x37;
    if (Address.Is4()) {
        size_t FinalSize = sizeof(Address.SA4) + 1;
        if (S < FinalSize) {
            return;
        }
        auto W = xel::xStreamWriter(B);
        W.W(LocalEC);
        for (size_t I = 0; I < sizeof(Address.SA4); ++I) {
            LocalEC ^= Address.SA4[I];
            W.W(LocalEC);
        }
        Output = std::span<ubyte>{ B, FinalSize };
    }
    if (Address.Is6()) {
        size_t FinalSize = sizeof(Address.SA6) + 1;
        if (S < FinalSize) {
            return;
        }
        auto W = xel::xStreamWriter(B);
        W.W(LocalEC);
        for (size_t I = 0; I < sizeof(Address.SA6); ++I) {
            LocalEC ^= Address.SA6[I];
            W.W(LocalEC);
        }
        Output = std::span<ubyte>{ B, FinalSize };
    }
}

xNetAddress DecryptAddressKey(const std::string_view & View) {
    auto Origin = xel::HexToStr(View);
    auto Size   = Origin.size();
    DEBUG_LOG("SourceAddressKey: %zi", Origin.size());
    if (Size == sizeof(xNetAddress::SA4) + 1) {  // try ipv4
        auto   NA      = xNetAddress::Make4();
        auto & SA      = NA.SA4;
        auto   LocalEC = Origin[0];
        for (size_t I = 1; I < Size; ++I) {
            SA[I - 1] = LocalEC ^ Origin[I];
            LocalEC   = Origin[I];
        }
        return NA;
    }
    if (Size == sizeof(xNetAddress::SA6) + 1) {  // try ipv6
        auto   NA      = xNetAddress::Make6();
        auto & SA      = NA.SA6;
        auto   LocalEC = Origin[0];
        for (size_t I = 1; I < Size; ++I) {
            SA[I - 1] = LocalEC ^ Origin[I];
            LocalEC   = Origin[I];
        }
        return NA;
    }
    return {};
}

static void OnUdpPacket(const xUdpServiceChannelHandle & Handle, xPacketCommandId CommandId, xPacketRequestId RequestId, ubyte * Payload, size_t DataSize) {
    DEBUG_LOG("CommandId=%" PRIx32 "", CommandId);

    if (CommandId == Cmd_DV_CC_GetAddressKey) {
        auto Req = xPP_AddressChallenge();
        if (!Req.Deserialize(Payload, DataSize)) {
            DEBUG_LOG("invalid protocol:\n%s", HexShow(Payload, DataSize).c_str());
            return;
        }
        auto RA = Handle.GetRemoteAddress();
        DEBUG_LOG("AddressChallenge from %s, data=%s", RA.IpToString().c_str(), std::string(Req.Message).c_str());
        Pass(RA);

        auto  RemoteAddress = Handle.GetRemoteAddress().Ip();
        ubyte Buffer[32];
        auto  Span = std::span<ubyte>(Buffer);
        EncryptAddressKey(Span, RemoteAddress);
        auto AddressKey = StrToHex(Span.data(), Span.size());

        auto Resp       = xPP_AddressChallengeResp();
        Resp.AddressKey = AddressKey;
        Handle.PostMessage(Cmd_DV_CC_GetAddressKeyResp, 0, Resp);
    }
}

xAddressChallengeService::xAddressChallengeService(const std::vector<xel::xNetAddress> & BindAddressList) {
    SERVICE_RUNTIME_ASSERT(ServiceRunState);
    SERVICE_RUNTIME_ASSERT(BindAddressList.size());
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
