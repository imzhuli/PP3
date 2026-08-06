#include "./relay_info_observer.hpp"

struct xRelayKeepAliveNode : xListNode {
    uint64_t LastRequestKeepAliveTimestampMS = 0;
    uint64_t LastKeepAliveRespTimestampMS    = 0;
};

class xRelayConnection final
    : public xel::xTcpConnection
    , public xRelayKeepAliveNode {
    uint64_t ServerId = 0;
};

class xRelayClientPool final
    : public xRaii
    , public xTcpConnection::iListener {
public:
    xRelayClientPool();
    ~xRelayClientPool();

private:
    void   OnConnected(xTcpConnection * TcpConnectionPtr) override;
    void   OnPeerClose(xTcpConnection * TcpConnectionPtr) override;
    void   OnFlush(xTcpConnection * TcpConnectionPtr) override;
    size_t OnData(xTcpConnection * TcpConnectionPtr, ubyte * DataPtr, size_t DataSize) override;

private:
    std::array<xRelayConnection, MAX_RELAY_SERVER_LIST_SIZE> RelayConnectionPool;
    xList<xRelayKeepAliveNode>                               RelayConnectionKeepAliveTimeoutList;
};
