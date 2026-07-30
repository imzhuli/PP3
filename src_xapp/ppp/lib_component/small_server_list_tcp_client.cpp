#include "./small_server_list_tcp_client.hpp"

#include <pp_common/service_runtime.hpp>

xSmallServerListTcpClient::xSmallServerListTcpClient() {
    if (!ClientPool.Init(ServiceIoContext, MAX_SMALL_SERVER_LIST_SIZE * 5)) {
        return;
    }
    SetRaiiReady();
}

xSmallServerListTcpClient::~xSmallServerListTcpClient() {
    if (!IsRaiiReady()) {
        return;
    }
    ClientPool.Clean();
}

void xSmallServerListTcpClient::Tick(uint64_t NowMS) {
    ClientPool.Tick(NowMS);
}

void xSmallServerListTcpClient::UpdateServerList(const xServerInfo * ServerInfoList, size_t ServerInfoListSize) {
    auto NewServerArray = std::array<xServerInfo, MAX_SMALL_SERVER_LIST_SIZE>();
    Reset(PackedServerList);
    for (auto I = size_t(0); I < ServerInfoListSize; ++I) {
        auto & SI = ServerInfoList[I];
        assert(SI.ServerId);
        auto Index                      = ExtractIndexFromSmallServerId(SI.ServerId);
        NewServerArray[Index]           = SI;
        PackedServerList[I].ServerIndex = Index;
    }
    Reset(PackedServerListSize, ServerInfoListSize);

    for (auto I = size_t(0); I < MAX_SMALL_SERVER_LIST_SIZE; ++I) {
        auto & Old = ServerInfoContainer[I];
        auto & New = NewServerArray[I];

        if (Old.ConnectionId) {
            if (X_LIKELY(New == Old.ServerInfo)) {
                DEBUG_LOG("KeepServer: ServerId=%" PRIx64 ", Address=%s", New.ServerId, New.Address.ToString().c_str());
                continue;
            }
            DEBUG_LOG("RemoveServer: ServerId=%" PRIx64 ", Address=%s, ConnectionId=%" PRIx64 "", Old.ServerInfo.ServerId, Old.ServerInfo.Address.ToString().c_str(), Old.ConnectionId);
            ClientPool.RemoveServer(Steal(Old.ConnectionId));
            if (New.ServerId) {
                SERVICE_RUNTIME_ASSERT(Old.ConnectionId = ClientPool.AddServer(New.Address));
                Old.ServerInfo = New;
                DEBUG_LOG("AddServer: ServerId=%" PRIx64 ", Address=%s", New.ServerId, New.Address.ToString().c_str());
            }
        } else {
            if (New.ServerId) {
                SERVICE_RUNTIME_ASSERT(Old.ConnectionId = ClientPool.AddServer(New.Address));
                Old.ServerInfo = New;
                DEBUG_LOG("AddServer: ServerId=%" PRIx64 ", Address=%s", New.ServerId, New.Address.ToString().c_str());
            }
        }
    }
}

void xSmallServerListTcpClient::PostMessage(uint32_t Hash, xPacketCommandId CmdId, xPacketRequestId ReqId, xBinaryMessage & Message) {
    if (!PackedServerListSize) {
        return;
    }
    auto & Selected        = PackedServerList[Hash % PackedServerListSize];
    auto & ServerLocalInfo = ServerInfoContainer[Selected.ServerIndex];
    assert(ServerLocalInfo.ConnectionId);
    ClientPool.PostMessage(ServerLocalInfo.ConnectionId, CmdId, ReqId, Message);
}
