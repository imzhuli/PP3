#pragma once
#include <pp_common/_.hpp>

class xAddressChallengeService final : public xel::xRaii {
public:
    xAddressChallengeService(const std::vector<xel::xNetAddress> & BindAddressList);
    ~xAddressChallengeService();

private:
    std::vector<xUdpService *> UdpServiceList;
};
