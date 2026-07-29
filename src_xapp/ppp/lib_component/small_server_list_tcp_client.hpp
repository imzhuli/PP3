#pragma once
#include <pp_common/_.hpp>

class xSmallServerListTcpClient final
    : public xRaii
    , xAbstract {
public:
    xSmallServerListTcpClient();
    ~xSmallServerListTcpClient();
    void Tick(uint64_t NowMS);
    void UpdateServerList(const xServerInfo * ServerInfoList, size_t ServerInfoListSize);

    void PostMessage(uint32_t Hash, xPacketCommandId CmdId, xPacketRequestId ReqId, xBinaryMessage & Message);
    void PostMessage(xPacketCommandId CmdId, xPacketRequestId ReqId, xBinaryMessage & Message) {
        PostMessage(NextPostDataServerIndex++, CmdId, ReqId, Message);
    }

    xTcpClientPool::xOnServerConnected & OnServerConnected = ClientPool.OnServerConnected;
    xTcpClientPool::xOnServerClean &     OnServerClean     = ClientPool.OnServerClean;
    xTcpClientPool::xOnServerPacket &    OnServerPacket    = ClientPool.OnServerPacket;

private:
    struct xServerLocalInfo : xListNode {
        uint64_t    ConnectionId = 0;
        xServerInfo ServerInfo   = {};
    };
    using xServerInfoContainer = std::array<xServerLocalInfo, MAX_SMALL_SERVER_LIST_SIZE>;

    xServerInfoContainer ServerInfoContainer = {};
    xTcpClientPool       ClientPool          = {};

    struct xPackedServerIndex {
        uint32_t ServerIndex = 0;
    };
    using xPackedServerList = std::array<xPackedServerIndex, MAX_SMALL_SERVER_LIST_SIZE>;
    xPackedServerList PackedServerList;
    size_t            PackedServerListSize    = 0;
    size_t            NextPostDataServerIndex = 0;
};
