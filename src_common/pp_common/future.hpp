#pragma once
#include "./_.hpp"

#include <expected>

class xFutureManager;
class xFutureBase;
class xFutureHandle;

template <typename T, typename U = ::xel::xNone>
using xExpected = std::expected<T, U>;
template <typename T>
using xUnexpected = std::unexpected<T>;

constexpr const auto UnexpctedResult = xUnexpected(None);

struct xFutureNode : xListNode {
    uint64_t StartTimestampMS = {};
};
using xFutureList = xList<xFutureNode>;

class xFutureManager : xAbstract {
public:
    virtual auto GetFuture(uint64_t FutureId) -> xFutureBase * = 0;
    virtual auto GetReadyFutureList() -> xFutureList &         = 0;
};

struct xFutureBase : xFutureNode {
    xFutureManager * Manager  = {};
    uint64_t         FutureId = {};
    bool             IsReady  = {};

    bool SetReady() {
        assert(!IsReady);
        Manager->GetReadyFutureList().GrabTail(*this);
        return IsReady = true;
    }
};

template <typename T>
    requires std::is_base_of_v<xFutureBase, T>
class xFuturePoolManager
    : public xFutureManager {
public:
    bool   Init(size_t PoolSize) { return FuturePool.Init(PoolSize); };
    void   Clean() { FuturePool.Clean(); }
    size_t GetActiveFutureCount() const { return Audit.FutureInUse; }

    T * AcquireFuture() {
        auto Id = FuturePool.Acquire();
        if (!Id) {
            return nullptr;
        }
        auto Future      = &FuturePool[Id];
        Future->Manager  = this;
        Future->FutureId = Id;
        ++Audit.FutureInUse;
        return Future;
    }
    auto GetFuture(uint64_t FutureId) -> xFutureBase * override { return FuturePool.CheckAndGet(FutureId); }
    auto GetReadyFutureList() -> xFutureList & override { return FutureList; }
    bool ReleaseFuture(uint64_t FutureId) {
        if (FuturePool.CheckAndRelease(FutureId)) {
            --Audit.FutureInUse;
            return true;
        }
        return false;
    }
    void ReleaseFuture(T * Future) {
        assert(Future);
        assert(Future == GetFuture(Future->FutureId));
        FuturePool.Release(Future->FutureId);
        --Audit.FutureInUse;
    }

private:
    xel::xIndexedStorage<T> FuturePool;
    xFutureList             FutureList;
    struct {
        size_t FutureInUse;
    } Audit;
};

struct xFutureHandle final {
public:
    xFutureHandle() = default;
    xFutureHandle(xFutureBase & FutureRef)
        : Manager(FutureRef.Manager), FutureId(FutureRef.FutureId) {
        assert(Manager && FutureId);
    }
    xFutureHandle(xFutureManager * Manager, uint64_t FutureId)
        : Manager(Manager), FutureId(FutureId) {
        assert(Manager && FutureId);
    }
    operator bool() const { return Manager && FutureId; }

    template <typename T>
        requires std::is_base_of_v<xFutureBase, T>
    T * Get() const {
        return static_cast<T *>(Manager ? Manager->GetFuture(FutureId) : nullptr);
    }

private:
    xFutureManager * Manager  = nullptr;
    uint64_t         FutureId = 0;
};
