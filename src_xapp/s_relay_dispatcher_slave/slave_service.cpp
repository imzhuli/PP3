#include "./slave_service.hpp"

#include <pp_common/service_runtime.hpp>

bool xRelayDispatcherSlaveService::Init(xIoContext * ICP, const xel::xNetAddress & BindAddress) {
    if (!TcpService.Init(ICP, BindAddress, 20'000)) {
        return false;
    }
    TcpService.OnClientConnected = [this](const xTcpServiceClientConnectionHandle & Handle) {
        auto Context = new xRelayDispatcherSlaveClientContext();
        ClientList.AddTail(*Context);
        Context->ClientHandel = Handle;

        Handle->UserContext.P = Context;
    };
    TcpService.OnClientClean = [this](const xTcpServiceClientConnectionHandle & Handle) {
        auto Context = (xRelayDispatcherSlaveClientContext *)Steal(Handle->UserContext.P);
        delete Context;
    };
    return true;
}

void xRelayDispatcherSlaveService::Clean() {
    TcpService.Clean();
}

void xRelayDispatcherSlaveService::Tick(uint64_t NowMS) {
    TcpService.Tick(NowMS);
}

void xRelayDispatcherSlaveService::DispatchData(const void * Payload, size_t PayloadSize) {
    ClientList.ForEach([=](xRelayDispatcherSlaveClientContext & Context) {
        Context.ClientHandel.PostData(Payload, PayloadSize);
    });
}
