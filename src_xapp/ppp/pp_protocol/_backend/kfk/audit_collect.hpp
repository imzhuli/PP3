#pragma once
#include <pp_common/_.hpp>

/*
1	timeMs	int64	统计时刻
2	auditId	int64	鉴权 ID
3	tcpConnections	int64	TCP 有效连接数
4	tcpUploadSize	int64	TCP 上行（字节）
5	tcpDownloadSize	int64	TCP 下行（字节）
6	udpConnections	int64	UDP 连接数
7	udpUploadSize	int64	UDP 上行（字节）
8	udpDownloadSize	int64	UDP 下行（字节）
*/

class xPPB_AuditUsage final : public xBinaryMessage {
public:
    void SerializeMembers() override;
    void DeserializeMembers() override;

public:
    uint64_t TimeMs;           // 统计时刻
    uint64_t AuditId;          // 鉴权 ID
    uint64_t TcpConnections;   // TCP 有效连接数
    uint64_t TcpUploadSize;    // TCP 上行（字节）
    uint64_t TcpDownloadSize;  // TCP 下行（字节）
    uint64_t UdpConnections;   // UDP 连接数
    uint64_t UdpUploadSize;    // UDP 上行（字节）
    uint64_t UdpDownloadSize;  // UDP 下行（字节）
};
