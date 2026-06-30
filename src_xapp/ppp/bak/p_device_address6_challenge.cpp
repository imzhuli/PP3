#include "./p_device_address6_challenge.hpp"

#include <pp_common/network.hpp>

static const char * CheckAddressKey = "$%NDSFASTT!";

void xPP_DeviceAddress6ChallengeResp::SerializeMembers() {
    auto PackedAddress = SignAndPackAddress(xel::GetTimestampMS(), CheckAddressKey, Address);
    W(PackedAddress);  //
}

void xPP_DeviceAddress6ChallengeResp::DeserializeMembers() {
    auto PackedAddress = std::string();
    R(PackedAddress);
    Address = ExtractAddressFromPack(PackedAddress, CheckAddressKey);
}
