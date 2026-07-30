#include "./device_side_service.hpp"

#include <pp_common/service_runtime.hpp>

xRelayDeviceSideService::xRelayDeviceSideService(const xNetAddress & BindAddress) {
    if (!TcpService.Init(ServiceIoContext, BindAddress, MAX_DEVICE_CONNECTION_COUNT)) {
        return;
    }
    SetRaiiReady();
}

xRelayDeviceSideService::~xRelayDeviceSideService() {
    if (!IsRaiiReady()) {
        return;
    }
    TcpService.Clean();
}

void xRelayDeviceSideService::Tick(uint64_t NowMS) {
    TcpService.Tick(NowMS);
}
