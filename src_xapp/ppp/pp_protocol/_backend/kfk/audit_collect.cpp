#include "./audit_collect.hpp"

void xPPB_AuditUsage::SerializeMembers() {
    W(TimeMs);
    W(AuditId);
    W(TcpConnections);
    W(TcpUploadSize);
    W(TcpDownloadSize);
    W(UdpConnections);
    W(UdpUploadSize);
    W(UdpDownloadSize);
}

void xPPB_AuditUsage::DeserializeMembers() {
    R(TimeMs);
    R(AuditId);
    R(TcpConnections);
    R(TcpUploadSize);
    R(TcpDownloadSize);
    R(UdpConnections);
    R(UdpUploadSize);
    R(UdpDownloadSize);
}
