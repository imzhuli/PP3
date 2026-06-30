#pragma once
#include <pp_common/_.hpp>

struct xPP_GetSmallServerList : public xBinaryMessage {  // from proxy_access to relay server
public:
    void SerializeMembers() override {
        W(ServerGroup);  //
    }

    void DeserializeMembers() override {
        R(ServerGroup);  //
    }

    xServerGroup ServerGroup;
};

struct xPP_GetSmallServerListResp : public xBinaryMessage {  // from proxy_access to relay server
public:
    void SerializeMembers() override;
    void DeserializeMembers() override;

    using xSmallServerList = std::array<xServerInfo, MAX_SMALL_SERVER_LIST_SIZE>;
    uint64_t           VersionTimestampMS;
    uint32_t           ServerListSize;
    xSmallServerList * ServerList;
};