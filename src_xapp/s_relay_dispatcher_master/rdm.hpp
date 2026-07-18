#pragma once
#include <pp_common/relay.hpp>

class xRelayDispatcherMaster final : public xRaii {
public:
    xRelayDispatcherMaster(const xel::xNetAddress & RelayEntrydAddress, const xNetAddress & DispatcherEntryAddress);
    ~xRelayDispatcherMaster();

    void Tick(uint64_t NowMS);

private:
    struct xRelayEntryContext {
        uint64_t         ServerId;
        eRelayServerType RelayServerType;
        xNetAddress      DeviceEntryAddress;
        xNetAddress      ProxyEntryAddress;
    };

    struct xDispatcherEntryContext : xListNode {
        uint64_t ContextId;
    };
    using xDispatcherEntryContextList = xList<xDispatcherEntryContext>;

private:
    void OnRelayEntryKeepAlive(const xTcpServiceClientConnectionHandle &);
    void OnRelayEntryClean(const xTcpServiceClientConnectionHandle &);
    bool OnRelayEntryPacket(const xTcpServiceClientConnectionHandle &, xPacketCommandId, xPacketRequestId, ubyte *, size_t);

    void OnDispatcherEntryKeepAlive(const xTcpServiceClientConnectionHandle &);
    void OnDispatcherEntryClean(const xTcpServiceClientConnectionHandle &);
    bool OnDispatcherEntryPacket(const xTcpServiceClientConnectionHandle &, xPacketCommandId, xPacketRequestId, ubyte *, size_t);

private:
    xIndexedStorage<xRelayEntryContext>      RelayEntryContextPool;
    xIndexedStorage<xDispatcherEntryContext> DispatcherEntryContextPool;
    xDispatcherEntryContextList              DispatcherEntryList;

    xTcpService RelayEntry;
    xTcpService DispatcherEntry;
};
