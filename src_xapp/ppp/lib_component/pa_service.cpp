#include "./pa_service.hpp"

#include "../const/ppp_const.hpp"

#include <pp_common/service_runtime.hpp>

static constexpr const size_t   PA_CLIENT_AUTH_TIMEOUT_MS        = 5'000;
static constexpr const uint64_t PA_FUTURE_TIMEOUT_MS             = 2'000;
static constexpr const size_t   PA_AUDIT_TIMEOUT_MS              = 5'000;
static constexpr const size_t   PA_GRACEFUL_KILL_TIMEOUT_MS      = 60'000;
static constexpr const size_t   PA_MAX_CLIENT_CONNECTION         = 20'0000;
static constexpr const size_t   PA_MAX_CLIENT_REQUEST_PER_SECOND = 5'0000;
static constexpr const size_t   PA_MAX_UDP_PACKET_SIZE           = 4200;
static constexpr const size_t   PA_UDP_RESERVED_HEADER_SIZE      = 32;
static constexpr const size_t   PA_CLIENT_DEFAULT_BUFFER_SIZE    = 16'000;

static_assert(PA_MAX_UDP_PACKET_SIZE + PA_UDP_RESERVED_HEADER_SIZE < xel::MaxPacketSize);  // core_io buffer size

static ubyte S5_CONNECTION_ESTABLISHED[] = {
    '\x05',                          // version
    '\x00',                          // connection established
    '\x00',                          // reserved
    '\x01',                          // type=ipv4
    '\x00', '\x00', '\x00', '\x00',  // addr.ipv4
    '\x00', '\x00',                  // port
};

static ubyte S5_CONNECTION_GENERIC_ERROR[] = {
    '\x05',                          // version
    '\x01',                          // generic error
    '\x00',                          // reserved
    '\x01',                          // type=ipv4
    '\x00', '\x00', '\x00', '\x00',  // addr.ipv4
    '\x00', '\x00',                  // port
};

static ubyte S5_UDPCHANNEL_GENERIC_ERROR[] = {
    '\x05',                          // version
    '\x01',                          // generic error
    '\x00',                          // reserved
    '\x01',                          // type=ipv4
    '\x00', '\x00', '\x00', '\x00',  // addr.ipv4
    '\x00', '\x00',                  // port
};

static constexpr auto HTTP_200 = "HTTP/1.1 200 Connection Established\r\n\r\n"sv;
static constexpr auto HTTP_404 = "HTTP/1.1 404 Not Found\r\n\r\n"sv;
static constexpr auto HTTP_407 = "HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=\"generic\"\r\nConnection: close\r\n\r\n"sv;
static constexpr auto HTTP_500 = "HTTP/1.1 500 Server Error\r\n\r\n"sv;
static constexpr auto HTTP_502 = "HTTP/1.1 502 Bad Gateway\r\n\r\n"sv;
static constexpr auto HTTP_503 = "HTTP/1.1 503 Service Unavailable\r\nX-Proxy-Error-Code: RESOURCE_EXHAUSTED\r\nRetry-After: 600\r\n\r\n"sv;

//////////

std::string xPA_ClientConnection::xHttpData::ToString() const {
    auto OS = std::ostringstream();
    OS << "Target= " << TargetHost << ":" << TargetPort << endl;
    OS << "RebuiltContent= " << endl
       << Content;
    return OS.str();
}

///////////

bool xProxyAccessService::Init(const std::string & AddressListFilename) {
    auto AddressList = std::vector<xExportBindAddress>();
    auto Lines       = xel::FileToLines(AddressListFilename);
    for (auto & L : Lines) {
        L = Trim(L);
        if (L.empty()) {
            continue;
        }
        auto Segs = Split(L, "->");
        if (Segs.size() != 2) {
            Logger->E("invalid config line: %s", L.c_str());
            return false;
        }
        auto Pair        = xExportBindAddress();
        Pair.BindAddress = xNetAddress::Parse(Trim(Segs[0]));
        Pair.ExportIp    = xNetAddress::Parse(Trim(Segs[1]));
        if (!Pair.BindAddress || !Pair.BindAddress.Port || !Pair.ExportIp || Pair.ExportIp.Port) {
            Logger->E("invalid config line: %s", L.c_str());
            return false;
        }
        AddressList.push_back(Pair);
    }
    return Init(AddressList);
}

bool xProxyAccessService::Init(const std::vector<xExportBindAddress> & AddressList) {
    X_RUNTIME_ASSERT(ServiceRunState);
    X_RUNTIME_ASSERT(AddressList.size());
    if (!ClientConnectionPool.Init(PA_MAX_CLIENT_CONNECTION)) {
        Logger->E("ClientConnectionPool init error");
        return false;
    }
    auto ClientConnectionPoolCleaner = xScopeCleaner(ClientConnectionPool);

    if (!ClientUdpChannelPool.Init(PA_MAX_CLIENT_CONNECTION)) {
        Logger->E("ClientUdpChannelPool init error");
        return false;
    }
    auto ClientUdpChannelPoolCleaner = xScopeCleaner(ClientUdpChannelPool);

    auto ClientEntryCleaner = xScopeGuard([this] {
        for (auto S : ClientEntryServerList) {
            S->Clean();
            delete S;
        }
        Reset(ClientEntryServerList);
    });
    for (auto & Pair : AddressList) {
        X_RUNTIME_ASSERT(Pair.BindAddress && Pair.BindAddress.Port);
        X_RUNTIME_ASSERT(Pair.ExportIp && !Pair.ExportIp.Port);
        auto Server      = new xClientEntryServer();
        Server->ExportIp = Pair.ExportIp;
        if (!Server->Init(ServiceIoContext, Pair.BindAddress, this)) {
            delete Server;
            return false;
        }
        ClientEntryServerList.push_back(Server);
    }

    if (!AuthFutureManager.Init(PA_MAX_CLIENT_REQUEST_PER_SECOND)) {
        Logger->E("AuthFutureManager init error");
        return false;
    }
    auto AuthFutureManagerCleaner = xScopeCleaner(AuthFutureManager);

    if (!AcquireDeviceFutureManager.Init(PA_MAX_CLIENT_REQUEST_PER_SECOND)) {
        Logger->E("AcquireDeviceFutureManager init error");
        return false;
    }
    auto AcquireDeviceFutureManagerCleaner = xScopeCleaner(AcquireDeviceFutureManager);

    if (!AcquireDeviceConnectionFutureManager.Init(PA_MAX_CLIENT_REQUEST_PER_SECOND)) {
        Logger->E("AcquireDeviceConnectionFutureManager init error");
        return false;
    }
    auto AcquireDeviceConnectionFutureManagerCleaner = xScopeCleaner(AcquireDeviceConnectionFutureManager);

    if (!AcquireDeviceUdpChannelFutureManager.Init(PA_MAX_CLIENT_REQUEST_PER_SECOND)) {
        Logger->E("AcquireDeviceUdpChannelFutureManager init error");
        return false;
    }
    auto AcquireDeviceUdpChannelFutureManagerCleaner = xScopeCleaner(AcquireDeviceUdpChannelFutureManager);

    ClientConnectionPoolCleaner.Dismiss();
    ClientUdpChannelPoolCleaner.Dismiss();
    ClientEntryCleaner.Dismiss();

    AuthFutureManagerCleaner.Dismiss();
    AcquireDeviceFutureManagerCleaner.Dismiss();
    AcquireDeviceConnectionFutureManagerCleaner.Dismiss();
    AcquireDeviceUdpChannelFutureManagerCleaner.Dismiss();

    DefaultBufferSize = PA_CLIENT_DEFAULT_BUFFER_SIZE;
    return true;
}

void xProxyAccessService::Clean() {
    AuthFutureManager.Clean();
    AcquireDeviceFutureManager.Clean();
    AcquireDeviceConnectionFutureManager.Clean();
    AcquireDeviceUdpChannelFutureManager.Clean();
    X_VAR xScopeGuard([this] {
        for (auto S : ClientEntryServerList) {
            S->Clean();
            delete S;
        }
        Reset(ClientEntryServerList);
    });
    ClientUdpChannelPool.Clean();
    ClientConnectionPool.Clean();
    Reset(DefaultBufferSize);
    Reset(Audit);
}

void xProxyAccessService::Tick(uint64_t NowMS) {
    LocalTicker.Update(NowMS);
    // process auth results:
    ProcessReadyAuthFuture();
    ProcessReadyAcquireDeviceFuture();
    ProcessReadyAcquireDeviceConnectionFuture();
    ProcessReadyAcquireDeviceUdpChannelFuture();

    DeferKillInitTimeoutConnection();
    DeferKillIdleTimeoutConnection();
    DeferGracefulKillConnection();
    ExcuteKillConnection();
    ClearTimeoutFuture();
}

void xProxyAccessService::ProcessReadyAuthFuture() {
    auto & FutureList = AuthFutureManager.GetReadyFutureList();
    while (auto P = static_cast<xPA_AuthFuture *>(FutureList.PopHead())) {
        DEBUG_LOG();
        OnAuthResult(P);
    }
}

void xProxyAccessService::ProcessReadyAcquireDeviceFuture() {
    auto & FutureList = AcquireDeviceFutureManager.GetReadyFutureList();
    while (auto P = static_cast<xPA_AcquireDeviceFuture *>(FutureList.PopHead())) {
        DEBUG_LOG();
        OnAcquireDeviceResult(P);
    }
}

void xProxyAccessService::ProcessReadyAcquireDeviceConnectionFuture() {
    auto & FutureList = AcquireDeviceConnectionFutureManager.GetReadyFutureList();
    while (auto P = static_cast<xPA_AcquireDeviceConnectionFuture *>(FutureList.PopHead())) {
        DEBUG_LOG();
        OnAcquireDeviceConnectionResult(P);
    }
}

void xProxyAccessService::ProcessReadyAcquireDeviceUdpChannelFuture() {
    auto & FutureList = AcquireDeviceUdpChannelFutureManager.GetReadyFutureList();
    while (auto P = static_cast<xPA_AcquireDeviceUdpChannelFuture *>(FutureList.PopHead())) {
        DEBUG_LOG();
        OnAcquireDeviceUdpChannelResult(P);
    }
}

std::string xProxyAccessService::OutputAudit() {
    auto SS = std::ostringstream();
    SS << "xProxyAccessService:" << endl;
    SS << "\tInvalidS5AuthTypeCount=" << Audit.InvalidS5AuthTypeCount << endl;
    SS << "\tInvalidS5AuthInfo=" << Audit.InvalidS5AuthInfo << endl;
    SS << "\tInvalidS5AuthResult=" << Audit.InvalidS5AuthResult << endl;
    SS << "\tInvalidS5Target=" << Audit.InvalidS5Target << endl;
    SS << "\tLocalBindUdpChannelCount=" << Audit.LocalBindUdpChannelCount << endl;
    SS << "\tRequestAuthenticationOOM=" << Audit.RequestAuthenticationOOM << endl;
    SS << "\tInitConnectionTimeoutCount=" << Audit.InitConnectionTimeoutCount << endl;
    Reset(Audit);
    SS << "\tAuthFutureCount=" << AuthFutureManager.GetActiveFutureCount() << endl;
    SS << "\tAcquireDeviceFutureCount=" << AcquireDeviceFutureManager.GetActiveFutureCount() << endl;
    SS << "\tAcquireDeviceConnectionFutureCount=" << AcquireDeviceConnectionFutureManager.GetActiveFutureCount() << endl;
    SS << "\tAcquireDeviceUdpChannelFutureCount=" << AcquireDeviceUdpChannelFutureManager.GetActiveFutureCount() << endl;
    return SS.str();
}

void xProxyAccessService::SetClientBufferSize(size_t Size) {
    DefaultBufferSize = Size;
}

bool xProxyAccessService::KeepAlive(xPA_ClientConnection * Connection) {
    if (Connection->DeleteMark) {
        return false;
    }
    Connection->TimestampMS = LocalTicker();
    ClientConnectionIdleTimeoutList.GrabTail(*Connection);
    return true;
}

void xProxyAccessService::DeferKill(xPA_ClientConnection * Connection) {
    Reset(Connection->DeleteMark, true);
    ClientConnectionKillList.GrabTail(*Connection);
}

void xProxyAccessService::DeferGracefulKill(xPA_ClientConnection * Connection) {
    if (Steal(Connection->DeleteMark, true)) {
        return;
    }
    if (Connection->HasPendingWrites()) {
        Connection->TimestampMS = LocalTicker();
        ClientConnectionGracefulKillList.GrabTail(*Connection);
    }
}

void xProxyAccessService::ReleaseAuthFuture(xPA_ClientConnection * Connection) {
    auto Future = Steal(Connection->AuthFuture);
    AuthFutureManager.ReleaseFuture(Future);
}

void xProxyAccessService::ReleaseAcquireDeviceFuture(xPA_ClientConnection * Connection) {
    auto Future = Steal(Connection->AcquireDeviceFuture);
    AcquireDeviceFutureManager.ReleaseFuture(Future);
}

void xProxyAccessService::ReleaseAcquireDeviceConnectionFuture(xPA_ClientConnection * Connection) {
    auto Future = Steal(Connection->AcquireDeviceConnectionFuture);
    AcquireDeviceConnectionFutureManager.ReleaseFuture(Future);
}

void xProxyAccessService::ReleaseAcquireDeviceUdpChannelFuture(xPA_ClientConnection * Connection) {
    auto Future = Steal(Connection->AcquireDeviceUdpChannelFuture);
    AcquireDeviceUdpChannelFutureManager.ReleaseFuture(Future);
}

void xProxyAccessService::DestroyConnection(xPA_ClientConnection * Connection) {
    DEBUG_LOG("ConnectionId=%" PRIu64 ", LifeTime=%" PRIu64 "", Connection->ConnectionId, LocalTicker() - Connection->Debug.StartupTimestampMS);
    assert(Connection == ClientConnectionPool.CheckAndGet(Connection->ConnectionId));
    if (Connection->LocalAuthId) {
        AuthService->ReleaseAuthInfo(
            Connection->LocalAuthId,
            {
                .TotalTcpConnections     = 1,
                .TotalTcpBytesFromClient = Connection->TotalTcpBytesFromClient,
                .TotalTcpBytesToClient   = Connection->TotalTcpBytesToClient,
                .TotalUdpChannels        = Connection->BindUdpChannel ? (uint64_t)1 : 0,
                .TotalUdpBytesFromClient = Connection->TotalUdpBytesFromClient,
                .TotalUdpBytesToClient   = Connection->TotalUdpBytesToClient,
            }
        );
    }

    Connection->AuthFuture ? ReleaseAuthFuture(Connection) : Pass();
    Connection->AcquireDeviceFuture ? ReleaseAcquireDeviceFuture(Connection) : Pass();
    Connection->AcquireDeviceConnectionFuture ? ReleaseAcquireDeviceConnectionFuture(Connection) : Pass();
    Connection->AcquireDeviceUdpChannelFuture ? ReleaseAcquireDeviceUdpChannelFuture(Connection) : Pass();

    if (auto UdpChannel = Steal(Connection->BindUdpChannel)) {
        DEBUG_LOG("close bind udp channel");
        assert(UdpChannel == ClientUdpChannelPool.CheckAndGet(UdpChannel->UdpChannelId));
        UdpChannel->Clean();
        ClientUdpChannelPool.Release(UdpChannel->UdpChannelId);
    }

    auto ConnectionId = Connection->ConnectionId;
    Connection->Clean();
    ClientConnectionPool.Release(ConnectionId);
}

void xProxyAccessService::ClearTimeoutFuture() {
    auto KillTimepoint = LocalTicker() - PA_FUTURE_TIMEOUT_MS;
    auto Cond          = [this, KillTimepoint](const xFutureNode & F) {
        return F.StartTimestampMS <= KillTimepoint;
    };
    while (auto P = static_cast<xPA_AuthFuture *>(AuthFutureTimeoutList.PopHead(Cond))) {
        OnAuthResult(P);
    }
    while (auto P = static_cast<xPA_AcquireDeviceFuture *>(AcquireDeviceFutureTimeoutList.PopHead(Cond))) {
        OnAcquireDeviceResult(P);
    }
    while (auto P = static_cast<xPA_AcquireDeviceConnectionFuture *>(AcquireDeviceConnectionFutureTimeoutList.PopHead(Cond))) {
        OnAcquireDeviceConnectionResult(P);
    }
    while (auto P = static_cast<xPA_AcquireDeviceUdpChannelFuture *>(AcquireDeviceUdpChannelFutureTimeoutList.PopHead(Cond))) {
        OnAcquireDeviceUdpChannelResult(P);
    }
}

void xProxyAccessService::DeferKillInitTimeoutConnection() {
    auto KillTimepoint = LocalTicker() - PA_CLIENT_AUTH_TIMEOUT_MS;
    auto Cond          = [this, KillTimepoint](const xPA_ClientConnectionTimeoutNode & N) {
        return N.TimestampMS <= KillTimepoint;
    };
    while (auto P = ClientConnectionInitTimeoutList.PopHead(Cond)) {
        ClientConnectionKillList.AddTail(*P);
        ++Audit.InitConnectionTimeoutCount;
    }
}

void xProxyAccessService::DeferKillIdleTimeoutConnection() {
    auto KillTimepoint = LocalTicker() - CLIENT_CONNECTION_IDLE_TIMEOUT_MS;
    auto Cond          = [this, KillTimepoint](const xPA_ClientConnectionTimeoutNode & N) {
        return N.TimestampMS <= KillTimepoint;
    };
    while (auto P = ClientConnectionIdleTimeoutList.PopHead(Cond)) {
        ClientConnectionKillList.AddTail(*P);
    }
}

void xProxyAccessService::DeferGracefulKillConnection() {
    auto KillTimepoint = LocalTicker() - PA_GRACEFUL_KILL_TIMEOUT_MS;
    auto Cond          = [this, KillTimepoint](const xPA_ClientConnectionTimeoutNode & N) {
        return N.TimestampMS <= KillTimepoint;
    };
    while (auto P = ClientConnectionGracefulKillList.PopHead(Cond)) {
        ClientConnectionKillList.AddTail(*P);
    }
}

void xProxyAccessService::ExcuteKillConnection() {
    while (auto Connection = static_cast<xPA_ClientConnection *>(ClientConnectionKillList.PopHead())) {
        DestroyConnection(Connection);
    }
}

// tcp server listener:
void xProxyAccessService::OnNewConnection(xTcpServer * TcpServerPtr, xSocket && NativeHandle) {
    auto ConnectionId = ClientConnectionPool.Acquire();
    if (!ConnectionId) {
        XelCloseSocket(NativeHandle);
        return;
    }
    auto & ClientConnection = ClientConnectionPool[ConnectionId];
    if (!ClientConnection.Init(ServiceIoContext, std::move(NativeHandle), this)) {
        ClientConnectionPool.Release(ConnectionId);
        return;
    }
    auto ClientEntryServer         = static_cast<xClientEntryServer *>(TcpServerPtr);
    ClientConnection.ConnectionId  = ConnectionId;
    ClientConnection.ExportIp      = ClientEntryServer->ExportIp;
    ClientConnection.DataProcessor = &xProxyAccessService::OnGuessProxyType;
    ClientConnection.TimestampMS   = LocalTicker();
    ClientConnectionInitTimeoutList.AddTail(ClientConnection);

    if (DefaultBufferSize) {
        ClientConnection.ResizeRecvBuffer(DefaultBufferSize);
        ClientConnection.ResizeSendBuffer(DefaultBufferSize);
    }

    ClientConnection.Debug.StartupTimestampMS = LocalTicker();
}

void xProxyAccessService::OnConnected(xTcpConnection * TcpConnection) {
    Unreachable();
}

void xProxyAccessService::OnPeerClose(xTcpConnection * TcpConnection) {
    auto ClientConnection = static_cast<xPA_ClientConnection *>(TcpConnection);
    if (ClientConnection->DeleteMark) {
        return;
    }
    if (ClientConnection->RelaySideConnectionId) {
        assert(ClientConnection->Type == xPA_ClientConnection::eType::S5_TCP || ClientConnection->Type == xPA_ClientConnection::eType::HTTP_NORMAL || ClientConnection->Type == xPA_ClientConnection::eType::HTTP_RAW);
        RelayService->DestroyConnection(ClientConnection->DeviceReference.RelayServerId, ClientConnection->RelaySideConnectionId);
    } else if (ClientConnection->Type == xPA_ClientConnection::eType::S5_UDP && ClientConnection->BindUdpChannel) {
        RelayService->DestroyUdpChannel(ClientConnection->DeviceReference.RelayServerId, ClientConnection->BindUdpChannel->RelaySideUdpChannelId);
    }
    DeferKill(ClientConnection);
}

void xProxyAccessService::OnFlush(xTcpConnection * TcpConnection) {
    auto ClientConnection = static_cast<xPA_ClientConnection *>(TcpConnection);
    if (ClientConnection->DeleteMark) {
        DeferKill(ClientConnection);
        return;
    }
}

size_t xProxyAccessService::OnData(xTcpConnection * TcpConnection, ubyte * DataPtr, size_t DataSize) {
    auto ClientConnection = static_cast<xPA_ClientConnection *>(TcpConnection);
    auto ConsumedSize     = (this->*ClientConnection->DataProcessor)(ClientConnection, DataPtr, DataSize);
    if (ConsumedSize != xel::InvalidDataSize) {
        ClientConnection->TotalTcpBytesFromClient += DataSize;
    }
    return ConsumedSize;
}

void xProxyAccessService::OnData(xUdpChannel * UdpChannelPtr, ubyte * DataPtr, size_t DataSize, const xNetAddress & SourceAddress) {
    auto ClientUdpChannel = static_cast<xPA_ClientUdpChannel *>(UdpChannelPtr);
    if (!ClientUdpChannel->RelaySideUdpChannelId) {  // not ready
        return;
    }
    if (SourceAddress.Ip() != ClientUdpChannel->ClientIpCheckAddress) {
        DEBUG_LOG("failed source ip check");
        return;
    }
    ClientUdpChannel->LastClientIpAddress = SourceAddress;

    auto ParentClientConnection = ClientUdpChannel->ParentConnection;
    assert(ParentClientConnection);

    ParentClientConnection->TotalUdpBytesFromClient += DataSize;
    KeepAlive(ParentClientConnection);

    // extract target info:
    auto R = xel::xStreamReader(DataPtr);
    if (DataSize < 4) {
        return;
    }
    if (R.R2()) {  // RSV
        return;
    }
    if (R.R()) {  // fragmentation not suppurted
        return;
    }
    auto AType         = R.R();
    auto TargetAddress = xNetAddress();
    if (AType == 0x01) {  // ipv4
        if (DataSize < 10) {
            return;
        }
        TargetAddress.Type = xNetAddress::IPV4;
        R.R(TargetAddress.SA4, 4);
        TargetAddress.Port = R.R2();
    } else if (AType == 0x04) {  // ipv6
        if (DataSize < 22) {
            return;
        }
        TargetAddress.Type = xNetAddress::IPV6;
        R.R(TargetAddress.SA6, 16);
        TargetAddress.Port = R.R2();
    } else {
        return;  // drop domain
    }
    auto PayloadSize = DataSize - R.Offset();
    if (!PayloadSize) {  // empty packet
        return;
    }
    RelayService->PostData(ParentClientConnection->DeviceReference.RelayServerId, ClientUdpChannel->RelaySideUdpChannelId, TargetAddress, R, PayloadSize);
}

size_t xProxyAccessService::KillConnectionOnData(xPA_ClientConnection * Connection, ubyte * DataPtr, size_t DataSize) {
    return InvalidDataSize;
}

size_t xProxyAccessService::OnGuessProxyType(xPA_ClientConnection * Connection, ubyte * DataPtr, size_t DataSize) {
    if (DataSize < 3) {
        DEBUG_LOG("invalid challenge size");
        return InvalidDataSize;
    }
    DEBUG_LOG("OnGuessProxyType: ConnectionId=%" PRIu64 "\n%s", Connection->ConnectionId, xel::HexShow({ (const char *)DataPtr, DataSize }).c_str());
    if ('\x05' == (char)DataPtr[0]) {
        return OnS5Challenge(Connection, DataPtr, DataSize);
    }
    if ('\x16' == (char)DataPtr[0]) {  // may be TLS test
        ubyte Buffer[] = { '\x15', '\x03', '\x01', '\x00', '\x02', '\x02', '\x28' };
        Connection->PostData(Buffer, Length(Buffer));
        return 0;
    }
    return OnHttpChallenge(Connection, DataPtr, DataSize);
}

size_t xProxyAccessService::OnS5Challenge(xPA_ClientConnection * Connection, ubyte * DataPtr, size_t DataSize) {
    xStreamReader R(DataPtr);
    R.Skip(1);  // skip type check bytes

    size_t NM = R.R1();  // number of methods
    if (!NM) {
        return InvalidDataSize;
    }
    size_t HeaderSize = 2 + NM;
    if (DataSize < HeaderSize) {
        return InvalidDataSize;
    }
    bool UserPassSupport = false;
    for (size_t i = 0; i < NM; ++i) {
        uint8_t Method = R.R1();
        if (Method == 0x02) {
            UserPassSupport = true;
            break;
        }
    }
    Connection->Type = xPA_ClientConnection::S5_CHALLENGE;
    if (!UserPassSupport) {
        auto RemoteAddress        = Connection->GetRemoteAddress();
        auto Auth                 = '\x00' + RemoteAddress.IpToString();
        Connection->NoAuth        = true;
        Connection->DataProcessor = &xProxyAccessService::KillConnectionOnData;
        auto Future               = RequestAuthentication(Connection, Auth);
        if (!(Connection->AuthFuture = Future)) {
            DEBUG_LOG("AuthFutureManager OOM");
            ++Audit.RequestAuthenticationOOM;
            Connection->PostData("\x05\xFF", 2);  // auth failure
            return HeaderSize;                    // wait for client side close
        }
        return HeaderSize;
    }

    DEBUG_LOG("Accept account/pass authentication");
    ubyte Socks5Auth[2] = { 0x05, 0x02 };
    Connection->PostData(Socks5Auth, sizeof(Socks5Auth));
    Connection->DataProcessor = &xProxyAccessService::OnS5AuthInfo;
    return HeaderSize;
}

size_t xProxyAccessService::OnS5AuthInfo(xPA_ClientConnection * Connection, ubyte * DataPtr, size_t DataSize) {
    DEBUG_LOG("\n%s", HexShow(DataPtr, DataSize).c_str());
    if (DataSize < 3) {
        return 0;
    }
    xStreamReader R(DataPtr);
    auto          Ver = R.R1();
    if (Ver != 0x01) {  // only version for user pass
        ++Audit.InvalidS5AuthTypeCount;
        return InvalidDataSize;
    }
    size_t NameLen = R.R1();
    if (DataSize < 3 + NameLen) {
        return 0;
    }
    char * KeyStart = (char *)DataPtr + R.Offset();
    R.Skip(NameLen);

    size_t PassLen = R.R1();
    if (DataSize < 3 + NameLen + PassLen) {
        return 0;
    }
    ((char *)DataPtr)[R.Offset() - 1] = '\0';  // make quick user/pass key
    R.Skip(PassLen);

    if (!NameLen || !PassLen) {
        ++Audit.InvalidS5AuthInfo;
        return InvalidDataSize;
    }
    Connection->DataProcessor = &xProxyAccessService::KillConnectionOnData;

    size_t KeyLength = NameLen + 1 + PassLen;
    auto   KeyView   = std::string_view{ KeyStart, KeyLength };
    DEBUG_LOG("Auth with Account: %s", StrToHex(KeyView).c_str());
    auto Future = RequestAuthentication(Connection, KeyView);
    if (!(Connection->AuthFuture = Future)) {
        DEBUG_LOG("AuthFutureManager OOM");
        ++Audit.RequestAuthenticationOOM;
        Connection->PostData("\x01\x01", 2);  // auth failure
        return R.Offset();
    }
    return R.Offset();
}

size_t xProxyAccessService::OnS5Target(xPA_ClientConnection * Connection, ubyte * DataPtr, size_t DataSize) {
    if (DataSize < 10) {
        return 0;
    }
    if (DataSize >= 6 + 256) {
        ++Audit.InvalidS5Target;
        return InvalidDataSize;
    }
    DEBUG_LOG("\n%s", HexShow(DataPtr, DataSize).c_str());

    xStreamReader R(DataPtr);
    uint8_t       Version   = R.R();
    uint8_t       Operation = R.R();
    uint8_t       Reserved  = R.R();
    uint8_t       AddrType  = R.R();
    if (Version != 0x05 || Reserved != 0x00) {
        DEBUG_LOG("invalid protocol");
        ++Audit.InvalidS5Target;
        return InvalidDataSize;
    }
    // extract target info:
    auto   TargetAddress = xNetAddress();
    char   DomainName[256];
    size_t DomainNameLength = 0;
    if (AddrType == 0x01) {  // ipv4
        TargetAddress.Type = xNetAddress::IPV4;
        R.R(TargetAddress.SA4, 4);
        TargetAddress.Port = R.R2();

        DEBUG_LOG("target address: %s", TargetAddress.ToString().c_str());
    } else if (AddrType == 0x04) {  // ipv6
        if (DataSize < 4 + 16 + 2) {
            return 0;
        }
        TargetAddress.Type = xNetAddress::IPV6;
        R.R(TargetAddress.SA6, 16);
        TargetAddress.Port = R.R2();

        DEBUG_LOG("target address: %s", TargetAddress.ToString().c_str());
    } else if (AddrType == 0x03) {
        DomainNameLength = R.R();
        if (DataSize < 4 + 1 + DomainNameLength + 2) {
            return 0;
        }
        R.R(DomainName, DomainNameLength);
        DomainName[DomainNameLength] = '\0';
        TargetAddress.Port           = R.R2();
        DEBUG_LOG("target domain: %s, port=%u", DomainName, (unsigned)TargetAddress.Port);
    }

    // the packet is complete
    Connection->DataProcessor = &xProxyAccessService::KillConnectionOnData;
    if (Operation == 0x01) {  // build tcp connection
        DEBUG_LOG("Operation: Connection");
        if (AddrType == 0x01 || AddrType == 0x04) {  // by address
            auto Future = RequestDeviceConnection(Connection, TargetAddress);
            if (!(Connection->AcquireDeviceConnectionFuture = Future)) {
                Connection->PostData(S5_CONNECTION_GENERIC_ERROR, sizeof(S5_CONNECTION_GENERIC_ERROR));
                return 0;
            }
            Connection->Type = xPA_ClientConnection::eType::S5_TCP;
        } else if (AddrType == 0x03) {  // by domain name
            auto Future = RequestDeviceConnection(Connection, { DomainName, DomainNameLength }, TargetAddress.Port);
            if (!(Connection->AcquireDeviceConnectionFuture = Future)) {
                Connection->PostData(S5_CONNECTION_GENERIC_ERROR, sizeof(S5_CONNECTION_GENERIC_ERROR));
                return 0;
            }
            Connection->Type = xPA_ClientConnection::eType::S5_TCP;
        } else {  // invalid address type
            DEBUG_LOG("invalid address type");
            ReleaseAcquireDeviceConnectionFuture(Connection);
            Connection->PostData(S5_CONNECTION_GENERIC_ERROR, sizeof(S5_CONNECTION_GENERIC_ERROR));
            return 0;
        }
    } else if (Operation == 0x03) {  // udp
        if (!Connection->EnableUdpTarget) {
            DEBUG_LOG("udp target not enabled");
            Connection->PostData(S5_UDPCHANNEL_GENERIC_ERROR, sizeof(S5_UDPCHANNEL_GENERIC_ERROR));
            return 0;
        }

        // create local binding udp channel
        const auto & BindIp         = Connection->GetLocalAddress().Ip();
        const auto & UdpBindAddress = ((AddrType == 0x01 && BindIp.Is4()) || ((AddrType == 0x04 && BindIp.Is6())) ? BindIp : xNetAddress{});
        if (!UdpBindAddress) {
            DEBUG_LOG("invalid target address");
            Connection->PostData(S5_UDPCHANNEL_GENERIC_ERROR, sizeof(S5_UDPCHANNEL_GENERIC_ERROR));
            return 0;
        }
        auto UdpChannelId = ClientUdpChannelPool.Acquire();
        if (!UdpChannelId) {
            DEBUG_LOG("udp channel OOM");
            Connection->PostData(S5_UDPCHANNEL_GENERIC_ERROR, sizeof(S5_UDPCHANNEL_GENERIC_ERROR));
            return 0;
        }
        auto & UdpChannel = ClientUdpChannelPool[UdpChannelId];
        if (!UdpChannel.Init(ServiceIoContext, UdpBindAddress, this)) {
            DEBUG_LOG("failed to bind udp channel");
            Connection->PostData(S5_UDPCHANNEL_GENERIC_ERROR, sizeof(S5_UDPCHANNEL_GENERIC_ERROR));
            ClientUdpChannelPool.Release(UdpChannelId);
            return 0;
        }
        UdpChannel.UdpChannelId         = UdpChannelId;
        UdpChannel.ParentConnection     = Connection;
        UdpChannel.ClientIpCheckAddress = Connection->GetRemoteAddress().Ip();
        Connection->BindUdpChannel      = &UdpChannel;

        auto Future = RequestDeviceUdpChannel(Connection);
        if (!(Connection->AcquireDeviceUdpChannelFuture = Future)) {
            DEBUG_LOG();
            Connection->PostData(S5_UDPCHANNEL_GENERIC_ERROR, sizeof(S5_UDPCHANNEL_GENERIC_ERROR));
            return 0;
        }
        Connection->Type = xPA_ClientConnection::eType::S5_UDP;
    } else {
        size_t        AddressLength = R.Offset() - 3;
        ubyte         Buffer[512];
        xStreamWriter W(Buffer);
        W.W(0x05);
        W.W(0x01);
        W.W(0x00);
        W.W((ubyte *)DataPtr + 3, AddressLength);
        Connection->PostData(W.Origin(), W.Offset());
        return 0;
    }
    return R.Offset();
}

size_t xProxyAccessService::OnRawData(xPA_ClientConnection * Connection, ubyte * DataPtr, size_t DataSize) {
    if (!KeepAlive(Connection)) {
        return 0;
    }
    auto & Device = Connection->DeviceReference;
    RelayService->PostData(Device.RelayServerId, Connection->RelaySideConnectionId, DataPtr, DataSize);
    return DataSize;
}

size_t xProxyAccessService::OnHttpChallenge(xPA_ClientConnection * Connection, ubyte * DataPtr, size_t DataSize) {
    std::string_view HostnameView;
    uint16_t         Port       = 0;
    std::string_view DataView   = { (const char *)DataPtr, DataSize };
    auto             LineStart  = DataView.data();
    auto             LineLength = DataView.find("\r\n");

    if (LineLength == 0) {
        DEBUG_LOG("Invalid Http challenge, new line at beginning");
        return InvalidDataSize;
    } else if (LineLength == DataView.npos) {
        return 0;
    }
    if (0 == strncasecmp("CONNECT ", LineStart, 8)) {
        auto LineEnd = LineStart + LineLength;
        LineStart   += 8;  // skip CONNECT & space
        for (auto Curr = LineStart; Curr < LineEnd; ++Curr) {
            char C = *Curr;
            if (C == ':') {
                HostnameView = { LineStart, (size_t)(Curr - LineStart) };
                Port         = atoi(++Curr);
                break;
            } else if (C == ' ') {
                HostnameView = { LineStart, (size_t)(Curr - LineStart) };
                Port         = 80;
                break;
            }
        }
        if (!HostnameView.length()) {
            DEBUG_LOG("Invalid HttpProxy Target");
            return InvalidDataSize;
        }
        Connection->Http.TargetHost = std::string(HostnameView);
        Connection->Http.TargetPort = Port;
        Connection->Type            = xPA_ClientConnection::eType::HTTP_RAW;
        Connection->DataProcessor   = &xProxyAccessService::OnHttpRawHeaderProcessor;

        DEBUG_LOG("HTTP_RAW: %s", Connection->Http.ToString().c_str());
        return LineLength + 2;
    }

    auto LineView      = std::string_view{ LineStart, LineLength };
    auto URLStartIndex = LineView.find(' ');
    if (URLStartIndex == LineView.npos) {
        DEBUG_LOG("Invalid Http Target Line");
        return InvalidDataSize;
    }
    ++URLStartIndex;
    Connection->Http.Content.append(LineStart, URLStartIndex);

    auto URLEndIndex = LineView.find(' ', URLStartIndex);
    if (URLEndIndex == LineView.npos) {
        DEBUG_LOG("Invalid Http Target Line");
        return InvalidDataSize;
    }
    auto URLStart = LineView.data() + URLStartIndex;
    if (0 == strncasecmp(URLStart, "http://", 7)) {
        URLStart      += 7;
        URLStartIndex += 7;
    } else if (0 == strncasecmp(URLStart, "https://", 8)) {
        URLStart      += 8;
        URLStartIndex += 7;
    }

    auto PathStartIndex = LineView.find('/', URLStartIndex);
    if (PathStartIndex == LineView.npos) {
        DEBUG_LOG("Invalid Http Target Line (No Path)");
        return InvalidDataSize;
    }
    // get host:port
    for (auto Curr = URLStart; Curr <= LineStart + PathStartIndex; ++Curr) {
        char C = *Curr;
        if (C == ':') {
            HostnameView = { URLStart, (size_t)(Curr - URLStart) };
            Port         = atoi(++Curr);
            break;
        } else if (C == '/') {
            HostnameView = { URLStart, (size_t)(Curr - URLStart) };
            Port         = 80;
            break;
        }
    }

    Connection->Http.TargetHost = std::string(HostnameView);
    Connection->Http.TargetPort = Port;
    Connection->Http.Content.append(LineStart + PathStartIndex, LineLength - PathStartIndex + 2);
    Connection->Type          = xPA_ClientConnection::eType::HTTP_NORMAL;
    Connection->DataProcessor = &xProxyAccessService::OnHttpNormalHeaderProcessor;

    DEBUG_LOG("HTTP_NORMAL: %s", Connection->Http.ToString().c_str());
    return LineLength + 2;
}

size_t xProxyAccessService::OnHttpRawHeaderProcessor(xPA_ClientConnection * Connection, ubyte * DataPtr, size_t DataSize) {
    auto   AuthKey      = std::string();
    auto   HeaderView   = std::string_view{ (const char *)DataPtr, DataSize };
    size_t ConsumedSize = 0;
    while (true) {
        auto LineEndIndex = HeaderView.find("\r\n");
        if (LineEndIndex == 0) {  // header reading finished
            Connection->DataProcessor = &xProxyAccessService::KillConnectionOnData;

            if (AuthKey.empty()) {
                DEBUG_LOG("Invalid Proxy-Authorization: Not Found!");
                Send407(Connection);
                return 0;
            }
            ConsumedSize += 2;

            DEBUG_LOG("Auth with Account: %s", StrToHex(AuthKey).c_str());
            auto Future = RequestAuthentication(Connection, AuthKey);
            if (!(Connection->AuthFuture = Future)) {
                DEBUG_LOG("Invalid auth future");
                Send500(Connection);
                return 0;
            }
            return ConsumedSize;
        }

        if (LineEndIndex == HeaderView.npos) {
            return ConsumedSize;
        };
        auto LineLength = LineEndIndex + 2;
        ConsumedSize   += LineLength;

        auto LineStart = HeaderView.data();
        if (LineEndIndex > 21 && 0 == strncasecmp(LineStart, "Proxy-Authorization: ", 21)) {
            auto AuthStart  = LineStart + 21;
            auto AuthLength = LineEndIndex - 21;
            if (AuthLength < 6 && 0 != strncasecmp(AuthStart, "Basic ", 6)) {
                DEBUG_LOG("Invalid Proxy-Authorization Request");
                return InvalidDataSize;
            }
            auto Base64Start = AuthStart + 6;
            auto Base64Size  = AuthLength - 6;

            AuthKey = Base64Decode(Base64Start, Base64Size);
            DEBUG_LOG("AuthKey=%s", AuthKey.c_str());
            auto PassStart = AuthKey.find(':');
            if (!PassStart) {
                DEBUG_LOG("HttpReqeust AuthKey Not Found!");
                Send404(Connection);
                return 0;
            }
            AuthKey[PassStart] = '\0';
        }
        HeaderView = HeaderView.substr(LineLength);
    }
    DEBUG_LOG("BUG: Impossible loop exit");
    return InvalidDataSize;
}

size_t xProxyAccessService::OnHttpNormalHeaderProcessor(xPA_ClientConnection * Connection, ubyte * DataPtr, size_t DataSize) {
    auto   ConsumedSize = 0;
    auto   AuthKey      = std::string();
    auto   DataView     = std::string_view{ (const char *)DataPtr, DataSize };
    auto & Http         = Connection->Http;
    auto & LineRef      = Http.TempHeaderLine;
    while (true) {
        // fill line:
        auto LineEndIndex = DataView.find("\r\n");
        if (LineEndIndex == DataView.npos) {
            LineRef.append(DataView.data(), DataView.length());
            return DataSize;
        }
        if (LineEndIndex == 0) {
            if (LineRef.empty()) {
                ConsumedSize += 2;
                break;  // process whole header
            }
        }
        LineRef.append(DataView.data(), LineEndIndex);
        ConsumedSize += LineEndIndex + 2;
        DataView      = DataView.substr(LineEndIndex + 2);

        // process line
        if (LineRef.length() > 18 && 0 == strncasecmp(LineRef.data(), "Proxy-Connection: ", 18)) {
            Pass();
        } else if (LineRef.length() > 12 && 0 == strncasecmp(LineRef.data(), "Connection: ", 12)) {
            Http.Content.append("Connection: close\r\n", 19);
        } else if (LineRef.length() > 21 && 0 == strncasecmp(LineRef.data(), "Proxy-Authorization: ", 21)) {
            auto AuthStart  = LineRef.data() + 21;
            auto AuthLength = LineEndIndex - 21;
            if (AuthLength < 6 && 0 != strncasecmp(AuthStart, "Basic ", 6)) {
                DEBUG_LOG("Invalid Proxy-Authorization Request");
                Send407(Connection);
                return 0;
            }
            auto Base64Start = AuthStart + 6;
            auto Base64Size  = AuthLength - 6;
            AuthKey          = Base64Decode(Base64Start, Base64Size);
            DEBUG_LOG("AuthKey=%s", AuthKey.c_str());
            auto PassStart = AuthKey.find(':');
            if (!PassStart) {
                DEBUG_LOG("HttpReqeust AuthKey Not Found!");
                Send404(Connection);
                return 0;
            }
            AuthKey[PassStart] = '\0';
        } else if (LineRef.length() > 16 && 0 == strncasecmp(LineRef.data(), "Content-Length: ", 16)) {
            Http.ContentLengthLeft = atoll(LineRef.data() + 16);
            Http.Content.append(LineRef);
            Http.Content.append("\r\n", 2);
        } else {
            Http.Content.append(LineRef);
            Http.Content.append("\r\n", 2);
        }
        // clear line:
        LineRef.clear();
    }
    Http.Content.append("\r\n", 2);
    DEBUG_LOG("Http:\n%s", Http.ToString().c_str());

    auto Future = RequestAuthentication(Connection, AuthKey);
    if (!(Connection->AuthFuture = Future)) {
        Send500(Connection);
        return 0;
    }
    Connection->DataProcessor = &xProxyAccessService::OnHttpBody;
    return ConsumedSize;
}

size_t xProxyAccessService::OnHttpBody(xPA_ClientConnection * Connection, ubyte * DataPtr, size_t DataSize) {
    if (!KeepAlive(Connection)) {
        return 0;
    }

    auto & Content = Connection->Http.Content;
    if (Content.size()) {
        Content.append((const char *)DataPtr, DataSize);
        return DataSize;
    }
    assert(Connection->DeviceReference.RelayServerId);
    assert(Connection->RelaySideConnectionId);
    RelayService->PostData(Connection->DeviceReference.RelayServerId, Connection->RelaySideConnectionId, DataPtr, DataSize);
    return DataSize;
}

xPA_AuthFuture * xProxyAccessService::RequestAuthentication(xPA_ClientConnection * Connection, std::string_view AuthView) {
    auto Future = AuthFutureManager.AcquireFuture();
    if (!Future) {
        return nullptr;
    }
    Future->ClientConnection = Connection;
    AuthService->AcquireAuthInfo(AuthView, *Future);
    return Future;
}

xPA_AcquireDeviceFuture * xProxyAccessService::RequestDevice(xPA_ClientConnection * Connection, const xDeviceRequest & Request) {
    auto Future = AcquireDeviceFutureManager.AcquireFuture();
    if (!Future) {
        return nullptr;
    }
    Future->ClientConnection = Connection;
    DeviceLocatorService->AcquireDevice(Request, *Future);
    return Future;
}

xPA_AcquireDeviceConnectionFuture * xProxyAccessService::RequestDeviceConnection(xPA_ClientConnection * Connection, const xNetAddress & TargetAddress) {
    ReportTarget(Connection->GlobalAuthId, TargetAddress.Ip());
    auto Future = AcquireDeviceConnectionFutureManager.AcquireFuture();
    if (!Future) {
        return nullptr;
    }
    Future->ClientConnection = Connection;
    RelayService->CreateConnection(
        Connection->DeviceReference.RelayServerId,      //
        Connection->DeviceReference.RelaySideDeviceId,  //
        Connection->ConnectionId,                       //
        TargetAddress, *Future
    );
    return Future;
}

xPA_AcquireDeviceConnectionFuture * xProxyAccessService::RequestDeviceConnection(xPA_ClientConnection * Connection, const std::string_view & TargetHostnameView, uint16_t TargetPort) {
    ReportTarget(Connection->GlobalAuthId, TargetHostnameView);
    auto Future = AcquireDeviceConnectionFutureManager.AcquireFuture();
    if (!Future) {
        return nullptr;
    }
    Future->ClientConnection = Connection;
    RelayService->CreateConnection(
        Connection->DeviceReference.RelayServerId,      //
        Connection->DeviceReference.RelaySideDeviceId,  //
        Connection->ConnectionId,                       //
        TargetHostnameView, TargetPort, *Future
    );
    return Future;
}

xPA_AcquireDeviceUdpChannelFuture * xProxyAccessService::RequestDeviceUdpChannel(xPA_ClientConnection * Connection) {
    auto Future = AcquireDeviceUdpChannelFutureManager.AcquireFuture();
    if (!Future) {
        return nullptr;
    }
    Future->ClientConnection = Connection;
    RelayService->CreateUdpChannel(
        Connection->DeviceReference.RelayServerId,      //
        Connection->DeviceReference.RelaySideDeviceId,  //
        Connection->BindUdpChannel->UdpChannelId,       //
        *Future
    );
    return Future;
}

void xProxyAccessService::SendS5AuthLimited(xPA_ClientConnection * Connection) {
    Connection->PostData("\x05\xFF", 2);  // no-auth failed
    DEBUG_LOG("resource limited");
}

void xProxyAccessService::SendS5AuthError(xPA_ClientConnection * Connection) {
    if (Connection->NoAuth) {
        Connection->PostData("\x05\xFF", 2);  // no-auth failed
        DEBUG_LOG("Ip white list:no result or no proxy_access found");
    } else {
        Connection->PostData("\x01\x01", 2);  // auth failure
        DEBUG_LOG("no result or no proxy_access found");
    }
}

void xProxyAccessService::SendS5AuthAccepted(xPA_ClientConnection * Connection) {
    if (Connection->NoAuth) {
        Connection->PostData("\x05\x00", 2);
    } else {
        Connection->PostData("\x01\x00", 2);
    }
}

void xProxyAccessService::Send200(xPA_ClientConnection * Connection) {
    Connection->PostData(HTTP_200.data(), HTTP_200.size());
}

void xProxyAccessService::Send404(xPA_ClientConnection * Connection) {
    Connection->PostData(HTTP_404.data(), HTTP_404.size());
}

void xProxyAccessService::Send407(xPA_ClientConnection * Connection) {
    Connection->PostData(HTTP_407.data(), HTTP_407.size());
}

void xProxyAccessService::Send500(xPA_ClientConnection * Connection) {
    Connection->PostData(HTTP_500.data(), HTTP_500.size());
}

void xProxyAccessService::Send502(xPA_ClientConnection * Connection) {
    Connection->PostData(HTTP_502.data(), HTTP_502.size());
}

void xProxyAccessService::Send503(xPA_ClientConnection * Connection) {
    Connection->PostData(HTTP_503.data(), HTTP_503.size());
}

void xProxyAccessService::OnAuthResult(xPA_AuthFuture * Future) {
    auto Connection = Future->ClientConnection;
    assert(Connection->AuthFuture == Future);
    X_SCOPE_GUARD([=, this] {
        ReleaseAuthFuture(Connection);
    });
    if (Connection->Type == xPA_ClientConnection::eType::S5_CHALLENGE) {
        return OnS5AuthResult(Connection, Future);
    }
    if (Connection->Type == xPA_ClientConnection::eType::HTTP_RAW) {
        return OnHttpAuthResult(Connection, Future);
    }
    if (Connection->Type == xPA_ClientConnection::eType::HTTP_NORMAL) {
        return OnHttpAuthResult(Connection, Future);
    }
}

void xProxyAccessService::OnS5AuthResult(xPA_ClientConnection * Connection, xPA_AuthFuture * Future) {
    auto Result = Future->Result ? &*Future->Result : nullptr;
    if (!Result) {
        SendS5AuthError(Connection);
        return;
    }
    Connection->LocalAuthId     = Result->LocalAuthId;
    Connection->GlobalAuthId    = Result->GlobalAuthId;
    Connection->EnableUdpTarget = Result->EnableUdp;

    // Acquire device
    auto DeviceRequest = xDeviceRequest{
        .Condition = {
            .ExportAddress = Result->ExportAddress,
        },
        .Strategy = DSS_STATIC_EXPORT_ADDRESS,
    };
    if (!(Connection->AcquireDeviceFuture = RequestDevice(Connection, DeviceRequest))) {
        DEBUG_LOG();
        SendS5AuthError(Connection);
        return;
    }
}

void xProxyAccessService::OnHttpAuthResult(xPA_ClientConnection * Connection, xPA_AuthFuture * Future) {
    auto Result = Future->Result ? &*Future->Result : nullptr;
    if (!Result) {
        Send407(Connection);
        return;
    }
    Connection->LocalAuthId  = Result->LocalAuthId;
    Connection->GlobalAuthId = Result->GlobalAuthId;

    // Acquire device
    auto DeviceRequest = xDeviceRequest{
        .Condition = {
            .ExportAddress = Result->ExportAddress,
        },
        .Strategy = DSS_STATIC_EXPORT_ADDRESS,
    };
    if (!(Connection->AcquireDeviceFuture = RequestDevice(Connection, DeviceRequest))) {
        DEBUG_LOG();
        Send407(Connection);
        return;
    }
}

void xProxyAccessService::OnAcquireDeviceResult(xPA_AcquireDeviceFuture * Future) {
    auto Connection = Future->ClientConnection;
    assert(Connection->AcquireDeviceFuture == Future);
    X_SCOPE_GUARD([=, this] {
        ReleaseAcquireDeviceFuture(Connection);
    });
    if (Connection->Type == xPA_ClientConnection::eType::S5_CHALLENGE) {
        return OnS5AcquireDeviceResult(Connection, Future);
    }
    if (Connection->Type == xPA_ClientConnection::eType::HTTP_RAW) {
        return OnHttpAcquireDeviceResult(Connection, Future);
    }
    if (Connection->Type == xPA_ClientConnection::eType::HTTP_NORMAL) {
        return OnHttpAcquireDeviceResult(Connection, Future);
    }
}

void xProxyAccessService::OnS5AcquireDeviceResult(xPA_ClientConnection * Connection, xPA_AcquireDeviceFuture * Future) {
    auto & Result = Future->Result;
    if (!Result) {
        SendS5AuthError(Connection);
        return;
    }
    Connection->DeviceReference = *Result;
    Connection->DataProcessor   = &xProxyAccessService::OnS5Target;
    SendS5AuthAccepted(Connection);
}

void xProxyAccessService::OnHttpAcquireDeviceResult(xPA_ClientConnection * Connection, xPA_AcquireDeviceFuture * DeviceFuture) {
    auto & Result = DeviceFuture->Result;
    if (!Result) {
        Send500(Connection);
        return;
    }
    Connection->DeviceReference = *Result;

    // do create connection:
    auto Future = RequestDeviceConnection(Connection, Connection->Http.TargetHost, Connection->Http.TargetPort);
    if (!(Connection->AcquireDeviceConnectionFuture = Future)) {
        Send500(Connection);
        return;
    }
}

void xProxyAccessService::OnAcquireDeviceConnectionResult(xPA_AcquireDeviceConnectionFuture * Future) {
    auto Connection = Future->ClientConnection;
    assert(Connection->AcquireDeviceConnectionFuture == Future);
    X_SCOPE_GUARD([=, this] {
        ReleaseAcquireDeviceConnectionFuture(Connection);
    });
    if (Connection->Type == xPA_ClientConnection::eType::S5_TCP) {
        return OnS5AcquireDeviceConnectionResult(Connection, Future);
    }
    if (Connection->Type == xPA_ClientConnection::eType::HTTP_RAW) {
        return OnHttpRawAcquireDeviceConnectionResult(Connection, Future);
    }
    if (Connection->Type == xPA_ClientConnection::eType::HTTP_NORMAL) {
        return OnHttpNormalAcquireDeviceConnectionResult(Connection, Future);
    }
}

void xProxyAccessService::OnS5AcquireDeviceConnectionResult(xPA_ClientConnection * Connection, xPA_AcquireDeviceConnectionFuture * Future) {
    DEBUG_LOG();
    auto & Result = Future->Result;
    if (!Result) {
        Connection->PostData(S5_CONNECTION_GENERIC_ERROR, sizeof(S5_CONNECTION_GENERIC_ERROR));
        return;
    }
    Connection->RelaySideConnectionId = *Result;
    Connection->DataProcessor         = &xProxyAccessService::OnRawData;
    Connection->PostData(S5_CONNECTION_ESTABLISHED, sizeof(S5_CONNECTION_ESTABLISHED));
}

void xProxyAccessService::OnHttpRawAcquireDeviceConnectionResult(xPA_ClientConnection * Connection, xPA_AcquireDeviceConnectionFuture * Future) {
    DEBUG_LOG();
    auto & Result = Future->Result;
    if (!Result) {
        Send502(Connection);
        return;
    }
    Connection->RelaySideConnectionId = *Result;
    Connection->DataProcessor         = &xProxyAccessService::OnRawData;
    Send200(Connection);
}

void xProxyAccessService::OnHttpNormalAcquireDeviceConnectionResult(xPA_ClientConnection * Connection, xPA_AcquireDeviceConnectionFuture * Future) {
    DEBUG_LOG();
    auto & Result = Future->Result;
    if (!Result) {
        Send502(Connection);
        return;
    }
    Connection->RelaySideConnectionId = *Result;

    auto   RelayServerId         = Connection->DeviceReference.RelayServerId;
    auto   RelaySideConnectionId = Connection->RelaySideConnectionId;
    auto & Content               = Connection->Http.Content;
    RelayService->PostData(RelayServerId, RelaySideConnectionId, Content.data(), Content.size());
    Reset(Content);
}

void xProxyAccessService::OnAcquireDeviceUdpChannelResult(xPA_AcquireDeviceUdpChannelFuture * Future) {
    auto Connection = Future->ClientConnection;
    assert(Connection->AcquireDeviceUdpChannelFuture == Future);
    auto UdpChannel = Connection->BindUdpChannel;
    assert(UdpChannel);
    X_SCOPE_GUARD([=, this] {
        ReleaseAcquireDeviceUdpChannelFuture(Connection);
    });

    assert(Connection->Type == xPA_ClientConnection::eType::S5_UDP);
    auto & Result = Future->Result;
    if (!Result) {
        DEBUG_LOG();
        Connection->PostData(S5_UDPCHANNEL_GENERIC_ERROR, sizeof(S5_UDPCHANNEL_GENERIC_ERROR));
        return;
    }
    UdpChannel->RelaySideUdpChannelId = *Result;
    //
    auto & UdpBindAddress             = UdpChannel->GetBindAddress();
    assert(UdpBindAddress);
    DEBUG_LOG("Enable udp channel at local address: %s", UdpBindAddress.ToString().c_str());

    ubyte Buffer[512];
    auto  W = xStreamWriter(Buffer);
    W.W(0x05);
    W.W(0x00);
    W.W(0x00);
    if (UdpBindAddress.Is4()) {
        W.W(0x01);
        W.W(UdpBindAddress.SA4, 4);
        W.W2(UdpBindAddress.Port);
    } else {
        assert(UdpBindAddress.Is6());
        W.W(0x04);
        W.W(UdpBindAddress.SA6, 16);
        W.W2(UdpBindAddress.Port);
    }
    DEBUG_LOG("SendS5 udp info by connection:\n%s", HexShow(Buffer, W.Offset()).c_str());
    Connection->PostData(Buffer, W.Offset());
    KeepAlive(Connection);
}

void xProxyAccessService::PostData(uint64_t ProxyClientConnectionId, ubyte * DataPtr, size_t DataSize) {
    auto ClientConnection = ClientConnectionPool.CheckAndGet(ProxyClientConnectionId);
    if (!ClientConnection) {
        DEBUG_LOG("Connection lost, ConnectionId=%" PRIu64 "", ProxyClientConnectionId);
        return;
    }
    if (!KeepAlive(ClientConnection)) {
        return;
    }
    // DEBUG_LOG("Connection data ConnectionId=%" PRIu64 ", size=%zi", ProxyClientConnectionId, DataSize);
    assert(!ClientConnection->AcquireDeviceConnectionFuture);  // SHOULD NOT HAPPEN: post data before AcquireDeviceConnectionResult
    ClientConnection->PostData(DataPtr, DataSize);
    ClientConnection->TotalTcpBytesToClient += DataSize;
    return;
}

void xProxyAccessService::PostData(uint64_t ProxyClientUdpChannelId, const xNetAddress & SourceAddress, ubyte * DataPtr, size_t DataSize) {
    auto UdpChannel = ClientUdpChannelPool.CheckAndGet(ProxyClientUdpChannelId);
    if (!UdpChannel) {
        DEBUG_LOG("udp channel lost, UdpChannelId=%" PRIu64 ", size=%zi", ProxyClientUdpChannelId, DataSize);
        return;
    }
    if (DataSize >= PA_MAX_UDP_PACKET_SIZE) {  // drop large packets
        DEBUG_LOG("oversized udp data");
        return;
    }
    if (!UdpChannel->LastClientIpAddress) {
        DEBUG_LOG("client address not determined");
        return;
    }

    auto ClientConnection = UdpChannel->ParentConnection;
    if (!KeepAlive(ClientConnection)) {
        return;
    }

    ubyte Buffer[PA_MAX_UDP_PACKET_SIZE + PA_UDP_RESERVED_HEADER_SIZE];
    auto  W = xStreamWriter(Buffer);
    W.W2(0x00);
    W.W(0);
    if (SourceAddress.Is4()) {
        W.W(0x01);
        W.W(SourceAddress.SA4, 4);
        W.W2(SourceAddress.Port);
    } else if (SourceAddress.Is6()) {
        W.W(0x04);
        W.W(SourceAddress.SA6, 16);
        W.W2(SourceAddress.Port);
    } else {  // drop unknown address type
        return;
    }
    W.W(DataPtr, DataSize);
    DEBUG_LOG("UdpChannel data, UdpChannelId=%" PRIu64 ", from: %s, data=\n%s", ProxyClientUdpChannelId, SourceAddress.ToString().c_str(), HexShow(Buffer, W.Offset()).c_str());
    UdpChannel->PostData(UdpChannel->LastClientIpAddress, Buffer, W.Offset());
    ClientConnection->TotalUdpBytesToClient += W.Offset();
    return;
}

void xProxyAccessService::CloseConnection(uint64_t ProxyClientConnectionId) {
    auto Connection = ClientConnectionPool.CheckAndGet(ProxyClientConnectionId);
    if (!Connection) {
        DEBUG_LOG("Connection lost, ConnectionId=%" PRIu64 "", ProxyClientConnectionId);
        return;
    }
    DeferGracefulKill(Connection);
}

void xProxyAccessService::ReportTarget(uint64_t GlobalAuthId, const xNetAddress & Address) {
    if (!TargetReportService) {
        return;
    }
    TargetReportService->ReportTarget(GlobalAuthId, Address, {}, 1);
}

void xProxyAccessService::ReportTarget(uint64_t GlobalAuthId, const std::string_view & TargetHost) {
    if (!TargetReportService) {
        return;
    }
    TargetReportService->ReportTarget(GlobalAuthId, {}, TargetHost, 1);
}
