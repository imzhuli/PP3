#include "./audit_service.hpp"

#include "../../pp_protocol/command.hpp"
#include "../../pp_protocol/p_audit_collect.hpp"
#include "../../pp_protocol/p_block_account.hpp"
#include "../../pp_protocol/p_target_collect.hpp"

#include <pp_common/service_runtime.hpp>

static std::string_view ExtractSecondLevelDomain(std::string_view Domain) {
    // remove port number
    // auto colon_pos = Domain.find(':');
    // if (colon_pos != std::string_view::npos) {
    //     Domain = Domain.substr(0, colon_pos);
    // }
    size_t LastDot = std::string_view::npos;
    for (size_t I = Domain.size(); I > 0;) {
        if (Domain[--I] == '.') {
            LastDot = I;
            break;
        }
    }
    if (LastDot == std::string_view::npos) {
        return Domain;
    }

    size_t SecondLastDot = std::string_view::npos;
    for (size_t I = LastDot; I > 0;) {
        if (Domain[--I] == '.') {
            SecondLastDot = I;
            break;
        }
    }
    if (SecondLastDot == std::string_view::npos) {
        return Domain;
    }
    return Domain.substr(SecondLastDot + 1);
}

bool xAuditService::Init(const xNetAddress & ServerListServerAddress, const xNetAddress & LocalBindAddress) {
    if (!TcpReporter.Init(ServiceIoContext, MAX_SMALL_SERVER_LIST_SIZE)) {
        return false;
    }
    if (!UdpReporter.Init(ServiceIoContext, LocalBindAddress)) {
        TcpReporter.Clean();
        return false;
    }
    if (!ServerListDownloader.Init(ServerListServerAddress, LocalBindAddress.Decay())) {
        TcpReporter.Clean();
        UdpReporter.Clean();
        return false;
    }
    if (!AuditClientPool.Init(ServiceIoContext, 3 * MAX_SMALL_SERVER_LIST_SIZE)) {
        ServerListDownloader.Clean();
        TcpReporter.Clean();
        UdpReporter.Clean();
    }
    ServerListDownloader.EnableServerGroup(ST_TARGET_COLLECTOR);
    ServerListDownloader.EnableServerGroup(ST_AUDIT_COLLECTOR);
    ServerListDownloader.OnServerListUpdated = Delegate(&xAuditService::OnServerListUpdated, this);

    Reset(ConnectionIdList);
    return true;
}

void xAuditService::Clean() {
    Reset(ConnectionIdList);
    AuditClientPool.Clean();
    ServerListDownloader.Clean();
    UdpReporter.Clean();
    TcpReporter.Clean();
}

void xAuditService::Tick(uint64_t NowMS) {
    ServerListDownloader.Tick(NowMS);
    TcpReporter.Tick(NowMS);
}

std::string xAuditService::OutputAudit() {
    auto OS = std::ostringstream();
    OS << "xAuditService" << endl;
    OS << "\tNoServerReport=" << Steal(Audit.NoServerReport) << endl;
    return OS.str();
}

void xAuditService::OnServerListUpdated(xServerGroup ServerGroup, const xServerInfo * ServerList, size_t ServerListSize, uint64_t VersionTimestampMS) {
    if (ServerGroup == ST_TARGET_COLLECTOR) {  // udp servers , simply replace the old list
        DEBUG_LOG("renew target collector server list (always use ServerListDownloader.GetServerListView()), version_timestamp_ms=%" PRIu64 "", VersionTimestampMS);
        return;
    }

    if (ServerGroup == ST_AUDIT_COLLECTOR) {
        auto OldList              = AuditServerList.Container.data();
        auto OldSize              = AuditServerList.Size;
        auto OldIndex             = size_t(0);
        auto NewList              = ServerList;
        auto NewSize              = ServerListSize;
        auto NewIndex             = size_t(0);
        auto TempConnectionIdList = xAuditConnectionIdList();
        auto TempConnectionCurror = size_t(0);
        DEBUG_LOG("renew audit collector server list, version_timestamp_ms=%" PRIu64 "", VersionTimestampMS);
        while (true) {
            if (NewIndex == NewSize) {
                for (; OldIndex < OldSize; ++OldIndex) {
                    auto & RemovedServerInfo = OldList[OldIndex];
                    DEBUG_LOG("remove audit collector server info: %" PRIx64 ", Address=%s", RemovedServerInfo.ServerId, RemovedServerInfo.Address.ToString().c_str());
                    Pass(RemovedServerInfo);
                    TcpReporter.RemoveServer(ConnectionIdList[OldIndex]);
                }
                break;
            }
            if (OldIndex == OldSize) {
                for (; NewIndex < NewSize; ++NewIndex) {
                    auto & AddedServerInfo = NewList[NewIndex];
                    DEBUG_LOG("add audit collector server info: %" PRIx64 ", Address=%s", AddedServerInfo.ServerId, AddedServerInfo.Address.ToString().c_str());
                    RuntimeAssert(TempConnectionIdList[TempConnectionCurror++] = TcpReporter.AddServer(AddedServerInfo.Address));
                }
                break;
            }
            auto & FromOld = OldList[OldIndex];
            auto & FromNew = NewList[NewIndex];
            if (FromOld.ServerId == FromNew.ServerId) {
                if (FromOld.Address == FromNew.Address) {
                    DEBUG_LOG("remain audit collector server info: %" PRIx64 ", Address=%s", FromOld.ServerId, FromOld.Address.ToString().c_str());
                    TempConnectionIdList[TempConnectionCurror++] = ConnectionIdList[OldIndex];
                } else {
                    DEBUG_LOG("remove audit collector server info: %" PRIx64 ", Address=%s", FromOld.ServerId, FromOld.Address.ToString().c_str());
                    TcpReporter.RemoveServer(ConnectionIdList[OldIndex]);
                    DEBUG_LOG("add audit collector server info: %" PRIx64 ", Address=%s", FromNew.ServerId, FromNew.Address.ToString().c_str());
                    RuntimeAssert(TempConnectionIdList[TempConnectionCurror++] = TcpReporter.AddServer(FromNew.Address));
                }
                ++OldIndex;
                ++NewIndex;
            } else if (FromOld.ServerId < FromNew.ServerId) {  // remove old
                DEBUG_LOG("remove audit collector server info: %" PRIx64 ", Address=%s", FromOld.ServerId, FromOld.Address.ToString().c_str());
                TcpReporter.RemoveServer(ConnectionIdList[OldIndex]);
                ++OldIndex;
            } else {
                DEBUG_LOG("add audit collector server info: %" PRIx64 ", Address=%s", FromNew.ServerId, FromNew.Address.ToString().c_str());
                RuntimeAssert(TempConnectionIdList[TempConnectionCurror++] = TcpReporter.AddServer(FromNew.Address));
                ++NewIndex;
            }
        }
        assert(TempConnectionCurror == NewSize);
        for (size_t I = 0; I < NewSize; ++I) {
            AuditServerList.Container[I] = ServerList[I];
            ConnectionIdList[I]          = TempConnectionIdList[I];
        }
        AuditServerList.Size = NewSize;
    }
}

void xAuditService::ReportTarget(uint64_t GlobalAuthId, const xel::xNetAddress & TargetAddress, const std::string_view & TargetHost, size_t Count) {
    auto [ServerList, ServerListSize] = ServerListDownloader.GetServerListView(ST_TARGET_COLLECTOR);
    if (!ServerList || !ServerListSize) {
        ++Audit.NoServerReport;
        return;
    }

    auto ShortDomain   = ExtractSecondLevelDomain(TargetHost);
    auto Req           = xPP_TargetCollect();
    Req.GlobalAuthId   = GlobalAuthId;
    Req.TargetAddress  = TargetAddress;
    Req.TargetHostView = ShortDomain;
    Req.Count          = Count;

    ubyte Buffer[xel::MaxPacketSize];
    auto  RSize        = WriteMessage(Buffer, Cmd_TargetReport, 0, Req);
    auto  Hash         = Req.Hash;
    auto  SelectTarget = ServerList[Hash % ServerListSize];
    DEBUG_LOG("SelectReportTarget: %s", SelectTarget.Address.ToString().c_str());

    UdpReporter.PostData(SelectTarget.Address, Buffer, RSize);
}

void xAuditService::ReportUsage(const xAuditUsage & UsageInfo) {
    if (!AuditServerList.Size) {
        ++Audit.NoServerReport;
        return;
    }
    auto SelectedConnectionId   = ConnectionIdList[UsageInfo.AuthId % AuditServerList.Size];
    auto Req                    = xPP_AuditCollect();
    Req.GlobalAuthId            = UsageInfo.AuthId;
    Req.StartTimestampMS        = UsageInfo.StartTimestampMS;
    Req.PeriodMS                = UsageInfo.PeriodMS;
    Req.TotalTcpConnections     = UsageInfo.TotalTcpConnections;
    Req.TotalTcpBytesFromClient = UsageInfo.TotalTcpBytesFromClient;
    Req.TotalTcpBytesToClient   = UsageInfo.TotalTcpBytesToClient;
    Req.TotalUdpChannels        = UsageInfo.TotalUdpChannels;
    Req.TotalUdpBytesFromClient = UsageInfo.TotalUdpBytesFromClient;
    Req.TotalUdpBytesToClient   = UsageInfo.TotalUdpBytesToClient;
    DEBUG_LOG("UsageInfo:%s", UsageInfo.ToString().c_str());

    TcpReporter.PostMessage(SelectedConnectionId, Cmd_AuditReport, 0, Req);
}

void xAuditService::ReportBlockAccount(const xAuditBlockAccount & BlockAccountInfo) {
    if (!AuditServerList.Size) {
        ++Audit.NoServerReport;
        return;
    }
    auto SelectedConnectionId = ConnectionIdList[BlockAccountInfo.AuthId % AuditServerList.Size];
    auto Req                  = xPP_BlockAccount();
    Req.GlobalAuthId          = BlockAccountInfo.AuthId;
    Req.StartTimestampMS      = BlockAccountInfo.StartTimestampMS;
    Req.BlockPeriodMS         = BlockAccountInfo.PeriodMS;
    Req.BlockReason           = BlockAccountInfo.Reason;
    Req.BlockThreshold        = BlockAccountInfo.Threshold;
    Req.BlockTriggerValue     = BlockAccountInfo.TriggerValue;

    DEBUG_LOG("BlockAccount:%s", BlockAccountInfo.ToString().c_str());

    TcpReporter.PostMessage(SelectedConnectionId, Cmd_BlockAccountReport, 0, Req);
}
