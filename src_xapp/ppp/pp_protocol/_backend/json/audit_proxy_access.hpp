#pragma once
#include <pp_common/_.hpp>

struct xAudit_ProxyAccess {

    const char * AuditName = "xAudit_ProxyAccess";

    std::string ServerUuid;
    uint64_t    ServerLocalTimestampMS;
    uint64_t    AuditDuration;

    uint64_t TotalClientConnectionCount;
    uint64_t TotalUploadSize;
    uint64_t TotalDownloadSize;

    uint64_t LocalAuthCachedCount;
    uint64_t LocalAuthCacheHitCount;
    uint64_t InvalidAuthenticationCount;
    uint64_t TimeoutAuthenticationCount;

    uint64_t Socks5ConnectionCount;
    uint64_t HttpConnectionCount;
    uint64_t RemoteDnsQueryCount;

    uint64_t UdpChannelCount;

    //
};
