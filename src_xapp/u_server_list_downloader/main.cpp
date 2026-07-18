#include <algorithm>
#include <lib_component/small_server_list_downloader.hpp>
#include <map>
#include <pp_common/service_runtime.hpp>

static auto EnabledServerGroup = std::map<xServerGroup, bool>{
    { ST_TARGET_COLLECTOR, false },
    { ST_AUDIT_COLLECTOR, false },
    { ST_RELAY_REGISTER, false },
    { ST_RELAY_DISPATCHER_MASTER, false },
    { ST_RELAY_DISPATCHER_SLAVE, false },
};

static bool IsAllDone() {
    for (auto & G : EnabledServerGroup) {
        if (!G.second) {
            return false;
        }
    }
    return true;
}

static void OnServerListUpdated(xServerGroup ServerGroup, const xServerInfo * ServerList, size_t ServerListSize, uint64_t VersionTimestampMS) {
    const char * GroupName = nullptr;
    switch (ServerGroup) {
        case ST_TARGET_COLLECTOR:
            GroupName = "ST_TARGET_COLLECTOR";
            break;
        case ST_AUDIT_COLLECTOR:
            GroupName = "ST_AUDIT_COLLECTOR";
            break;
        case ST_RELAY_REGISTER:
            GroupName = "ST_RELAY_REGISTER";
            break;
        case ST_RELAY_DISPATCHER_MASTER:
            GroupName = "ST_RELAY_DISPATCHER_MASTER";
            break;
        case ST_RELAY_DISPATCHER_SLAVE:
            GroupName = "ST_RELAY_DISPATCHER_SLAVE";
            break;

        default:
            break;
    }
    if (!GroupName) {
        return;
    }

    EnabledServerGroup[ServerGroup] = true;

    auto OS = std::ostringstream();
    OS << "Update Group: " << GroupName << ", Version=" << VersionTimestampMS << endl;
    auto fmt = OS.flags();
    OS << std::hex;
    for (size_t I = 0; I < ServerListSize; ++I) {
        auto S = ServerList[I];
        OS << "\tId=" << S.ServerId << ", Address=" << S.Address.ToString() << endl;
    }
    OS.flags(fmt);

    cout << OS.str() << endl;
}

int main(int argc, char ** argv) {
    X_VAR xServiceEnvironmentGuard(argc, argv, ServiceConsoleLogger);

    auto CL = xel::xCommandLine{
        argc,
        argv,
        {
            { 's', "server_list_server", "server_list_server", true },
        }
    };

    auto SLS = CL["server_list_server"];
    X_RUNTIME_ASSERT(SLS);

    auto Downloader = xSmallServerListDownloader();
    X_RUNTIME_ASSERT(Downloader.Init(xNetAddress::Parse(*SLS)));
    for (auto & G : EnabledServerGroup) {
        Downloader.EnableServerGroup(G.first);
    }
    Downloader.OnServerListUpdated = OnServerListUpdated;

    while (ServiceRunState) {
        ServiceUpdateOnce(Downloader);
        if (IsAllDone()) {
            ServiceRunState.Stop();
        }
    }

    return 0;
}
