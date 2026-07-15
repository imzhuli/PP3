#pragma once
#include <pp_common/_.hpp>

struct xPP_AddressChallenge final : public xBinaryMessage {  // from proxy_access to relay server
    void SerializeMembers() override {
        W(Message);
    }
    void DeserializeMembers() override {
        R(Message);
    }

    std::string_view Message;
};

struct xPP_AddressChallengeResp final : public xBinaryMessage {  // from proxy_access to relay server
    void SerializeMembers() override {
        W(AddressKey);
    }
    void DeserializeMembers() override {
        R(AddressKey);
    }

    std::string_view AddressKey;
};
