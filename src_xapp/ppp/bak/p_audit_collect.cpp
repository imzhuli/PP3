#include "./p_audit_collect.hpp"

void xPP_AuditCollect::SerializeMembers() {
    W(GlobalAuthId);
    W(StartTimestampMS);
    W(PeriodMS);
    W(TotalTcpConnections);
    W(TotalTcpBytesFromClient);
    W(TotalTcpBytesToClient);
    W(TotalUdpChannels);
    W(TotalUdpBytesFromClient);
    W(TotalUdpBytesToClient);
}

void xPP_AuditCollect::DeserializeMembers() {
    R(GlobalAuthId);
    R(StartTimestampMS);
    R(PeriodMS);
    R(TotalTcpConnections);
    R(TotalTcpBytesFromClient);
    R(TotalTcpBytesToClient);
    W(TotalUdpChannels);
    R(TotalUdpBytesFromClient);
    R(TotalUdpBytesToClient);
}
