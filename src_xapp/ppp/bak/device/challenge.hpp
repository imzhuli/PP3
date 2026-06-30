#pragma once
#include "../internal/challenge_key.hpp"

#include <pp_common/_.hpp>

struct xPP_DeviceChallenge : xBinaryMessage {
    void SerializeMembers() override {
        W(AppVersion, ChannelId, Timestamp);
        W(Tcp4AddressKey);
        W(Tcp6AddressKey);
        W(Udp4AddressKey);
        W(Udp6AddressKey);
        W(Sign);
    };

    void DeserializeMembers() override {
        R(AppVersion, ChannelId, Timestamp);
        R(Tcp4AddressKey);
        R(Tcp6AddressKey);
        R(Udp4AddressKey);
        R(Udp6AddressKey);
        R(Sign);
    };

    uint32_t AppVersion = 0;
    uint32_t ChannelId  = 0;
    uint64_t Timestamp  = 0;

    std::string Tcp4AddressKey = {};
    std::string Tcp6AddressKey = {};
    std::string Udp4AddressKey = {};
    std::string Udp6AddressKey = {};
    std::string Sign           = {};

    xDeviceChallengePack Extract() const;
};

struct xPP_DeviceChallengeResp : xBinaryMessage {

    void SerializeMembers() override {
        W(Accepted, RelayAddress, RelayCheckKey);
        W(EnableOldVersion, EnableSpeedControl);
        W(BanVersionTimeMS);
    };
    void DeserializeMembers() override {
        R(Accepted, RelayAddress, RelayCheckKey);
        R(EnableOldVersion, EnableSpeedControl);
        R(BanVersionTimeMS);
    };

    bool        Accepted           = false;
    xNetAddress RelayAddress       = {};
    std::string RelayCheckKey      = {};
    bool        EnableOldVersion   = false;
    bool        EnableSpeedControl = false;

    // on error:
    uint64_t BanVersionTimeMS = 0;

    //
};
