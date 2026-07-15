#pragma once
#include "../abstract/audit_abstract.hpp"
#include "../abstract/auth_abstract.hpp"

#include <filesystem>
#include <pp_common/_.hpp>

struct xStringHash {
    using is_transparent = void;  // 关键标记
    std::size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
};

struct xStringEqual {
    using is_transparent = void;  // 关键标记
    bool operator()(std::string_view a, std::string_view b) const noexcept {
        return a == b;
    }
};

struct xAuthLocalRecord final {
    uint64_t    LocalAuthId     = 0;
    uint64_t    GlobalAuthId    = 0;
    xCountryId  CountryId       = 0;
    bool        EnableTcp       = true;
    bool        EnableUdp       = false;
    uint64_t    BandwidthLimit  = 0;
    uint64_t    ConnectionLimit = 0;
    xNetAddress ProxyClientAddress;
    xNetAddress StaticExportAddress;
    //
    uint64_t    BlockStartTimestampMS = 0;

    std::string ToString() const;
};
using xAuthLocalMap = std::unordered_map<std::string, xAuthLocalRecord, xStringHash, xStringEqual>;

struct xAuthLocalUsageAuditNode final : xListNode {
    uint64_t    LocalAuthId           = 0;
    uint64_t    GlobalAuthId          = 0;
    size_t      ReferenceCount        = 0;
    uint64_t    LastReportTimestampMS = 0;
    //
    xLocalUsage Audit;

    std::string ToString() const;
};

class xAuthLocalService final : public xAuthServiceAbstract {
public:
    bool        Init(const std::string & AuthFilePath);
    void        Clean();
    void        Tick(uint64_t NowMS);
    std::string OutputAudit() const;
    void        BindAuditService(xAuditServiceAbstract * Service) { AuditService = Service; }

    void AcquireAuthInfo(const std::string_view AccountPassView, xAuthResultFuture & Future) override;
    void ReleaseAuthInfo(uint64_t LocalAuthId, const xLocalUsage & Usage) override;

private:
    void CheckAndReportLocalAudit();
    void ReloadAuthFile();
    auto CheckAndSetLocalAuthId(xAuthLocalRecord & LocalRecord) -> xAuthLocalUsageAuditNode *;

private:
    xRunState                                 RunState;
    xTicker                                   LocalTicker;
    //
    std::filesystem::path                     AuthFileDir;
    xAuthLocalMap                             AuthLocalMap;
    xIndexedStorage<xAuthLocalUsageAuditNode> LocalAuditNodePool;
    xList<xAuthLocalUsageAuditNode>           LocalAuditTimeoutList;
    std::thread                               ReloadAuthFileThread;
    //
    xAuditServiceAbstract *                   AuditService;

    struct {
        std::vector<std::filesystem::path>           FileList;
        std::vector<std::filesystem::file_time_type> FileTimestampList;
        std::atomic<xAuthLocalMap *>                 AuthMap;
    } LastReloadInfo;

    struct {
        size_t UnReleasedAuthInfoCount  = 0;
        size_t CachedAuthInfoCount      = 0;
        size_t CachedAuthInfoMapVersion = 0;
    } Audit;
};
