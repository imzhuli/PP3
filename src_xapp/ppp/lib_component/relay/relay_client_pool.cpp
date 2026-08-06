#include "./relay_client_pool.hpp"

void xRelayClientPool::OnConnected(xTcpConnection * TcpConnectionPtr) {
}

void xRelayClientPool::OnPeerClose(xTcpConnection * TcpConnectionPtr) {
}

void xRelayClientPool::OnFlush(xTcpConnection * TcpConnectionPtr) {
}

size_t xRelayClientPool::OnData(xTcpConnection * TcpConnectionPtr, ubyte * DataPtr, size_t DataSize) {
    return DataSize;
}
