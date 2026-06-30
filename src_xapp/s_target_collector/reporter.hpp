#pragma once

#include <pp_common/_.hpp>

struct xKfkContext;

class xTargetCollectReporter {
public:
    bool Init(const std::string & ConfigFilename);
    void Clean();
    void Tick(uint64_t NowMS);
    void PostTargetCollect(uint64_t GlobalAuthId, const xel::xNetAddress & TargetAddress, const std::string_view & TargetHost, size_t Count);

    std::string GetAuditOutput() const;

private:
    struct xTargetCollectNode
        : xListNode {
        uint64_t    GlobalAuthId;
        xNetAddress TargetAddress;
        std::string TargetHost;
        size_t      Count;

        std::string ToString() const;
    };
    using xTargetCollectList = xList<xTargetCollectNode>;

    void KfkThreadFunc();

private:
    xRunState                       RunState;
    std::thread                     KfkThread;
    //
    xMemoryPool<xTargetCollectNode> NodePool;
    //
    xel::xAutoResetEvent            PostCollectEvent;
    xSpinlock                       SwitchListLock;
    xTargetCollectList              PostCollectionList;
    xTargetCollectList              PendingCollectionList;
    xTargetCollectList              FinishedCollectionList;
    xKfkContext *                   KfkContext = nullptr;
};