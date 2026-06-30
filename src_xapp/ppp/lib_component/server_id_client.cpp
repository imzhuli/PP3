#include "./server_id_client.hpp"

#include <fstream>

static uint64_t LoadLocalServerId(const std::string & LocalServerIdFilename) {
    auto File  = LocalServerIdFilename;
    auto FSOpt = FileToStr(File);
    if (!FSOpt) {
        return 0;
    }
    return (uint64_t)strtoumax(FSOpt->c_str(), nullptr, 10);
}

static void DumpLocalServerId(const std::string & LocalServerIdFilename, uint64_t LocalServerId) {
    if (LocalServerIdFilename.empty()) {
        return;
    }
    auto File  = LocalServerIdFilename;
    auto FSOpt = std::ofstream(File, std::ios_base::binary | std::ios_base::out);
    if (!FSOpt) {
        cerr << "failed to dump file to LocalCacheFile" << endl;
        return;
    }
    FSOpt << LocalServerId << endl;
    return;
}

bool xServerIdClient::Init(xIoContext * ICP, const xServerIdClientOptions & Options, const xNetAddress & ServerIdCenterAddress, const std::string & DumpFilename) {
    auto ProcessedOptions = Options;
    auto InitServerId     = uint64_t(0);
    LocalServerIdFilename = DumpFilename;
    if (!LocalServerIdFilename.empty()) {
        InitServerId = LoadLocalServerId(LocalServerIdFilename);
    }
    if (InitServerId) {
        ProcessedOptions.PreviousServerId = InitServerId;
    }

    if (!InternalService.Init(ICP, ProcessedOptions, ServerIdCenterAddress)) {
        return false;
    }
    InternalService.OnServerIdUpdated = Delegate(&xServerIdClient::InternalOnServerIdUpdated, this);
    return true;
}

void xServerIdClient::Clean() {
    InternalService.Clean();
    Reset(LocalServerIdFilename);
}

void xServerIdClient::InternalOnServerIdUpdated(uint64_t NewServerId) {
    if (!LocalServerIdFilename.empty()) {
        DumpLocalServerId(LocalServerIdFilename, NewServerId);
    }
    OnServerIdUpdated(NewServerId);
}
