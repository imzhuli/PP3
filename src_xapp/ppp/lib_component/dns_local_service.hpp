#pragma once
#include "../abstract/dns_abstract.hpp"

#include <map>

struct xDnsRequestNode : xListNode {
    uint64_t                 RequestNodeId;
    std::string              Hostname;
    std::vector<xNetAddress> A4List;
    std::vector<xNetAddress> A6List;
};
using xDnsRequestList = xList<xDnsRequestNode>;

struct xDnsLocalSourceRequestNode : xListNode {
    uint64_t      SourceRequestNodeId;
    xFutureHandle FutureHandle;
};
using xDnsLocalSourceRequestList = xList<xDnsLocalSourceRequestNode>;

struct xDnsLocalCacheResult {
    std::vector<xNetAddress> A4List;
    std::vector<xNetAddress> A6List;
    size_t                   LastA4ResultIndex = 0;
    size_t                   LastA6ResultIndex = 0;
};

struct xDnsLocalCacheNode : xListNode {
    std::string                                Hostname;
    bool                                       QueryFinished = false;
    uint64_t                                   TimestampMS   = 0;
    xDnsLocalSourceRequestList                 PendingSourceRequestList;
    std::expected<xDnsLocalCacheResult, xNone> Result = std::unexpected(None);
};
using xDnsLocalCacheTimeoutList = xList<xDnsLocalCacheNode>;

class xDnsLocalService final : public xDnsServiceAbstract {
public:
    bool Init();
    void Clean();
    void Tick(uint64_t NowMS);
    auto OutputAudit() const -> std::string;

    bool ResolveDns(const std::string_view & HostnameView, xDnsReultFuture & Future) override;

private:
    xDnsResult PickDnsResult(xDnsLocalCacheNode * CacheNode);
    void       DispatchResolveResults();
    void       ProcessRequestTimeoutCacheNodes();
    void       ProcessTimeoutCacheNodes();

    void QueryThreadFunc();

private:
    xTicker                  LocalTicker;
    xel::xRunState           QueryThreadsRunState;
    std::vector<std::thread> QueryThreads;

    xel::xAutoResetEvent RequestEvent;
    xDnsRequestList      RequestList;
    xel::xSpinlock       RequestFinishLock;
    xDnsRequestList      RequestFinishedList;

    xIndexedStorage<xDnsRequestNode>                             DnsRequestPool;
    xIndexedStorage<xDnsLocalSourceRequestNode>                  DnsSourceRequestPool;
    xIndexedStorage<xDnsLocalCacheNode>                          DnsLocalCachePool;
    std::map<std::string, xDnsLocalCacheNode *, std::less<void>> CacheMap;
    xDnsLocalCacheTimeoutList                                    CacheRequestTimeoutList;
    xDnsLocalCacheTimeoutList                                    CacheTimeoutList;

    struct {
        size_t OnGoingDnsRequestCount = 0;
    } Audit;
};
