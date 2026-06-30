#include "./check_address.hpp"

#include <pp_common/network.hpp>

static const char * CheckAddressKey = "$%NDSFAS!";

void xPP_AddressAutoPush::SerializeMembers() {
    auto PackedAddress = SignAndPackAddress(xel::GetTimestampMS(), CheckAddressKey, ConnectionAddress);
    W(PackedAddress);  //
}

void xPP_AddressAutoPush::DeserializeMembers() {
    auto PackedAddress = std::string();
    R(PackedAddress);
    ConnectionAddress = ExtractAddressFromPack(PackedAddress, CheckAddressKey);
}
