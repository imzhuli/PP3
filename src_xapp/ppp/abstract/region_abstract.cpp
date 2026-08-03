#include "./region_abstract.hpp"

namespace {

    struct xFakeRegionManager : xFutureManager {
    public:
        xFutureBase * GetFuture(uint64_t FutureId) override { return nullptr; }
        xFutureList & GetReadyFutureList() override { return List; }

    private:
        xFutureList List;
    };

    static xFakeRegionManager FakeRegionManager;

}  // namespace

xRegionFuture::xRegionFuture() {
    Manager = &FakeRegionManager;
}
