#include "./relay_info_register.hpp"

#include "../../pp_protocol/command.hpp"
#include "../../pp_protocol/p_relay_register.hpp"

#include <pp_common/service_runtime.hpp>

xRelayInfoRegister::xRelayInfoRegister(const xNetAddress & MasterDispatcherAddress, const xRelayInfo & RelayInfo) {
    if (!DispatcherClient.Init(ServiceIoContext, MasterDispatcherAddress)) {
        return;
    }

    DispatcherClient.OnServerConnected = [=, this] {
        auto Register                    = xPP_RelayRegister();
        Register.RelayServerType         = RelayInfo.RelayServerType;
        Register.ExportDeviceSideAddress = RelayInfo.ExportDeviceSideAddress;
        Register.ExportProxySideAddrfess = RelayInfo.ExportProxySideAddrfess;
        DispatcherClient.PostMessage(Cmd_RelayInfoRegister, 0, Register);
    };

    SetRaiiReady();
}

xRelayInfoRegister::~xRelayInfoRegister() {
    if (!IsRaiiReady()) {
        return;
    }
    DispatcherClient.Clean();
    DispatcherClient.OnServerConnected = Noop<>;
}

void xRelayInfoRegister::Tick(uint64_t NowMS) {
    DispatcherClient.Tick(NowMS);
}
