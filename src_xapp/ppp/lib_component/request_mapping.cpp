#include "./request_mapping.hpp"

#include <pp_common/service_runtime.hpp>

static constexpr const uint64_t MIN_REQUEST_TIMEOUT = 100;
static constexpr const uint64_t MAX_REQUEST_TIMEOUT = 10 * 60'000;

bool xTcpServiceRequestMap::Init(size_t MaxRequest, uint64_t RequestTimeoutMS) {
    LocalTicker.Update();
    this->RequestTimeoutMS = RequestTimeoutMS = std::min(std::max(MIN_REQUEST_TIMEOUT, RequestTimeoutMS), MAX_REQUEST_TIMEOUT);
    if (!SourceRequestPool.Init(RequestTimeoutMS)) {
        Reset(LocalTicker, xTicker(ZeroInit));
        Reset(this->RequestTimeoutMS);
        return false;
    }
    return true;
}

void xTcpServiceRequestMap::Clean() {
    SourceRequestPool.Clean();
    Reset(LocalTicker, xTicker(ZeroInit));
    Reset(RequestTimeoutMS);
}

void xTcpServiceRequestMap::Tick(uint64_t NowMS) {
    LocalTicker.Update(NowMS);
    auto Cond = [KillTimepointMS = uint64_t(NowMS - RequestTimeoutMS)](const xSourceRequest & R) {
        return R.SourceRequestTimestampMS <= KillTimepointMS;
    };
    while (auto P = SourceRequestTimeoutQueue.PopHead(Cond)) {
        // DEBUG_LOG("ReleaseTimeoutRequest: Rid=%" PRIu64 ", SourceConnectionId=%" PRIu64 ", SourceRequestId:%" PRIu64 "", P->RequestId, P->SourceHandle.GetConnectionId(), P->SourceRequestId);
        SourceRequestPool.Release(P->RequestId);
        --Audit.TotalRequestCount;
    }
}

uint64_t xTcpServiceRequestMap::Acquire(xTcpServiceClientConnectionHandle SourceHandle, uint64_t SourceRequestId) {
    auto RequestId = SourceRequestPool.Acquire();
    if (!RequestId) {
        return 0;
    }
    auto & Node                   = SourceRequestPool[RequestId];
    Node.RequestId                = RequestId;
    Node.SourceHandle             = SourceHandle;
    Node.SourceRequestId          = SourceRequestId;
    Node.SourceRequestTimestampMS = LocalTicker();
    SourceRequestTimeoutQueue.AddTail(Node);

    ++Audit.TotalRequestCount;
    return RequestId;
}

xTcpServiceRequestMap::xSourceRequest xTcpServiceRequestMap::GetAndRelease(uint64_t RequestId) {
    auto P = SourceRequestPool.CheckAndGet(RequestId);
    if (!P) {
        return {};
    }
    auto Ret = *X_DEBUG_STEAL(P);
    SourceRequestPool.Release(RequestId);
    --Audit.TotalRequestCount;
    return Ret;
}

void xTcpServiceRequestMap::Release(uint64_t RequestId) {
    assert(SourceRequestPool.Check(RequestId));
    SourceRequestPool.Release(RequestId);
    --Audit.TotalRequestCount;
}
