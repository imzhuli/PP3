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

void xRelayDispatcherSlaveService::DispatchData(xPacketCommandId CmdId, xPacketRequestId ReqId, const void * Payload, size_t PayloadSize) {
    ubyte RebuiltPacket[MaxPacketSize];
    auto  RSize = xel::BuildPacket(RebuiltPacket, CmdId, ReqId, Payload, PayloadSize);
    assert(RSize);

    DEBUG_LOG("DispatchData: \n%s", HexShow(RebuiltPacket, RSize).c_str());
    ClientList.ForEach([=](xRelayDispatcherSlaveClientContext & Context) {
        Context.ClientHandel.PostData(RebuiltPacket, RSize);
    });
}
