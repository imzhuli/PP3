#include "./auth_by_user_pass.hpp"

#include <sstream>

std::string xPPB_BackendAuthByUserPassResp::ToString() const {
    auto O = std::ostringstream();
    O << "EC: " << (uint32_t)ErrorCode << endl;

    O << "AuthId:" << AuthId << endl;
    O << "ContinentId:" << ContinentId << endl;
    O << "CountryId:" << CountryId << endl;
    O << "StateId:" << StateId << endl;
    O << "CityId:" << CityId << endl;
    O << "Duration:" << Duration << endl;
    O << "Random:" << Random << endl;
    O << "AutoChangeIpOnDeviceOffline:" << YN(AutoChangeIpOnDeviceOffline) << endl;
    O << "EnableUdp:" << YN(EnableUdp) << endl;
    O << "Ipv6Prefered:" << YN(Ipv6Prefered) << endl;
    O << "PoolFlags:" << PoolFlags << endl;
    O << "ServerToken:" << ServerToken << endl;
    O << "Redirect:" << Redirect << endl;
    O << "AuthIdForThirdPartyResource:" << AuthIdForThirdPartyResource << endl;

    return O.str();
}