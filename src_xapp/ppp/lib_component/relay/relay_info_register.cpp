#include "./relay_info_register.hpp"

#include <pp_common/service_runtime.hpp>

xRelayInfoRegister::xRelayInfoRegister(const xNetAddress & MasterDispatcherAddress) {
    if (!DispatcherClient.Init(ServiceIoContext, MasterDispatcherAddress)) {
        return;
    }
    SetRaiiReady();
}

xRelayInfoRegister::~xRelayInfoRegister() {
    if (!IsRaiiReady()) {
        return;
    }
    DispatcherClient.Clean();
}

void xRelayInfoRegister::Tick(uint64_t NowMS) {
    DispatcherClient.Tick(NowMS);
}
