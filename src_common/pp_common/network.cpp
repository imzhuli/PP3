#include "./network.hpp"

bool IsLocalNetAddress(const xNetAddress & Addr) {
    if (Addr.Is4()) {
        ubyte B0 = Addr.SA4[0];
        ubyte B1 = Addr.SA4[1];
        if (B0 == 10) {
            return true;
        }
        if (B0 == 192 && B1 == 168) {
            return true;
        }
        if (B0 == 172 && (B1 >= 16 && B1 < 31)) {
            return true;
        }
    } else if (Addr.Is6()) {
        ubyte B0 = Addr.SA6[0];
        ubyte B1 = Addr.SA6[1];
        if ((B0 & 0xFE) == 0xFC) {
            return true;
        }
        if (B0 == 0xFE && (B1 & 0xC0) == 0x80) {
            return true;
        }
    }
    return false;
}

std::string SignAndPackAddress(uint64_t Timestamp, const std::string & SecretKey, const xNetAddress & Address) {
    auto IpString = Address.ToString();
    auto Sign     = AppSign(Timestamp, SecretKey, IpString);
    return IpString + "=" + Sign;
}

xNetAddress ExtractAddressFromPack(const std::string & SignedPack, const std::string & SecretKey) {
    auto Segs = Split(SignedPack, "=");
    if (Segs.size() != 2) {
        return {};
    }
    const auto & IpString = Segs[0];
    if (!ValidateAppSign(Segs[1], SecretKey, IpString)) {
        return {};
    }
    return xNetAddress::Parse(IpString);
}
