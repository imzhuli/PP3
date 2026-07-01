#pragma once
#include <pp_common/_.hpp>

class xTcpServiceRequestMap final {
public:
    struct xSourceRequest {
        xTcpServiceClientConnectionHandle SourceHandle;
        uint64_t                          SourceRequestId;
        uint64_t                          SourceRequestTimestampMS;
    };

    struct xSourceRequestNode
        : xSourceRequest
        , xListNode {
        uint64_t RequestId;
    };

public:
    struct xAudit {
        size_t TotalRequestCount = 0;
    };

public:
    bool Init(size_t MaxRequest, uint64_t RequestTimeoutMS = 1'000);
    void Clean();
    void Tick(uint64_t NowMS);

    uint64_t       Acquire(xTcpServiceClientConnectionHandle SourceHandle, uint64_t SourceRequestId);
    xSourceRequest GetAndRelease(uint64_t RequestId);
    void           Release(uint64_t RequestId);

    const xAudit & GetAudit() const { return Audit; }

private:
    xTicker                                  LocalTicker;
    uint64_t                                 RequestTimeoutMS = 0;
    xel::xIndexedStorage<xSourceRequestNode> SourceRequestPool;
    xList<xSourceRequestNode>                SourceRequestTimeoutQueue;
    xAudit                                   Audit;
};
