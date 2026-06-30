#include "./auth_abstract.hpp"

std::string xAuthResult::ToString() const {
    auto SS = std::ostringstream();

    SS << "LocalAuthId:" << LocalAuthId << endl;
    SS << "GlobalAuthId:" << GlobalAuthId << endl;
    SS << "CountryId:" << CountryId << endl;
    SS << "ProxyAccessAddress:" << ProxyAccessAddress << endl;
    SS << "ExportAddress:" << ExportAddress << endl;
    SS << "EnableTcp:" << EnableTcp << endl;
    SS << "EnableUdp:" << EnableUdp << endl;
    SS << "BandwidthLimit:" << BandwidthLimit << endl;
    SS << "ConnectionLimit:" << ConnectionLimit << endl;
    SS << "ExpireTime:" << ExpireTime << endl;
    return SS.str();
}

std::string CombineAccountPass(std::string_view AccountView, std::string_view PassView) {
    return std::string(AccountView) + '\x00' + std::string(PassView);
}
