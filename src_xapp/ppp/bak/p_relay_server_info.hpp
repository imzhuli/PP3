#pragma once
#include <pp_common/_.hpp>

struct xPP_RelayServerInfo : public xBinaryMessage {  // from proxy_access to relay server
public:
    void SerializeMembers() override {
        W(RelayServerId);  //
    }

    void DeserializeMembers() override {
        R(RelayServerId);  //
    }

    uint64_t RelayServerId;
};

struct xPP_RelayServerInfoResp : public xBinaryMessage {  // from proxy_access to relay server
public:
    void SerializeMembers() override {
        W(RelayServerId);  //
    }

    void DeserializeMembers() override {
        R(RelayServerId);  //
    }

    uint64_t RelayServerId;
};
