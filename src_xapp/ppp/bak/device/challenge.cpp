#include "./challenge.hpp"

#include <pp_common/network.hpp>

static const char * CheckAddressKey = "$%NDSFAS!";

xDeviceChallengePack xPP_DeviceChallenge::Extract() const {
    auto Result = xDeviceChallengePack();
    auto Key    = std::string(CheckAddressKey);

    Result.Version     = AppVersion;
    Result.ChannelId   = ChannelId;
    Result.Tcp4Address = ExtractAddressFromPack(Tcp4AddressKey, Key);
    Result.Tcp6Address = ExtractAddressFromPack(Tcp6AddressKey, Key);
    Result.Udp4Address = ExtractAddressFromPack(Udp4AddressKey, Key);
    Result.Udp6Address = ExtractAddressFromPack(Udp6AddressKey, Key);
    return Result;
}
