#include "./local_device_manager.hpp"

#include "../../const/ppp_const.hpp"

bool xLocalDeviceManager::Init(const std::vector<xLocalDeviceBinding> & LocalDeviceBindingList) {

    assert(LocalDeviceList.empty());
    assert(Ipv4ToDeviceIndex.empty());
    assert(Ipv6ToDeviceIndex.empty());

    if (!ConnectionPool.Init(MAX_TOTAL_DEVICE_CONNECTION_COUNT)) {
        return false;
    }
    if (!UdpChannelPool.Init(MAX_TOTAL_DEVICE_UDPCHANNEL_COUNT)) {
        ConnectionPool.Clean();
        return false;
    }

    size64_t IndexCounter = 0;
    for (auto & B : LocalDeviceBindingList) {
        auto D = xLocalDevice{
            ++IndexCounter,
            B,
        };  // add 1 first, 0 is reserved for "no device found"
        X_RUNTIME_ASSERT(B.BindAddress.Type == B.ExportAddress.Type);
        X_RUNTIME_ASSERT(B.BindAddress && !B.BindAddress.Port && B.ExportAddress && !B.ExportAddress.Port);
        if (B.ExportAddress.Is4()) {
            auto Key = std::to_array(B.ExportAddress.SA4);
            X_RUNTIME_ASSERT(Ipv4ToDeviceIndex.find(Key) == Ipv4ToDeviceIndex.end());
            Ipv4ToDeviceIndex[Key] = IndexCounter;
        } else if (B.ExportAddress.Is6()) {
            auto Key = std::to_array(B.ExportAddress.SA6);
            X_RUNTIME_ASSERT(Ipv6ToDeviceIndex.find(Key) == Ipv6ToDeviceIndex.end());
            Ipv6ToDeviceIndex[Key] = IndexCounter;
        } else {
            xel::FatalPrintf("invalid binding address");
        }
        LocalDeviceList.push_back(D);
    }

    return true;
}

void xLocalDeviceManager::Clean() {
    UdpChannelPool.Clean();
    ConnectionPool.Clean();

    Reset(LocalDeviceList);
    Reset(Ipv4ToDeviceIndex);
    Reset(Ipv6ToDeviceIndex);
}

void xLocalDeviceManager::Tick(uint64_t NowMS) {
    LocalTicker.Update(NowMS);
    ProcessIdleConnections();
    ProcessIdleUdpChannelConnections();
    PerformKillConnections();
    PerformKillUdpChannels();
}

uint64_t xLocalDeviceManager::FindDeviceByExportAddress(const xNetAddress & Address) {
    assert(!Address.Port);
    if (Address.Is4()) {
        auto Iter = Ipv4ToDeviceIndex.find(std::to_array(Address.SA4));
        if (Iter == Ipv4ToDeviceIndex.end()) {
            return 0;
        }
        return Iter->second;
    } else if (Address.Is6()) {
        auto Iter = Ipv6ToDeviceIndex.find(std::to_array(Address.SA6));
        if (Iter == Ipv6ToDeviceIndex.end()) {
            return 0;
        }
        return Iter->second;
    }
    return 0;
}

////////////////////

void xLocalDeviceManager::KeepAlive(xLocalDeviceConnection & Connection) {
    if (Connection.KillMark) {
        return;
    }
    Connection.KeepAliveTimestamp = LocalTicker();
    IdleConnectionList.GrabTail(Connection);
}

void xLocalDeviceManager::KeepAlive(xLocalDeviceUdpChannel & UdpChannel) {
    if (UdpChannel.KillMark) {
        return;
    }
    UdpChannel.KeepAliveTimestamp = LocalTicker();
    IdleUdpChannelList.GrabTail(UdpChannel);
}

void xLocalDeviceManager::DeferKill(xLocalDeviceConnection & Connection) {
    if (Steal(Connection.KillMark, true)) {
        return;
    }
    KillConnectionList.GrabTail(Connection);
}

void xLocalDeviceManager::DeferKill(xLocalDeviceUdpChannel & UdpChannel) {
    if (Steal(UdpChannel.KillMark, true)) {
        return;
    }
    KillUdpChannelList.GrabTail(UdpChannel);
}

void xLocalDeviceManager::ProcessIdleConnections() {
    auto KillTimepoint = LocalTicker() - TARGET_CONNECTION_IDLE_TIMEOUT_MS;
    auto Condition     = [this, KillTimepoint](const xLocalDeviceConnection & C) {
        return C.KeepAliveTimestamp <= KillTimepoint;
    };
    while (auto N = IdleConnectionList.PopHead(Condition)) {
        KillConnectionList.AddTail(*N);
    }
}

void xLocalDeviceManager::ProcessIdleUdpChannelConnections() {
    auto KillTimepoint = LocalTicker() - TARGET_UDPCHANNEL_IDLE_TIMEOUT_MS;
    auto Condition     = [this, KillTimepoint](const xLocalDeviceUdpChannel & C) {
        return C.KeepAliveTimestamp <= KillTimepoint;
    };
    while (auto N = IdleUdpChannelList.PopHead(Condition)) {
        KillUdpChannelList.AddTail(*N);
    }
}

void xLocalDeviceManager::PerformKillConnections() {
    while (auto N = KillConnectionList.PopHead()) {
        assert(N->KillMark);
        N->Clean();
        ConnectionPool.Release(N->LocalId);
    }
}

void xLocalDeviceManager::PerformKillUdpChannels() {
    while (auto N = KillUdpChannelList.PopHead()) {
        assert(N->KillMark);
        N->Clean();
        UdpChannelPool.Release(N->LocalId);
    }
}

//////////////////////////

void xLocalDeviceManager::CreateConnection(uint64_t SourceContextId, uint64_t DeviceId, const xel::xNetAddress & TargetAddress, xDeviceCreateConnectionFuture & Future) {
    Future.SetReady();
}

void xLocalDeviceManager::CreateConnection(uint64_t SourceContextId, uint64_t DeviceId, const std::string & TargetHost, xDeviceCreateConnectionFuture & Future) {
    Future.SetReady();
}

void xLocalDeviceManager::DestroyConnection(uint64_t ConnectionId) {
}

void xLocalDeviceManager::CreateUdpChannel(uint64_t SourceContextId, uint64_t DeviceId, xDeviceCreateUdpChannelFuture & Future) {
    Future.SetReady();
}

void xLocalDeviceManager::DestroyUdpChannel(uint64_t UdpChannelId) {
}

void xLocalDeviceManager::PostData(uint64_t ConnectionId, const void * DataPtr, size_t DataSize) {
}

void xLocalDeviceManager::PostData(uint64_t UdpChannelId, const xNetAddress & TargetAddress, const void * DataPtr, size_t DataSize) {
}
