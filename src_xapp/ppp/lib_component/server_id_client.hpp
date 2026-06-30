#pragma once
#include <pp_common/_.hpp>
#include <xel/service/client/server_id_client.hpp>

using xel::service::xServerIdClientOptions;

class xServerIdClient {
public:
    bool Init(xIoContext * ICP, const xServerIdClientOptions & Options, const xNetAddress & ServerIdCenterAddress, const std::string & DumpFilename);
    void Clean();

    void     Tick(uint64_t NowMS) { InternalService.Tick(NowMS); }
    uint64_t GetLocalServerId() const { return InternalService.GetLocalServerId(); }

    std::function<void(uint64_t)> OnServerIdUpdated = Noop<>;

private:
    void InternalOnServerIdUpdated(uint64_t NewServerId);

private:
    xel::service::xServerIdClient InternalService;
    std::string                   LocalServerIdFilename;
};
