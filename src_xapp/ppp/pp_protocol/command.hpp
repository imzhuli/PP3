#pragma once
#include <pp_common/_.hpp>

// xPacketCommandId
static constexpr const xPacketCommandId Cmd_PPP_Base  = 0x01'000;
static constexpr const xPacketCommandId Cmd_Resp_Flag = 0x00'800;

#define PPP_CMD(RequestName, Cmd) static constexpr const xPacketCommandId Cmd_##RequestName = (Cmd_PPP_Base + Cmd)
PPP_CMD(EchoTest, 0x01);
PPP_CMD(EchoTestResp, 0x02);

// backend base:
static constexpr const xPacketCommandId Cmd_BackendBase                  = 0x04'000;
static constexpr const xPacketCommandId Cmd_BackendAuditTerminalInfo     = Cmd_BackendBase + 0x01;   // 旧版, 弃用
static constexpr const xPacketCommandId Cmd_BackendAuditTerminalInfoResp = Cmd_BackendBase + 0x02;   // 一般不处理返回
static constexpr const xPacketCommandId Cmd_BackendTargetReport          = Cmd_BackendBase + 0x301;  //
static constexpr const xPacketCommandId Cmd_BackendBlockAccountReport    = Cmd_BackendBase + 0x302;  //
static constexpr const xPacketCommandId Cmd_BackendAuditReport           = Cmd_BackendBase + 0x303;  //
//
static constexpr const xPacketCommandId Cmd_AuditUsageByAuthId           = Cmd_BackendBase + 0x03;  //
static constexpr const xPacketCommandId Cmd_AuditUsageByAuthIdResp       = Cmd_BackendBase + 0x04;  // 一般不处理返回
static constexpr const xPacketCommandId Cmd_AuditTerminalInfo2           = Cmd_BackendBase + 0x05;  // 2025-03-09 新增, 无返回
static constexpr const xPacketCommandId Cmd_DeviceRelayServerInfo2       = Cmd_BackendBase + 0x06;  // 2025-03-09 新增, 无返回
static constexpr const xPacketCommandId Cmd_DeviceRelayServerInfoSingle  = Cmd_BackendBase + 0x07;  // 2025-05-28 新增, 无返回
static constexpr const xPacketCommandId Cmd_AuditThirdAccountUsage       = Cmd_BackendBase + 0x08;  // 2025-05-28 新增, 无返回
//
static constexpr const xPacketCommandId Cmd_BackendChallenge             = Cmd_BackendBase + 0x101;
static constexpr const xPacketCommandId Cmd_BackendChallengeResp         = Cmd_BackendBase + 0x102;
static constexpr const xPacketCommandId Cmd_BackendAuthByUserPass        = Cmd_BackendBase + 0x201;
static constexpr const xPacketCommandId Cmd_BackendAuthByUserPassResp    = Cmd_BackendBase + 0x202;

// server id & server list:
static constexpr const xPacketCommandId Cmd_ServerManagementBase             = 0x01'000;
static constexpr const xPacketCommandId Cmd_DownloadSmallServerList          = Cmd_ServerManagementBase + 0x01;
static constexpr const xPacketCommandId Cmd_DownloadSmallServerListResp      = Cmd_ServerManagementBase + 0x02;
// proxy side report
static constexpr const xPacketCommandId Cmd_AuditBase                        = 0x02'000;
static constexpr const xPacketCommandId Cmd_TargetReport                     = Cmd_AuditBase + 0x01;
static constexpr const xPacketCommandId Cmd_BlockAccountReport               = Cmd_AuditBase + 0x02;
static constexpr const xPacketCommandId Cmd_AuditReport                      = Cmd_AuditBase + 0x03;
//
static constexpr const xPacketCommandId Cmd_RelayBase                        = 0x03'000;
static constexpr const xPacketCommandId Cmd_RelayInfoRegister                = Cmd_RelayBase + 0x01;
static constexpr const xPacketCommandId Cmd_RelayInfoRegisterResp            = Cmd_RelayBase + 0x02;
static constexpr const xPacketCommandId Cmd_RelayDispatcherSlaveRegister     = Cmd_RelayBase + 0x03;
static constexpr const xPacketCommandId Cmd_RelayDispatcherSlaveRegisterResp = Cmd_RelayBase + 0x04;

///////////////////////// PP2 server list

static constexpr const xPacketCommandId Cmd_AuditAccountUsage = Cmd_ServerManagementBase + 0x501;
static constexpr const xPacketCommandId Cmd_AuditTarget       = Cmd_ServerManagementBase + 0x502;

// pa-relay:
static constexpr const xPacketCommandId Cmd_PA_RL_Base                  = 0x06'000;
static constexpr const xPacketCommandId Cmd_PA_RL_Challenge             = Cmd_PA_RL_Base + 0x01;
static constexpr const xPacketCommandId Cmd_PA_RL_ChallengeResp         = Cmd_PA_RL_Base + 0x02;
static constexpr const xPacketCommandId Cmd_PA_RL_CreateConnection      = Cmd_PA_RL_Base + 0x03;
static constexpr const xPacketCommandId Cmd_PA_RL_DestroyConnection     = Cmd_PA_RL_Base + 0x04;
static constexpr const xPacketCommandId Cmd_PA_RL_NotifyConnectionState = Cmd_PA_RL_Base + 0x05;
static constexpr const xPacketCommandId Cmd_PA_RL_PostData              = Cmd_PA_RL_Base + 0x06;
static constexpr const xPacketCommandId Cmd_PA_RL_ProxyClientNotify     = Cmd_PA_RL_Base + 0x09;  //
static constexpr const xPacketCommandId Cmd_PA_RL_CreateUdpBinding      = Cmd_PA_RL_Base + 0x0A;  //
static constexpr const xPacketCommandId Cmd_PA_RL_DestroyUdpBinding     = Cmd_PA_RL_Base + 0x0B;  //
static constexpr const xPacketCommandId Cmd_PA_RL_KeepAliveUdpBinding   = Cmd_PA_RL_Base + 0x0C;  //
static constexpr const xPacketCommandId Cmd_PA_RL_PostUdpData           = Cmd_PA_RL_Base + 0x0D;  //

// cc-device:
static constexpr const xPacketCommandId Cmd_DV_CC_Base              = 0x07'000;
static constexpr const xPacketCommandId Cmd_DV_CC_GetAddressKey     = Cmd_DV_CC_Base + 0x01;
static constexpr const xPacketCommandId Cmd_DV_CC_GetAddressKeyResp = Cmd_DV_CC_Base + 0x02;
static constexpr const xPacketCommandId Cmd_DV_CC_Challenge         = Cmd_DV_CC_Base + 0x03;
static constexpr const xPacketCommandId Cmd_DV_CC_ChallengeResp     = Cmd_DV_CC_Base + 0x04;

// device-relay:
static constexpr const xPacketCommandId Cmd_DV_RL_Base                 = 0x08'000;
static constexpr const xPacketCommandId Cmd_DV_RL_Handshake            = Cmd_DV_RL_Base + 0x01;
static constexpr const xPacketCommandId Cmd_DV_RL_HandshakeResp        = Cmd_DV_RL_Base + 0x02;
static constexpr const xPacketCommandId Cmd_DV_RL_ConfigDevice         = Cmd_DV_RL_Base + 0x03;
static constexpr const xPacketCommandId Cmd_DV_RL_InitDataStreamResp   = Cmd_DV_RL_Base + 0x04;  // deprecated
static constexpr const xPacketCommandId Cmd_DV_RL_CreateConnection     = Cmd_DV_RL_Base + 0x05;
static constexpr const xPacketCommandId Cmd_DV_RL_CreateConnectionHost = Cmd_DV_RL_Base + 0x06;
static constexpr const xPacketCommandId Cmd_DV_RL_CreateConnectionResp = Cmd_DV_RL_Base + 0x07;
static constexpr const xPacketCommandId Cmd_DV_RL_PostConnectionData   = Cmd_DV_RL_Base + 0x08;
static constexpr const xPacketCommandId Cmd_DV_RL_DestroyConnection    = Cmd_DV_RL_Base + 0x09;
static constexpr const xPacketCommandId Cmd_DV_RL_KeepAliveConnection  = Cmd_DV_RL_Base + 0x0A;
//
static constexpr const xPacketCommandId Cmd_DV_RL_CreateUdpChannel     = Cmd_DV_RL_Base + 0x10;
static constexpr const xPacketCommandId Cmd_DV_RL_DestroyUdpChannel    = Cmd_DV_RL_Base + 0x11;
static constexpr const xPacketCommandId Cmd_DV_RL_PostUdpChannelData   = Cmd_DV_RL_Base + 0x12;
static constexpr const xPacketCommandId Cmd_DV_RL_CreateUdpChannelResp = Cmd_DV_RL_Base + 0x13;
static constexpr const xPacketCommandId Cmd_DV_RL_KeepAliveUdpChannel  = Cmd_DV_RL_Base + 0x14;
//
static constexpr const xPacketCommandId Cmd_DV_RL_DnsQuery             = Cmd_DV_RL_Base + 0x20;
static constexpr const xPacketCommandId Cmd_DV_RL_DnsQueryResp         = Cmd_DV_RL_Base + 0x21;
//
static constexpr const xPacketCommandId Cmd_DV_RL_AddressChallenge     = Cmd_DV_RL_Base + 0x30;
static constexpr const xPacketCommandId Cmd_DV_RL_AddressChallengeResp = Cmd_DV_RL_Base + 0x31;

// device-state-relay(dsr) / device-selector
static constexpr const xPacketCommandId Cmd_DSR_DS_Base         = 0x09'000;
static constexpr const xPacketCommandId Cmd_DSR_DS_DeviceUpdate = Cmd_DSR_DS_Base + 0x01;

// device-selector
static constexpr const xPacketCommandId Cmd_DeviceSelector_Base = 0x0D'000;

// auth-service
static constexpr const xPacketCommandId Cmd_AuthService_Base = 0x0E'000;

std::vector<ubyte> Encrypt(const void * Data, size_t DataSize, const std::string & AesKey);
std::vector<ubyte> Decrypt(const void * Data, size_t DataSize, const std::string & AesKey);
