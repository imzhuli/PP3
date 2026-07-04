#pragma once
#include <expected>
#include <pp_common/_region.hpp>
#include <pp_common/future.hpp>

//
struct xAuthResult;
class xAuthAbstract;

struct xAuthResult {
    uint64_t    LocalAuthId;
    uint64_t    GlobalAuthId;
    xCountryId  CountryId;
    xNetAddress ProxyAccessAddress;
    xNetAddress ExportAddress;
    bool        EnableTcp;
    bool        EnableUdp;
    uint32_t    BandwidthLimit;
    uint32_t    ConnectionLimit;
    uint64_t    ExpireTime;

    std::string ToString() const;
};

struct xLocalUsage {
    bool     Dirty                   = false;
    uint64_t TotalTcpConnections     = {};
    uint64_t TotalTcpBytesFromClient = {};
    uint64_t TotalTcpBytesToClient   = {};
    uint64_t TotalUdpChannels        = {};
    uint64_t TotalUdpBytesFromClient = {};
    uint64_t TotalUdpBytesToClient   = {};
};

struct xAuthResultFuture : xFutureBase {
    xExpected<xAuthResult> Result = UnexpctedResult;
};

class xAuthServiceAbstract : xAbstract {
public:
    virtual void AcquireAuthInfo(const std::string_view AccountPassView, xAuthResultFuture & Future) = 0;
    virtual void ReleaseAuthInfo(uint64_t LocalAuthId, const xLocalUsage & Audit)                    = 0;
};

extern std::string CombineAccountPass(std::string_view AccountView, std::string_view PassView);