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
        //
        ubyte            HeartbeatBuffer[72];
        size_t           HeartbeatDataSize = 0;
    };

    struct xDispatcherSlaveContext : xListNode {
        uint64_t                          ContextId;
        xTcpServiceClientConnectionHandle ConnectionHandle;
    };
    using xDispatcherEntryContextList = xList<xDispatcherSlaveContext>;

private:
    void OnRelayEntryKeepAlive(const xTcpServiceClientConnectionHandle &);
    void OnRelayEntryClean(const xTcpServiceClientConnectionHandle &);
    bool OnRelayEntryPacket(const xTcpServiceClientConnectionHandle &, xPacketCommandId, xPacketRequestId, ubyte *, size_t);

    void OnDispatcherEntryKeepAlive(const xTcpServiceClientConnectionHandle &);
    void OnDispatcherEntryClean(const xTcpServiceClientConnectionHandle &);
    bool OnDispatcherEntryPacket(const xTcpServiceClientConnectionHandle &, xPacketCommandId, xPacketRequestId, ubyte *, size_t);

    void DispatchRelayHeartbeat(xRelayEntryContext & RelayContext);

private:
    xIndexedStorage<xRelayEntryContext>      RelayEntryContextPool;
    xIndexedStorage<xDispatcherSlaveContext> DispatcherEntryContextPool;
    xDispatcherEntryContextList              DispatcherEntryList;
    xHolder<std::mt19937>                    RandomGeneratorHolder;

    xTcpService RelayEntry;
    xTcpService DispatcherEntry;
};
