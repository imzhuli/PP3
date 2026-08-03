#pragma once
#include <pp_common/_.hpp>

struct xPP_DeviceChallenge final : xBinaryMessage {
    void SerializeMembers() override {
        W(Version);
        W(ChannelId);
        W(Timestamp);
        W(AddressKey4);
        W(AddressKey6);
        W(Sign);
    }

    void DeserializeMembers() override {
        R(Version);
        R(ChannelId);
        R(Timestamp);
        R(AddressKey4);
        R(AddressKey6);
        R(Sign);
    }

    uint32_t         Version     = {};
    uint32_t         ChannelId   = {};
    uint64_t         Timestamp   = {};
    std::string_view AddressKey4 = {};
    std::string_view AddressKey6 = {};
    std::string_view Sign        = {};
};

struct xPP_DeviceChallengeResp final : xBinaryMessage {

    void SerializeMembers() override {
        W(Accepted);
        W(RelayAddress);
        W(RelayCheckKey);
        W(BanVersionTimeMS);
    }

    void DeserializeMembers() override {
        R(Accepted);
        R(RelayAddress);
        R(RelayCheckKey);
        R(BanVersionTimeMS);
    }

    bool             Accepted         = {};
    xNetAddress      RelayAddress     = {};
    std::string_view RelayCheckKey    = {};
    uint64_t         BanVersionTimeMS = {};
};
