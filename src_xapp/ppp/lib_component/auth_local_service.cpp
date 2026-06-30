#include "./auth_local_service.hpp"

#include "./pa_future.hpp"

#include <rapidcsv.h>

#include <pp_common/service_runtime.hpp>
#include <system_error>

namespace fs = std::filesystem;

#ifndef NDEBUG
static constexpr const auto     RELOAD_TIMEOUT                   = 15s;
static constexpr const uint64_t ACCOUNT_BLOCK_TIMEOUT_MS         = 1 * 60'000;
static constexpr const size_t   LOCAL_AUDIT_NODE_IDLE_TIMEOUT_MS = 1 * 60'000;
#else
static constexpr const auto     RELOAD_TIMEOUT                   = 5 * 60s;
static constexpr const uint64_t ACCOUNT_BLOCK_TIMEOUT_MS         = 15 * 60'000;
static constexpr const size_t   LOCAL_AUDIT_NODE_IDLE_TIMEOUT_MS = 5 * 60'000;
#endif

static constexpr const uint64_t BANDWITH_LIMIT_OVER_FACTOR   = 3;
static constexpr const uint64_t CONNECTION_LIMIT_OVER_FACTOR = 3;
static constexpr const size_t   LOCAL_AUDIT_NODE_POOL_SIZE   = 10'0000;

std::string xAuthLocalRecord::ToString() const {
    auto OS = std::ostringstream();
    OS << "xAuthLocalRecord:" << endl;
    OS << "\tCountryId:" << CountryId << endl;
    OS << "\tEnableUdp:" << EnableUdp << endl;
    OS << "\tProxyClientAddress:" << ProxyClientAddress.IpToString() << endl;
    OS << "\tStaticExportAddress:" << StaticExportAddress.IpToString();
    return OS.str();
}

std::string xAuthLocalUsageAuditNode::ToString() const {
    auto OS = std::ostringstream();
    OS << "xAuthLocalUsageAuditNode:" << endl;
    OS << "\tLocalAuthId:" << LocalAuthId << endl;
    OS << "\tReferenceCount:" << ReferenceCount << endl;
    OS << "\tGlobalAuthId:" << GlobalAuthId << endl;
    OS << "\tTotalTcpConnections:" << Audit.TotalTcpConnections << endl;
    OS << "\tTotalTcpBytesFromClient:" << Audit.TotalTcpBytesFromClient << endl;
    OS << "\tTotalTcpBytesToClient:" << Audit.TotalTcpBytesToClient << endl;
    OS << "\tTotalUdpBytesFromClient:" << Audit.TotalUdpBytesFromClient << endl;
    OS << "\tTotalUdpBytesToClient:" << Audit.TotalUdpBytesToClient << endl;

    return OS.str();
}

bool xAuthLocalService::Init(const std::string & AuthFileDir) {
    if (!LocalAuditNodePool.Init(LOCAL_AUDIT_NODE_POOL_SIZE)) {
        return false;
    }

    if (!fs::is_directory(AuthFileDir, XR(std::error_code()))) {
        Logger->E("Failed to open AuthFileDir");
        return false;
    }
    this->AuthFileDir = AuthFileDir;

    RunState.Start();
    Reset(ReloadAuthFileThread, std::thread([this]() { ReloadAuthFile(); }));
    return true;
}

void xAuthLocalService::Clean() {
    RunState.Stop();
    ReloadAuthFileThread.join();
    Reset(ReloadAuthFileThread);
    RunState.Finish();

    Reset(AuthLocalMap);
    Reset(AuthFileDir);

    LocalAuditNodePool.Clean();
}

void xAuthLocalService::Tick(uint64_t NowMS) {
    LocalTicker.Update(NowMS);
    CheckAndReportLocalAudit();
}

void xAuthLocalService::CheckAndReportLocalAudit() {
    auto NowMS          = LocalTicker();
    auto AuditTimepoint = NowMS - LOCAL_AUDIT_NODE_IDLE_TIMEOUT_MS;
    auto Cond           = [=](const xAuthLocalUsageAuditNode & N) {
        return N.LastReportTimestampMS <= AuditTimepoint;
    };
    while (auto P = LocalAuditTimeoutList.PopHead(Cond)) {
        if (Steal(P->Audit.Dirty)) {
            auto UsageInfo                    = xAuditUsage();
            UsageInfo.AuthId                  = P->GlobalAuthId;
            UsageInfo.StartTimestampMS        = P->LastReportTimestampMS;
            UsageInfo.PeriodMS                = NowMS - P->LastReportTimestampMS;
            UsageInfo.TotalTcpConnections     = Steal(P->Audit.TotalTcpConnections);
            UsageInfo.TotalTcpBytesFromClient = Steal(P->Audit.TotalTcpBytesFromClient);
            UsageInfo.TotalTcpBytesToClient   = Steal(P->Audit.TotalTcpBytesToClient);
            UsageInfo.TotalUdpChannels        = Steal(P->Audit.TotalUdpChannels);
            UsageInfo.TotalUdpBytesFromClient = Steal(P->Audit.TotalUdpBytesFromClient);
            UsageInfo.TotalUdpBytesToClient   = Steal(P->Audit.TotalUdpBytesToClient);
            AuditService->ReportUsage(UsageInfo);
        }

        if (P->ReferenceCount) {
            P->LastReportTimestampMS = NowMS;
            LocalAuditTimeoutList.AddTail(*P);
        } else {
            LocalAuditNodePool.Release(P->LocalAuthId);
            --Audit.UnReleasedAuthInfoCount;
        }
    }
}

std::string xAuthLocalService::OutputAudit() const {
    auto OS = std::ostringstream();
    OS << "xAuthLocalService:" << endl;
    OS << "\tUnReleasedAuthInfoCount:" << Audit.UnReleasedAuthInfoCount << endl;
    OS << "\tCachedAuthInfoMapVersion:" << Audit.CachedAuthInfoMapVersion << endl;
    OS << "\tCachedAuthInfoCount:" << Audit.CachedAuthInfoCount << endl;
    return OS.str();
}

void xAuthLocalService::AcquireAuthInfo(const std::string_view AccountPassView, xAuthResultFuture & Future) {
    DEBUG_LOG("\n%s", HexShow(AccountPassView).c_str());

    // check and update auth map;
    auto NewMap = LastReloadInfo.AuthMap.exchange(nullptr);
    if (NewMap) {
        for (auto & Entry : AuthLocalMap) {
            auto & LocalRecord = Entry.second;
            if (LocalRecord.LocalAuthId) {
                auto & LocalUsageNode = LocalAuditNodePool[LocalRecord.LocalAuthId];
                --LocalUsageNode.ReferenceCount;
            }
        }

        AuthLocalMap              = std::move(*NewMap);
        Audit.CachedAuthInfoCount = AuthLocalMap.size();
        ++Audit.CachedAuthInfoMapVersion;

#ifndef NDEBUG
        for (auto & Entry : AuthLocalMap) {
            auto & LR = Entry.second;
            Logger->D("New auth entry: %s, LocalAuthId=%" PRIx64 ", GlobalAuthId=%" PRIu64 "", StrToHex(Entry.first), LR.LocalAuthId, LR.GlobalAuthId);
        }
#endif

        delete NewMap;
    }

    Future.SetReady();
    auto Iter = AuthLocalMap.find(AccountPassView);
    if (Iter == AuthLocalMap.end()) {
        return;
    }
    auto & LocalRecord    = Iter->second;
    auto   LocalUsageNode = CheckAndSetLocalAuthId(LocalRecord);
    if (LocalRecord.BlockStartTimestampMS) {
        if (LocalRecord.BlockStartTimestampMS + ACCOUNT_BLOCK_TIMEOUT_MS <= LocalTicker()) {  // unblock
            DEBUG_LOG("UnblockLocalRecord: GlobalAuthId=%" PRIu64 "", LocalRecord.GlobalAuthId);
            LocalRecord.BlockStartTimestampMS = 0;
        } else {
            DEBUG_LOG("AccountBlocked: GlobalAuthId=%" PRIu64 "", LocalRecord.GlobalAuthId);
            return;
        }
    }
    if (LocalRecord.BandwidthLimit) {  // check if the account is about to be blocked
        auto & Audit         = LocalUsageNode->Audit;
        auto   TotalConsumed = Audit.TotalTcpBytesFromClient + Audit.TotalTcpBytesToClient + Audit.TotalUdpBytesFromClient + Audit.TotalUdpBytesToClient;
        if (TotalConsumed > LocalRecord.BandwidthLimit) {
            LocalRecord.BlockStartTimestampMS = LocalTicker();
            DEBUG_LOG("TriggerAccountBlocking: GlobalAuthId=%" PRIu64 ", ConsumedSize=%" PRIu64 ", Threshold=%" PRIu64 "", LocalRecord.GlobalAuthId, TotalConsumed, LocalRecord.BandwidthLimit);
            auto BlockAccountInfo             = xAuditBlockAccount();
            BlockAccountInfo.AuthId           = LocalRecord.GlobalAuthId;
            BlockAccountInfo.StartTimestampMS = LocalTicker();
            BlockAccountInfo.PeriodMS         = ACCOUNT_BLOCK_TIMEOUT_MS;
            BlockAccountInfo.Reason           = eBlockAccountReason::BANDWITH_LIMIT;
            BlockAccountInfo.Threshold        = LocalRecord.BandwidthLimit;
            BlockAccountInfo.TriggerValue     = TotalConsumed;
            AuditService->ReportBlockAccount(BlockAccountInfo);
            return;
        }
    }
    if (LocalRecord.ConnectionLimit) {
        auto & Audit               = LocalUsageNode->Audit;
        auto   TotalTcpConnections = Audit.TotalTcpConnections;
        if (TotalTcpConnections > LocalRecord.ConnectionLimit) {
            LocalRecord.BlockStartTimestampMS = LocalTicker();
            DEBUG_LOG("TriggerAccountBlocking: GlobalAuthId=%" PRIu64 ", ConnectionCount=%" PRIu64 ", Threshold=%" PRIu64 "", LocalRecord.GlobalAuthId, TotalTcpConnections, LocalRecord.BandwidthLimit);
            auto BlockAccountInfo             = xAuditBlockAccount();
            BlockAccountInfo.AuthId           = LocalRecord.GlobalAuthId;
            BlockAccountInfo.StartTimestampMS = LocalTicker();
            BlockAccountInfo.PeriodMS         = ACCOUNT_BLOCK_TIMEOUT_MS;
            BlockAccountInfo.Reason           = eBlockAccountReason::CONNECTION_LIMIT;
            BlockAccountInfo.Threshold        = LocalRecord.ConnectionLimit;
            BlockAccountInfo.TriggerValue     = TotalTcpConnections;
            AuditService->ReportBlockAccount(BlockAccountInfo);
            return;
        }
    }

    DEBUG_LOG("FoundAuthRecord:%s", LocalRecord.ToString().c_str());
    auto & Result              = Future.Result;
    Result                     = {};
    Result->LocalAuthId        = LocalRecord.LocalAuthId;
    Result->GlobalAuthId       = LocalRecord.GlobalAuthId;
    Result->ProxyAccessAddress = LocalRecord.ProxyClientAddress;
    Result->ExportAddress      = LocalRecord.StaticExportAddress;
    Result->EnableTcp          = LocalRecord.EnableTcp;
    Result->EnableUdp          = LocalRecord.EnableUdp;
    ++LocalUsageNode->ReferenceCount;
    return;
}

void xAuthLocalService::ReleaseAuthInfo(uint64_t LocalAuthId, const xLocalUsage & Usage) {
    assert(LocalAuthId);
    assert(LocalAuditNodePool.CheckAndGet(LocalAuthId)->LocalAuthId == LocalAuthId);

    auto & LocalAuditNode          = LocalAuditNodePool[LocalAuthId];
    //
    auto & Audit                   = LocalAuditNode.Audit;
    Audit.Dirty                    = true;
    Audit.TotalTcpConnections     += Usage.TotalTcpConnections;
    Audit.TotalTcpBytesFromClient += Usage.TotalTcpBytesFromClient;
    Audit.TotalTcpBytesToClient   += Usage.TotalTcpBytesToClient;
    Audit.TotalUdpChannels        += Usage.TotalUdpChannels;
    Audit.TotalUdpBytesFromClient += Usage.TotalUdpBytesFromClient;
    Audit.TotalUdpBytesToClient   += Usage.TotalUdpBytesToClient;
    --LocalAuditNode.ReferenceCount;
}

xAuthLocalUsageAuditNode * xAuthLocalService::CheckAndSetLocalAuthId(xAuthLocalRecord & LocalRecord) {
    if (LocalRecord.LocalAuthId) {
        assert(LocalAuditNodePool.CheckAndGet(LocalRecord.LocalAuthId));
        return &LocalAuditNodePool[LocalRecord.LocalAuthId];
    }
    auto LocalAuthId = LocalAuditNodePool.Acquire();
    X_RUNTIME_ASSERT((LocalRecord.LocalAuthId = LocalAuthId));

    auto & LocalAuditNode                = LocalAuditNodePool[LocalAuthId];
    LocalAuditNode.LocalAuthId           = LocalAuthId;
    LocalAuditNode.GlobalAuthId          = LocalRecord.GlobalAuthId;
    LocalAuditNode.ReferenceCount        = 1;
    LocalAuditNode.LastReportTimestampMS = LocalTicker();
    LocalAuditTimeoutList.AddTail(LocalAuditNode);

    ++Audit.UnReleasedAuthInfoCount;
    return &LocalAuditNode;
}

static void LoadFile(xAuthLocalMap & TargetMap, const fs::path & FilePath) {
    DEBUG_LOG("LoadFile:%s", FilePath.c_str());
    auto   Doc      = rapidcsv::Document(FilePath.string(), rapidcsv::LabelParams(0, 0));
    size_t RowCount = Doc.GetRowCount();  // 数据行数（不含表头）

    auto AuthIdIndex          = Doc.GetColumnIdx("auth_id");
    auto AccountIndex         = Doc.GetColumnIdx("account");
    auto PasswordIndex        = Doc.GetColumnIdx("password");
    auto CountryCodeIndex     = Doc.GetColumnIdx("country_code");
    auto ProxyIpIndex         = Doc.GetColumnIdx("pa_ip");
    auto ExportAddressIndex   = Doc.GetColumnIdx("proxy_ip");
    auto UdpIndex             = Doc.GetColumnIdx("is_udp");
    auto BandwidthLimitIndex  = Doc.GetColumnIdx("bandwidth_limit");
    auto ConnectionLimitIndex = Doc.GetColumnIdx("conn_limit");
    auto ExpiredTimeIndex     = Doc.GetColumnIdx("expired_time");
    auto Now                  = xel::GetUnixTimestamp();

    for (size_t Row = 0; Row < RowCount; ++Row) {
        auto ExpiredTimeLimit = Doc.GetCell<int64_t>(ExpiredTimeIndex, Row);
        if (ExpiredTimeLimit < Now) {
            DEBUG_LOG("AuthRow expired");
            continue;
        }
        auto AuthId          = (uint64_t)strtoumax(Doc.GetCell<std::string>(AuthIdIndex, Row).c_str(), nullptr, 10);
        auto Account         = Doc.GetCell<std::string>(AccountIndex, Row);
        auto Password        = Doc.GetCell<std::string>(PasswordIndex, Row);
        auto CountryCode     = Doc.GetCell<std::string>(CountryCodeIndex, Row);
        auto ProxyIp         = xNetAddress::Parse(Doc.GetCell<std::string>(ProxyIpIndex, Row));
        auto ExportAddress   = xNetAddress::Parse(Doc.GetCell<std::string>(ExportAddressIndex, Row));
        auto Udp             = Doc.GetCell<std::string>(UdpIndex, Row);
        auto BandwidthLimit  = (uint64_t)std::strtoumax(Doc.GetCell<std::string>(BandwidthLimitIndex, Row).c_str(), nullptr, 10);
        auto ConnectionLimit = (uint64_t)std::strtoumax(Doc.GetCell<std::string>(ConnectionLimitIndex, Row).c_str(), nullptr, 10);

        auto   AccountKey = CombineAccountPass(Account, Password);
        auto & Node       = TargetMap[AccountKey];

        Node.GlobalAuthId        = AuthId;
        Node.CountryId           = CountryCodeToCountryId(CountryCode.c_str());
        Node.ProxyClientAddress  = ProxyIp;
        Node.StaticExportAddress = ExportAddress;
        Node.EnableUdp           = atoi(Udp.c_str());
        Node.BandwidthLimit      = uint64_t(BANDWITH_LIMIT_OVER_FACTOR * BandwidthLimit * (LOCAL_AUDIT_NODE_IDLE_TIMEOUT_MS / 1000.));
        Node.ConnectionLimit     = uint64_t(CONNECTION_LIMIT_OVER_FACTOR * ConnectionLimit * (LOCAL_AUDIT_NODE_IDLE_TIMEOUT_MS / 1000.));

        (void)ConnectionLimit;
    }
}

void xAuthLocalService::ReloadAuthFile() {
    auto ReloadTimer = xTimer(ZeroInit);
    while (RunState) {
        if (!ReloadTimer.TestAndTag(RELOAD_TIMEOUT)) {
            std::this_thread::sleep_for(1s);
            continue;
        }
        try {
            std::vector<std::filesystem::path>           FileList;
            std::vector<std::filesystem::file_time_type> FileTimestampList;
            for (const auto & Entry : fs::directory_iterator(AuthFileDir)) {  // build file list.
                if (!Entry.is_regular_file()) {
                    continue;
                }
                const fs::path & FilePath = Entry.path();
                if (ToLower(FilePath.extension()) != ".csv"s) {
                    continue;
                }
                FileList.push_back(FilePath);
                FileTimestampList.push_back(std::filesystem::last_write_time(FilePath));
            }
            // compare with the old list if any filename or last file save timestamp is different, reload all director && create new map
            bool NeedReload = false;
            do {
                if (FileList.size() != LastReloadInfo.FileList.size()) {
                    NeedReload = true;
                    break;
                }
                for (auto Index = decltype(FileList.size())(0); Index < FileList.size(); ++Index) {
                    auto & FilePath         = FileList[Index];
                    auto & FileTimestamp    = FileTimestampList[Index];
                    auto & OldFilePath      = LastReloadInfo.FileList[Index];
                    auto & OldFIleTimestamp = LastReloadInfo.FileTimestampList[Index];
                    if (FilePath != OldFilePath || FileTimestamp != OldFIleTimestamp) {
                        NeedReload = true;
                        break;
                    }
                }
            } while (false);
            if (NeedReload) {
                auto NewMap = new xAuthLocalMap();
                try {
                    for (auto & F : FileList) {
                        LoadFile(*NewMap, F);
                    }
                } catch (const std::exception & e) {
                    Logger->E("Failed to load auth map: reason=%s", e.what());
                    delete NewMap;
                    return;
                }
                auto OldMap = LastReloadInfo.AuthMap.exchange(NewMap);
                if (OldMap) {
                    delete OldMap;
                }
                Reset(LastReloadInfo.FileList, std::move(FileList));
                Reset(LastReloadInfo.FileTimestampList, std::move(FileTimestampList));

                Logger->I("new auth file(s) loaded");
            }
        } catch (const fs::filesystem_error & e) {
            AuditLogger->E("iterate directory error：%s", e.what());
            continue;
        }
    }

    // cleanup:
    do {
        if (auto OldMap = LastReloadInfo.AuthMap.exchange(nullptr)) {
            delete OldMap;
        }
        Reset(LastReloadInfo.FileList);
        Reset(LastReloadInfo.FileTimestampList);
    } while (false);
}
