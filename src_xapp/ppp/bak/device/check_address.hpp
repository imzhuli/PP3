#pragma once
#include <pp_common/_.hpp>

struct xPP_AddressChallenge : public xBinaryMessage {  // from proxy_access to relay server
public:
    void SerializeMembers() override {
        W(Hello);  //
    }

    void DeserializeMembers() override {
        R(Hello);  //
    }

    std::string Hello;
};

struct xPP_AddressAutoPush : public xBinaryMessage {  // from proxy_access to relay server
public:
    void SerializeMembers() override;
    void DeserializeMembers() override;

    xNetAddress ConnectionAddress;
};
