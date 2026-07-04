#pragma once
#include "../../abstract/audit_abstract.hpp"
#include "../small_server_list_downloader.hpp"

#include <pp_common/_.hpp>
#include <random>
#include <xel/object/object.hpp>

struct xSmallServerList {
    using xServerListContainer = std::array<xServerInfo, MAX_SMALL_SERVER_LIST_SIZE>;
    using xServerListSize      = size_t;

    xServerListContainer Container;
    xServerListSize      Size = 0;
};

class xAuditService final
    : public xTargetReporterServiceAbstract
    , public xAuditServiceAbstract {
public:
    bool Init(const xNetAddress & ServerListServerAddress, const xNetAddress & LocalBindAddress);
    void Clean();
    void Tick(uint64_t NowMS);
    auto OutputAudit() -> std::string;

    void ReportTarget(uint64_t GlobalAuthId, const xel::xNetAddress & TargetAddress, const std::string_view & TargetHost, size_t Count) override;
    void ReportUsage(const xAuditUsage & UsageInfo) override;
    void ReportBlockAccount(const xAuditBlockAccount & BlockAccountInfo) override;

private:
    void OnServerListUpdated(xServerGroup ServerGroup, const xServerInfo * ServerList, size_t ServerListSize, uint64_t VersionTimestampMS);

private:
    using xAuditConnectionIdList = std::array<uint64_t, MAX_SMALL_SERVER_LIST_SIZE>;

    xel::xTcpClientPool        TcpReporter;
    xel::xUdpService           UdpReporter;
    xSmallServerListDownloader ServerListDownloader;
    //
    xSmallServerList           AuditServerList;
    xAuditConnectionIdList     ConnectionIdList;
    xTcpClientPool             AuditClientPool;

    struct xAudit {
        size_t NoServerReport = 0;
    };
    xAudit Audit;
};
