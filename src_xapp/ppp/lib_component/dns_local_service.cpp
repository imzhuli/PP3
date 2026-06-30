#include "./dns_local_service.hpp"

#include <arpa/inet.h>
#include <netdb.h>

#include <pp_common/service_runtime.hpp>

static constexpr const size_t   DNS_REQUEST_POOL_SIZE        = 3000;
static constexpr const size_t   DNS_SOURCE_REQUEST_POOL_SIZE = 20000;
static constexpr const size_t   DNS_QUERY_THREAD_COUNT       = 50;
static constexpr const uint64_t DNS_QUERY_TIMEOUT_MS         = 5'000;
static constexpr const uint64_t DNS_CACHE_TIMEOUT_MS         = 45 * 60'000;

// #include <ares.h>
// static X_SCOPE_GUARD([] { X_RUNTIME_ASSERT(!ares_library_init(ARES_LIB_INIT_ALL)); }, [] { ares_library_cleanup(); });
// [[maybe_unused]] static void AresCallback(void * arg, int status, int timeouts, struct hostent * host) {
//     if (status == ARES_SUCCESS) {
//         printf("DNS查询成功: %s\n", host->h_name);
//     } else {
//         printf("DNS查询失败: %s\n", ares_strerror(status));
//     }
// }

bool xDnsLocalService::Init() {
    if (!DnsRequestPool.Init(DNS_REQUEST_POOL_SIZE)) {
        return false;
    }
    if (!DnsSourceRequestPool.Init(DNS_SOURCE_REQUEST_POOL_SIZE)) {
        DnsRequestPool.Clean();
        return false;
    }

    QueryThreadsRunState.Start();
    for (size_t I = 0; I < DNS_QUERY_THREAD_COUNT; ++I) {
        QueryThreads.push_back(std::thread([this] {
            QueryThreadFunc();
        }));
    }
    return true;
}

void xDnsLocalService::Clean() {
    QueryThreadsRunState.Stop();
    for (auto & T : QueryThreads) {
        T.join();
    }
    Reset(QueryThreads);
    QueryThreadsRunState.Finish();

    DnsSourceRequestPool.Clean();
    DnsRequestPool.Clean();
}

void xDnsLocalService::Tick(uint64_t NowMS) {
    LocalTicker.Update(NowMS);
    DispatchResolveResults();
    ProcessRequestTimeoutCacheNodes();
    ProcessTimeoutCacheNodes();
}

std::string xDnsLocalService::OutputAudit() const {
    auto OS = std::ostringstream();
    OS << "xDnsLocalService:" << endl;
    OS << "\tOnGoingDnsRequestCount=" << Audit.OnGoingDnsRequestCount << endl;
    return OS.str();
}

xDnsResult xDnsLocalService::PickDnsResult(xDnsLocalCacheNode * CacheNode) {
    assert(CacheNode->Result);
    auto   Result       = xDnsResult();
    auto & CachedResult = *CacheNode->Result;
    if (CachedResult.A4List.size()) {
        if (++CachedResult.LastA4ResultIndex >= CachedResult.A4List.size()) {
            CachedResult.LastA4ResultIndex = 0;
        }
        DEBUG_LOG("PickResult4 %zi out of %zi from Node %p", CachedResult.LastA4ResultIndex, CachedResult.A4List.size(), CacheNode);
        Result.A4 = CachedResult.A4List[CachedResult.LastA4ResultIndex];
    }
    if (CachedResult.A6List.size()) {
        if (++CachedResult.LastA6ResultIndex >= CachedResult.A6List.size()) {
            CachedResult.LastA6ResultIndex = 0;
        }
        Result.A6 = CachedResult.A6List[CachedResult.LastA6ResultIndex];
        DEBUG_LOG("PickResult6 %zi out of %zi from Node %p", CachedResult.LastA6ResultIndex, CachedResult.A6List.size(), CacheNode);
    }
    return Result;
}

void xDnsLocalService::DispatchResolveResults() {
    auto TempResultList = xDnsRequestList();
    do {
        auto G = xel::xSpinlockGuard(RequestFinishLock);
        TempResultList.GrabListTail(RequestFinishedList);
    } while (false);
    while (auto P = TempResultList.PopHead()) {
        --Audit.OnGoingDnsRequestCount;
        DEBUG_LOG("Update dns cache node: Hostname=%s", P->Hostname.c_str());
        auto CacheNodeIter = CacheMap.find(P->Hostname);
        if (CacheNodeIter != CacheMap.end()) {
            auto & CacheNode         = CacheNodeIter->second;
            CacheNode->QueryFinished = true;
            CacheNode->Result        = xDnsLocalCacheResult();
            CacheNode->TimestampMS   = LocalTicker();
            CacheTimeoutList.GrabTail(*CacheNode);

            auto & CacheResult = *CacheNode->Result;
            CacheResult.A4List = std::move(P->A4List);
            CacheResult.A6List = std::move(P->A6List);

            while (auto SourceRequest = CacheNode->PendingSourceRequestList.PopHead()) {
                if (auto F = SourceRequest->FutureHandle.Get<xDnsReultFuture>()) {
                    F->Result = PickDnsResult(CacheNode);
                    F->SetReady();
                }
                DnsSourceRequestPool.Release(SourceRequest->SourceRequestNodeId);
            }
        }
        assert(P == DnsRequestPool.CheckAndGet(P->RequestNodeId));
        DnsRequestPool.Release(P->RequestNodeId);
    }
}

void xDnsLocalService::ProcessRequestTimeoutCacheNodes() {
    auto Cond = [this, KillTimestamp = LocalTicker() - DNS_QUERY_TIMEOUT_MS](const xDnsLocalCacheNode & N) {
        assert(!N.QueryFinished);
        return N.TimestampMS <= KillTimestamp;
    };
    while (auto CacheNode = CacheRequestTimeoutList.PopHead(Cond)) {
        DEBUG_LOG("Timeout dns query: Hostname=%s", CacheNode->Hostname.c_str());
        assert(!CacheNode->Result);
        CacheNode->QueryFinished = true;
        CacheNode->Result        = xDnsLocalCacheResult();
        CacheNode->TimestampMS   = LocalTicker();
        CacheTimeoutList.AddTail(*CacheNode);

        while (auto SourceRequest = CacheNode->PendingSourceRequestList.PopHead()) {
            if (auto F = SourceRequest->FutureHandle.Get<xDnsReultFuture>()) {
                F->SetReady();
            }
            DnsSourceRequestPool.Release(SourceRequest->SourceRequestNodeId);
        }
    }
}

void xDnsLocalService::ProcessTimeoutCacheNodes() {
    auto Cond = [this, KillTimestamp = LocalTicker() - DNS_CACHE_TIMEOUT_MS](const xDnsLocalCacheNode & N) {
        assert(!N.QueryFinished);
        return N.TimestampMS <= KillTimestamp;
    };
    while (auto P = CacheRequestTimeoutList.PopHead(Cond)) {
        assert(P->QueryFinished);
        assert(P->PendingSourceRequestList.IsEmpty());
        CacheTimeoutList.AddTail(*P);
    }
}

void xDnsLocalService::QueryThreadFunc() {
    auto TmpRequestList = xDnsRequestList();
    auto GetRequest     = [&, this] {
        TmpRequestList.GrabListTail(RequestList);
    };
    while (QueryThreadsRunState) {
        RequestEvent.WaitFor(10ms, GetRequest);
        while (auto P = TmpRequestList.PopHead()) {
            do {  // using sync getaddrinfo
                struct addrinfo hints, *res, *iter;
                memset(&hints, 0, sizeof(hints));
                hints.ai_family   = AF_UNSPEC;    //
                hints.ai_socktype = SOCK_STREAM;  //
                do {
                    int status = getaddrinfo(P->Hostname.c_str(), NULL, &hints, &res);
                    if (status != 0) {
                        DEBUG_LOG("failed to resolve hostname [%s]", P->Hostname.c_str());
                        break;
                    }
                    for (iter = res; iter != NULL; iter = iter->ai_next) {
                        if (iter->ai_family == AF_INET) {  // IPv4
                            struct sockaddr_in * ipv4    = (struct sockaddr_in *)iter->ai_addr;
                            auto                 Address = xel::xNetAddress::Parse(ipv4);
                            P->A4List.push_back(Address);
                            DEBUG_LOG("resolved hostname: %s, address=%s", P->Hostname.c_str(), Address.ToString().c_str());
                        } else if (iter->ai_family == AF_INET6) {  // IPv6
                            struct sockaddr_in6 * ipv6    = (struct sockaddr_in6 *)iter->ai_addr;
                            auto                  Address = xel::xNetAddress::Parse(ipv6);
                            P->A6List.push_back(Address);
                            DEBUG_LOG("resolved hostname: %s, address=%s", P->Hostname.c_str(), Address.ToString().c_str());
                        } else {
                            continue;
                        }
                    }
                    freeaddrinfo(res);
                } while (false);
            } while (false);
            do {
                auto G = xel::xSpinlockGuard(RequestFinishLock);
                RequestFinishedList.AddTail(*P);
            } while (false);
        }
    }
}

bool xDnsLocalService::ResolveDns(const std::string_view & HostnameView, xDnsReultFuture & Future) {
    auto HintIter = CacheMap.find(HostnameView);
    if (HintIter == CacheMap.end()) {
        auto Hostname = std::string(HostnameView);
        DEBUG_LOG("new query, hostname=%s", Hostname.c_str());

        auto SourceRequestNodeId = DnsSourceRequestPool.Acquire();
        if (!SourceRequestNodeId) {
            return false;
        }
        auto & SourceRequestNode              = DnsSourceRequestPool[SourceRequestNodeId];
        SourceRequestNode.SourceRequestNodeId = SourceRequestNodeId;
        SourceRequestNode.FutureHandle        = Future;

        auto RequestNodeId = DnsRequestPool.Acquire();
        if (!RequestNodeId) {
            DnsSourceRequestPool.Release(SourceRequestNodeId);
            return false;
        }
        auto & RequestNode        = DnsRequestPool[RequestNodeId];
        RequestNode.RequestNodeId = RequestNodeId;
        RequestNode.Hostname      = Hostname;

        auto [Iter, Inserted] = CacheMap.insert(std::make_pair(Hostname, nullptr));
        assert(Inserted);
        Pass(Inserted);
        auto & CacheNode       = Iter->second;
        CacheNode              = new xDnsLocalCacheNode();
        CacheNode->Hostname    = Hostname;
        CacheNode->TimestampMS = LocalTicker();
        CacheNode->PendingSourceRequestList.AddTail(SourceRequestNode);
        CacheRequestTimeoutList.AddTail(*CacheNode);

        RequestEvent.Notify([this, &RequestNode] {
            RequestList.AddTail(RequestNode);
            ++Audit.OnGoingDnsRequestCount;
        });

        return true;
    }

    auto & CacheNode = HintIter->second;
    assert(CacheNode->TimestampMS);
    if (!CacheNode->QueryFinished) {  // append request
        DEBUG_LOG("unfinished query, hostname=%s", std::string(HostnameView).c_str());

        auto SourceRequestNodeId = DnsSourceRequestPool.Acquire();
        if (!SourceRequestNodeId) {
            return false;
        }
        auto & SourceRequestNode              = DnsSourceRequestPool[SourceRequestNodeId];
        SourceRequestNode.SourceRequestNodeId = SourceRequestNodeId;
        SourceRequestNode.FutureHandle        = Future;
        CacheNode->PendingSourceRequestList.AddTail(SourceRequestNode);
        return true;
    }

    Future.Result = PickDnsResult(CacheNode);
    Future.SetReady();
    DEBUG_LOG("Cached result: Hostname=%s, A4=%s, A6=%s", std::string(HostnameView).c_str(), Future.Result->A4.IpToString().c_str(), Future.Result->A6.IpToString().c_str());
    return true;
}