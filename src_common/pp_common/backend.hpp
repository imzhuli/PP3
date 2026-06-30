#include "./_.hpp"
#include "./_region.hpp"

using xAuthId = uint64_t;

struct xTcpClientAuthResult {
    xAuthId     AuthId;
    xCountryId  CountryId;
    xStateId    StateId;
    xCityId     CityId;
    bool        RequireIpv6;
    bool        RequireUdp;
    bool        AlwaysChangeIp;
    bool        PersistentDeviceBinding;
    std::string PAToken;
    std::string ThirdRedirect;
};

struct xAuditAccountInfo {
    uint64_t AuthId = {};

    uint64_t TotalTcpCount        = {};
    uint64_t TotalTcpUploadSize   = {};
    uint64_t TotalTcpDownloadSize = {};

    uint64_t TotalUdpCount        = {};
    uint64_t TotalUdpUploadSize   = {};
    uint64_t TotalUdpDownloadSize = {};
};
