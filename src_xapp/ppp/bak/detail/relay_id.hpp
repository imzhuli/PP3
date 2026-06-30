#pragma once
#include <pp_common/_.hpp>

struct xAcquireRelayServerId final : xBinaryMessage {
    void SerializeMembers() {
        W(PreviousRelayServerId);
    }
    void DeserializeMembers() {
        R(PreviousRelayServerId);
    }
    uint64_t PreviousRelayServerId;
};

struct xAcquireRelayServerIdResp final : xBinaryMessage {
    void SerializeMembers() {
        W(RelayServerId);
    }
    void DeserializeMembers() {
        R(RelayServerId);
    }
    uint64_t RelayServerId;
};
