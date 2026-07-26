#pragma once
#include <pp_common/_.hpp>

struct xRelayDispatcherSlaveClientContext : xListNode {
    xel::xTcpServiceClientConnectionHandle ClientHandel;
};

class xRelayDispatcherSlaveService final : xel::xNonCopyable {

public:
    bool Init(xIoContext * ICP, const xel::xNetAddress & BindAddress);
    void Clean();
    void Tick(uint64_t NowMS);

    void DispatchData(const void * Payload, size_t PayloadSize);

private:
    xTcpService                               TcpService;
    xList<xRelayDispatcherSlaveClientContext> ClientList;
};
