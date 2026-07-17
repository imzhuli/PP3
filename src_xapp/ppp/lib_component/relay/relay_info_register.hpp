#pragma once
#include "../../abstract/relay_abstract.hpp"
#include "../small_server_list_downloader.hpp"

struct xRelayInfo {
    eRelayServerType RelayServerType;
    xNetAddress      ExportDeviceSideAddress;
    xNetAddress      ExportProxySideAddrfess;
};

class xRelayInfoRegister final : xRaii {
public:
    xRelayInfoRegister(const xNetAddress & MasterDispatcherAddress, const xRelayInfo & RelayInfo);
    ~xRelayInfoRegister();

public:
    void Tick(uint64_t NowMS);

private:
    xTcpClient                 DispatcherClient;
    xSmallServerListDownloader ServerListDownloader;
};
