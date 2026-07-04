#pragma once
#include <pp_common/_.hpp>
#include <pp_common/_error.hpp>

struct xAuditUsage {
    uint64_t AuthId;
    uint64_t StartTimestampMS;
    uint64_t PeriodMS;
    uint64_t TotalTcpConnections;
    uint64_t TotalTcpBytesFromClient;
    uint64_t TotalTcpBytesToClient;
    uint64_t TotalUdpChannels;
    uint64_t TotalUdpBytesFromClient;
    uint64_t TotalUdpBytesToClient;

    std::string ToString() const;
};

struct xAuditBlockAccount {
    uint64_t            AuthId;
    uint64_t            StartTimestampMS;
    uint64_t            PeriodMS;
    eBlockAccountReason Reason;
    uint64_t            Threshold;
    uint64_t            TriggerValue;

    std::string ToString() const;
};

struct xAuditServiceAbstract : xAbstract {
    virtual void ReportUsage(const xAuditUsage & UsageInfo)                      = 0;
    virtual void ReportBlockAccount(const xAuditBlockAccount & BlockAccountInfo) = 0;
};

struct xTargetReporterServiceAbstract : xAbstract {
    virtual void ReportTarget(uint64_t GlobalAuthId, const xel::xNetAddress & TargetAddress, const std::string_view & TargetHost, size_t Count) = 0;
};
