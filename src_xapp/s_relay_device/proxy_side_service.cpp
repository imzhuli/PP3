#include "./proxy_side_service.hpp"

#include <pp_common/service_runtime.hpp>

xRelayProxySideService::xRelayProxySideService(const xNetAddress & BindAddress) {
    if (!TcpService.Init(ServiceIoContext, BindAddress, MAX_PROXY_SIDE_CONNECTION_COUNT)) {
        return;
    }
}

xRelayProxySideService::~xRelayProxySideService() {
    if (!IsRaiiReady()) {
        return;
    }
    TcpService.Clean();
}

void xRelayProxySideService::Tick(uint64_t NowMS) {
    TcpService.Tick(NowMS);
}
