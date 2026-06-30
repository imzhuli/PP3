#include "./relay_local_device_service.hpp"

#include <pp_common/service_runtime.hpp>

static constexpr const size_t DEVICE_ID_HIGH32_MAGIC           = 0xCDEF7788;
static constexpr const size_t MAX_MANAGED_CONNECTION_SIZE      = 15'0000;
static constexpr const size_t MAX_MANAGED_UDPCHANNEL_SIZE      = 10'0000;
static constexpr const size_t IDLE_CONNECTION_TIMEOUT_MS       = 125'000;
static constexpr const size_t IDLE_UDPCHANNEL_TIMEOUT_MS       = 125'000;
static constexpr const size_t MAX_DNS_FUTURE_COUNT             = 1'0000;
static constexpr const size_t LOCAL_DEVICE_DEFAULT_BUFFER_SIZE = 16'000;

static uint64_t MakeLocalDeviceId(size_t Index) {
    X_RUNTIME_ASSERT(Index < std::numeric_limits<uint32_t>::max());
    return Make64(DEVICE_ID_HIGH32_MAGIC, Index);
}

static size_t ExtractIndexFromDeviceId(uint64_t DeviceId) {
    assert((DeviceId >> 32) == DEVICE_ID_HIGH32_MAGIC);
    return DeviceId & 0xFFFFFFFF;
}

static size_t EstimateMaxConnectionPoolSize(size_t LocalAddressSize) {
    return std::min(4'0000 * LocalAddressSize, MAX_MANAGED_CONNECTION_SIZE);
}
static size_t EstimateMaxUdpChannelPoolSize(size_t LocalAddressSize) {
    return std::min(6'0000 * LocalAddressSize, MAX_MANAGED_UDPCHANNEL_SIZE);
}

std::string xRelayLocalBindingOption::ToString() const {
    auto OS = std::ostringstream();
    OS << "[ ";
    OS << "B:" << BindAddress.IpToString() << " ";
    OS << "E:" << ExportAddress.IpToString() << " ";
    OS << (EnableUdp ? "udp" : "no-udp") << " ";
    OS << "]";
    return OS.str();
}

std::string xRelayLocalDevice::ToString() const {
    auto OS = std::ostringstream();
    OS << "[ ";
    OS << "DeviceId:" << DeviceId << " ";
    OS << "B:" << BindAddress.IpToString() << " ";
    OS << "E:" << ExportAddress.IpToString() << " ";
    OS << "EnableTcp:" << YN(EnableTcp) << " ";
    OS << "EnableUdp:" << YN(EnableUdp) << " ";
    OS << "]";
    return OS.str();
}

bool xRelayLocalBindingService::Init(uint64_t ServerId, const std::string & AddressPairFile) {
    // parse file:
    auto BindAddressPairList = std::vector<xRelayLocalBindingOption>();
    auto Lines               = xel::FileToLines(AddressPairFile);
    for (auto L : Lines) {
        L = xel::Trim(L);
        if (L.empty()) {
            continue;
        }
        DEBUG_LOG("AddressPairLine:%s", L.c_str());
        auto Segs = xel::Split(L, "|");
        // [0]: addr4 mapping
        // [1]: addr6 mapping
        // [2]: functions
        if (Segs.size() != 2) {
            Logger->F("invalid config line");
            return false;
        }
        auto AM     = Split(Segs[0], "->");
        auto FN     = Split(Segs[1], ",");
        auto Option = xRelayLocalBindingOption();
        X_RUNTIME_ASSERT(AM.size() == 2);
        auto B = xNetAddress::Parse(Trim(AM[0]));
        auto E = xNetAddress::Parse(Trim(AM[1]));
        X_RUNTIME_ASSERT(B && E);
        Option.BindAddress   = B;
        Option.ExportAddress = E;
        do {
            for (auto & F : FN) {
                F = Trim(F);
                if (!strcmp(F.c_str(), "tcp")) {  // enable tcp
                    X_RUNTIME_ASSERT(!Steal(Option.EnableTcp, true));
                }
                if (!strcmp(F.c_str(), "udp")) {  // enable tcp
                    X_RUNTIME_ASSERT(!Steal(Option.EnableUdp, true));
                }
                X_RUNTIME_ASSERT(Option.EnableTcp || Option.EnableUdp);
            }
        } while (false);
        BindAddressPairList.push_back(Option);
    }
    return Init(ServerId, BindAddressPairList);
}

bool xRelayLocalBindingService::Init(uint64_t ServerId, const std::vector<xRelayLocalBindingOption> & BindAddressPairList) {
    X_RUNTIME_ASSERT(ServerId);
    X_RUNTIME_ASSERT(ServiceRunState);
    if (!CreateLocalDeviceList(BindAddressPairList)) {
        Logger->E("CreateLocalDeviceList error");
        return false;
    }
    if (!LocalConnectionPool.Init(EstimateMaxConnectionPoolSize(BindAddressPairList.size()))) {
        Logger->E("LocalConnectionPool error");
        DestroyLocalDeviceList();
        return false;
    }
    if (!LocalUdpChannelPool.Init(EstimateMaxUdpChannelPoolSize(BindAddressPairList.size()))) {
        Logger->E("LocalUdpChannelPool error");
        LocalConnectionPool.Clean();
        DestroyLocalDeviceList();
        return false;
    }
    if (!DnsFutureManager.Init(MAX_DNS_FUTURE_COUNT)) {
        Logger->E("DnsFutureManager error");
        LocalUdpChannelPool.Clean();
        LocalConnectionPool.Clean();
        DestroyLocalDeviceList();
        return false;
    }

    LocalRelayServerId = ServerId;
    DefaultBufferSize  = LOCAL_DEVICE_DEFAULT_BUFFER_SIZE;
    return true;
}

void xRelayLocalBindingService::Clean() {
    DnsFutureManager.Clean();
    Reset(DnsService);
    CleanAllConnections();
    CleanAllUdpChannels();
    LocalUdpChannelPool.Clean();
    LocalConnectionPool.Clean();
    DestroyLocalDeviceList();

    Reset(DefaultBufferSize);
    Reset(Audit);
}

void xRelayLocalBindingService::SetDeviceBufferSize(size_t Size) {
    DefaultBufferSize = Size;
}

void xRelayLocalBindingService::BindProxyService(xProxyAbstractService * ProxyService) {
    X_RUNTIME_ASSERT(!Steal(this->ProxyService, ProxyService));
}

void xRelayLocalBindingService::BindDnsService(xDnsAbstractService * DnsService) {
    X_RUNTIME_ASSERT(!Steal(this->DnsService, DnsService));
}

void xRelayLocalBindingService::Tick(uint64_t NowMS) {
    LocalTicker.Update(NowMS);
    ProcessDnsResults();
    DeferDestroyIdleConnections();
    DeferDestroyIdleUdpChannels();
    CleanDyingConnections();
    CleanDyingUdpChannels();
}

std::string xRelayLocalBindingService::OutputAudit() const {
    auto OS = std::ostringstream();
    OS << "xRelayLocalBindingService: " << endl;
    OS << "\tConnectionCount=" << Audit.ConnectionCount << endl;
    OS << "\tUdpChannelCount=" << Audit.UdpChannelCount << endl;
    OS << "\tDnsFutureCount=" << DnsFutureManager.GetActiveFutureCount() << endl;
    return OS.str();
}

bool xRelayLocalBindingService::CreateLocalDeviceList(const std::vector<xRelayLocalBindingOption> & OptionList) {
    X_RUNTIME_ASSERT(LocalDeviceList.empty());
    size_t IndexCount = 0;
    for (auto & O : OptionList) {
        auto DInfo = xRelayLocalDevice{
            .DeviceId      = MakeLocalDeviceId(IndexCount++),
            .BindAddress   = O.BindAddress,
            .ExportAddress = O.ExportAddress,
            .EnableTcp     = O.EnableTcp,
            .EnableUdp     = O.EnableUdp,
        };
        auto & E = LocalDeviceExportAddressMap[DInfo.ExportAddress];
        X_RUNTIME_ASSERT(!Steal(E, DInfo.DeviceId));
        LocalDeviceList.push_back(DInfo);
    }
    X_RUNTIME_ASSERT(LocalDeviceList.size() == LocalDeviceExportAddressMap.size());
    for (auto & D : LocalDeviceList) {
        AuditLogger->I("EnableDevice:\n\t%s", D.ToString().c_str());
    }
    return true;
}

void xRelayLocalBindingService::DestroyLocalDeviceList() {
    Reset(LocalDeviceList);
    Reset(LocalDeviceExportAddressMap);
}

const xRelayLocalDevice * xRelayLocalBindingService::GetDevice(uint64_t DeviceId) const {
    auto H32 = High32(DeviceId);
    auto L32 = Low32(DeviceId);
    if (H32 != DEVICE_ID_HIGH32_MAGIC) {
        return nullptr;
    }
    if (L32 >= LocalDeviceList.size()) {
        return nullptr;
    }
    return &LocalDeviceList[L32];
}

void xRelayLocalBindingService::AcquireDevice(const xDeviceRequest & Request, xAcquireDeviceFuture & Future) {
    Future.SetReady();
    if (!(Request.Strategy & DSS_STATIC_EXPORT_ADDRESS)) {
        DEBUG_LOG("only DSS_STATIC_EXPORT_ADDRESS strategy is supported");
        return;
    }
    auto Iter = LocalDeviceExportAddressMap.find(Request.Condition.ExportAddress);
    if (Iter == LocalDeviceExportAddressMap.end()) {
        DEBUG_LOG("no device found by export address");
        return;
    }
    Future.Result = {
        .RelayServerId     = 0,
        .RelaySideDeviceId = Iter->second,
    };
    return;
}

void xRelayLocalBindingService::CreateConnection(uint64_t RelayServerId, uint64_t DeviceId, uint64_t ProxySideConnectionId, const std::string_view & TargetHostnameView, uint16_t TargetPort, xRelayCreateConnectionFuture & Future) {
    auto DnsFuture = DnsFutureManager.AcquireFuture();
    if (!DnsFuture) {
        Future.SetReady();
        return;
    }
    DnsFuture->RelayServerId         = RelayServerId;
    DnsFuture->RelaySideDeviceId     = DeviceId;
    DnsFuture->ProxySideConnectionId = ProxySideConnectionId;
    DnsFuture->TargetPort            = TargetPort;

    if (!DnsService->ResolveDns(TargetHostnameView, *DnsFuture)) {
        DnsFutureManager.ReleaseFuture(DnsFuture);
        Future.SetReady();
        return;
    }
    DnsFuture->CreateConnectionFutureHandle = Future;
}

void xRelayLocalBindingService::CreateConnection(uint64_t RelayServerId, uint64_t DeviceId, uint64_t ProxySideConnectionId, const xNetAddress & TargetAddress, xRelayCreateConnectionFuture & Future) {
    assert(ProxySideConnectionId);
    auto Device = GetDevice(DeviceId);
    if (!Device) {
        Future.SetReady();
        return;
    }
    CreateConnection(ProxySideConnectionId, Device->BindAddress, TargetAddress, Future);
    return;
}

void xRelayLocalBindingService::CreateUdpChannel(uint64_t RelayServerId, uint64_t DeviceId, uint64_t ProxySideUdpChannelId, xRelayCreateUdpChannelFuture & Future) {
    Future.SetReady();  // always has immediate result
    auto Device = GetDevice(DeviceId);
    if (!Device) {
        return;
    }
    auto UdpChannelId = LocalUdpChannelPool.Acquire();
    if (!UdpChannelId) {
        return;
    }
    auto & UdpChannel = LocalUdpChannelPool[UdpChannelId];
    if (!UdpChannel.Init(ServiceIoContext, Device->BindAddress, this)) {
        LocalUdpChannelPool.Release(UdpChannelId);
        return;
    }
    UdpChannel.UdpChannelId          = UdpChannelId;
    UdpChannel.ProxySideUdpChannelId = ProxySideUdpChannelId;
    Future.Result                    = UdpChannelId;

    ++Audit.UdpChannelCount;
    return;
}

void xRelayLocalBindingService::DestroyConnection(uint64_t RelayServerId, uint64_t ConnectionId) {
    auto Connection = LocalConnectionPool.CheckAndGet(ConnectionId);
    if (!Connection) {
        return;
    }
    // clear pa side futures:
    Connection->ClearFutures();
    DeferDestroyConnection(Connection);
}

void xRelayLocalBindingService::DestroyUdpChannel(uint64_t RelayServerId, uint64_t UdpChannelId) {
    auto UdpChannel = LocalUdpChannelPool.CheckAndGet(UdpChannelId);
    if (!UdpChannel) {
        return;
    }
    DeferDestroyUdpChannel(UdpChannel);
}

void xRelayLocalBindingService::KeepUdpChannelAlive(uint64_t RelayServerId, uint64_t UdpChannelId) {
    auto UdpChannel = LocalUdpChannelPool.CheckAndGet(UdpChannelId);
    if (!UdpChannel) {
        return;
    }
    KeepAlive(UdpChannel);
}

////////////////////////////////

bool xRelayLocalBindingService::KeepAlive(xRelayLocalDeviceConnection * Connection) {
    if (Connection->DeleteMark) {
        return false;
    }
    Connection->TimestampMS = LocalTicker();
    ConnectionIdleTimeoutList.GrabTail(*Connection);
    return true;
}

bool xRelayLocalBindingService::KeepAlive(xRelayLocalDeviceUdpChannel * UdpChannel) {
    if (UdpChannel->DeleteMark) {
        return false;
    }
    UdpChannel->TimestampMS = LocalTicker();
    UdpChannelIdleTimeoutList.GrabTail(*UdpChannel);
    return true;
}

void xRelayLocalBindingService::DeferDestroyConnection(xRelayLocalDeviceConnection * Connection) {
    assert(Connection == LocalConnectionPool.CheckAndGet(Connection->ConnectionId));
    if (!Steal(Connection->DeleteMark, true)) {
        DEBUG_LOG("ConnectionId=%" PRIu64 "", Connection->ConnectionId);
        if (auto Future = Steal(Connection->CreateConnectionFutureHandle).Get<xRelayCreateConnectionFuture>()) {
            Future->SetReady();
        }
        ConnectionKillList.GrabTail(*Connection);
    }
}

void xRelayLocalBindingService::DeferDestroyUdpChannel(xRelayLocalDeviceUdpChannel * UdpChannel) {
    assert(UdpChannel == LocalUdpChannelPool.CheckAndGet(UdpChannel->UdpChannelId));
    Reset(UdpChannel->DeleteMark, true);
    UdpChannelKillList.GrabTail(*UdpChannel);
}

void xRelayLocalBindingService::CreateConnection(uint64_t ProxySideConnectionId, const xNetAddress & DeviceBindAddress, const xNetAddress & TargetAddress, xRelayCreateConnectionFuture & Future) {
    auto ConnectionId = LocalConnectionPool.Acquire();
    if (!ConnectionId) {
        Future.SetReady();
        return;
    }
    auto & Connection = LocalConnectionPool[ConnectionId];
    if (!Connection.Init(ServiceIoContext, TargetAddress, DeviceBindAddress, this)) {
        LocalConnectionPool.Release(ConnectionId);
        Future.SetReady();
        return;
    }
    Connection.ConnectionId                 = ConnectionId;
    Connection.ProxySideConnectionId        = ProxySideConnectionId;
    Connection.CreateConnectionFutureHandle = xFutureHandle(Future);

    if (DefaultBufferSize) {
        Connection.ResizeRecvBuffer(DefaultBufferSize);
        Connection.ResizeSendBuffer(DefaultBufferSize);
    }

    ++Audit.ConnectionCount;
}

void xRelayLocalBindingService::CreateConnectionWithDnsResult(xRelayDnsResultFuture * DnsFuture) {
    assert(DnsFuture);
    auto Future = DnsFuture->CreateConnectionFutureHandle.Get<xRelayCreateConnectionFuture>();
    if (!Future) {
        DEBUG_LOG("source request lost");
        return;
    }

    auto & Result = DnsFuture->Result;
    if (Result) {
        auto Device = GetDevice(DnsFuture->RelaySideDeviceId);
        if (!Device) {
            DEBUG_LOG("device lost");
            Future->SetReady();
            return;
        }
        auto TargetAddress = Device->BindAddress.Is4() ? Result->A4 : Result->A6;
        if (!TargetAddress) {
            DEBUG_LOG("no target address found");
            Future->SetReady();
            return;
        }
        TargetAddress.Port = DnsFuture->TargetPort;
        CreateConnection(DnsFuture->RelayServerId, DnsFuture->RelaySideDeviceId, DnsFuture->ProxySideConnectionId, TargetAddress, *Future);
    } else {
        DEBUG_LOG("no dns result found");
        Future->SetReady();
    }
}

void xRelayLocalBindingService::ProcessDnsResults() {
    auto & DnsReadyList = DnsFutureManager.GetReadyFutureList();
    while (auto DnsFuture = static_cast<xRelayDnsResultFuture *>(DnsReadyList.PopHead())) {
        auto Future = DnsFuture->CreateConnectionFutureHandle.Get<xRelayCreateConnectionFuture>();
        if (!Future) {
            DEBUG_LOG("Dns result source request future lost");
            // pass through
        } else {
            CreateConnectionWithDnsResult(DnsFuture);
        }
        DnsFutureManager.ReleaseFuture(DnsFuture);
    }
}

void xRelayLocalBindingService::DeferDestroyIdleConnections() {
    auto NowMS = LocalTicker();
    auto Cond  = [KillTimepoint = NowMS - IDLE_CONNECTION_TIMEOUT_MS](const xRelayLocalDeviceConnectionTimeoutNode & N) {
        return N.TimestampMS <= KillTimepoint;
    };
    while (auto P = static_cast<xRelayLocalDeviceConnection *>(ConnectionIdleTimeoutList.PopHead(Cond))) {
        DeferDestroyConnection(P);
    }
}

void xRelayLocalBindingService::DeferDestroyIdleUdpChannels() {
    auto NowMS = LocalTicker();
    auto Cond  = [KillTimepoint = NowMS - IDLE_CONNECTION_TIMEOUT_MS](const xRelayLocalDeviceUdpChannelTimeoutNode & N) {
        return N.TimestampMS <= KillTimepoint;
    };
    while (auto P = static_cast<xRelayLocalDeviceUdpChannel *>(UdpChannelIdleTimeoutList.PopHead(Cond))) {
        DeferDestroyUdpChannel(P);
    }
}

void xRelayLocalBindingService::CleanDyingConnections() {
    while (auto P = static_cast<xRelayLocalDeviceConnection *>(ConnectionKillList.PopHead())) {
        DEBUG_LOG("ConnectionId=%" PRIu64 "", P->ConnectionId);
        P->Clean();
        LocalConnectionPool.Release(P->ConnectionId);
        --Audit.ConnectionCount;
    }
}

void xRelayLocalBindingService::CleanDyingUdpChannels() {
    while (auto P = static_cast<xRelayLocalDeviceUdpChannel *>(UdpChannelKillList.PopHead())) {
        P->Clean();
        LocalUdpChannelPool.Release(P->UdpChannelId);
        --Audit.UdpChannelCount;
    }
}

void xRelayLocalBindingService::CleanAllConnections() {
    ConnectionKillList.GrabListTail(ConnectionEstablishTimeoutList);
    ConnectionKillList.GrabListTail(ConnectionIdleTimeoutList);
    CleanDyingConnections();
}

void xRelayLocalBindingService::CleanAllUdpChannels() {
    UdpChannelKillList.GrabListTail(UdpChannelIdleTimeoutList);
    CleanDyingUdpChannels();
}

////////////////////////////////

const xRelayLocalDevice * xRelayLocalBindingService::FindDeviceByExportAddress(const xNetAddress & ExportAddress) {
    auto Iter = LocalDeviceExportAddressMap.find(ExportAddress);
    if (Iter == LocalDeviceExportAddressMap.end()) {
        return nullptr;
    }
    auto DeviceId    = Iter->second;
    auto DeviceIndex = ExtractIndexFromDeviceId(DeviceId);
    return &LocalDeviceList[DeviceIndex];
}

////////////////////////////////

void xRelayLocalBindingService::OnConnected(xTcpConnection * TcpConnectionPtr) {
    auto Connection = static_cast<xRelayLocalDeviceConnection *>(TcpConnectionPtr);
    if (auto Future = !Connection->CreateConnectionFutureHandle ? nullptr : Steal(Connection->CreateConnectionFutureHandle).Get<xRelayCreateConnectionFuture>()) {
        Future->Result = Connection->ConnectionId;
        Future->SetReady();
        KeepAlive(Connection);
    }
}

void xRelayLocalBindingService::OnPeerClose(xTcpConnection * TcpConnectionPtr) {
    auto Connection = static_cast<xRelayLocalDeviceConnection *>(TcpConnectionPtr);
    ProxyService->CloseConnection(Connection->ProxySideConnectionId);
    DeferDestroyConnection(Connection);
}

size_t xRelayLocalBindingService::OnData(xTcpConnection * TcpConnectionPtr, ubyte * DataPtr, size_t DataSize) {
    auto Connection = static_cast<xRelayLocalDeviceConnection *>(TcpConnectionPtr);
    if (!KeepAlive(Connection)) {
        DEBUG_LOG("connection lost");
        return 0;
    }
    ProxyService->PostData(Connection->ProxySideConnectionId, DataPtr, DataSize);
    return DataSize;
}

void xRelayLocalBindingService::OnData(xUdpChannel * ChannelPtr, ubyte * DataPtr, size_t DataSize, const xNetAddress & RemoteAddress) {
    DEBUG_LOG();
    auto UdpChannel = static_cast<xRelayLocalDeviceUdpChannel *>(ChannelPtr);
    assert(UdpChannel->ProxySideUdpChannelId);
    ProxyService->PostData(UdpChannel->ProxySideUdpChannelId, RemoteAddress, DataPtr, DataSize);
}

void xRelayLocalBindingService::PostData(uint64_t RelayServerId, uint64_t ConnectionId, const void * Payload, size_t PayloadSize) {
    auto Connection = LocalConnectionPool.CheckAndGet(ConnectionId);
    if (!Connection || Connection->DeleteMark || !Connection->IsConnected()) {
        DEBUG_LOG("Connection lost");
        return;
    }
    DEBUG_LOG("Post connection data: size=%zi", PayloadSize);
    Connection->PostData(Payload, PayloadSize);
}

void xRelayLocalBindingService::PostData(uint64_t RelayServerId, uint64_t UdpChannelId, const xel::xNetAddress & TargetAddress, const void * Payload, size_t PayloadSize) {
    auto UdpChannel = LocalUdpChannelPool.CheckAndGet(UdpChannelId);
    if (!UdpChannel || UdpChannel->DeleteMark) {
        DEBUG_LOG("UdpChannel lost");
        return;
    }
    DEBUG_LOG("Post udp channel data: from=%s to=%s, data=\n%s", UdpChannel->GetBindAddress().ToString().c_str(), TargetAddress.ToString().c_str(), HexShow(Payload, PayloadSize).c_str());
    UdpChannel->PostData(TargetAddress, Payload, PayloadSize);
}
