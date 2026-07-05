#include "./local_device_manager.hpp"

bool xLocalDeviceManager::Init(const std::vector<xLocalDeviceBinding> & LocalDeviceBindingList) {

    assert(LocalDeviceList.empty());
    assert(Ipv4ToDeviceIndex.empty());
    assert(Ipv6ToDeviceIndex.empty());

    size64_t IndexCounter = 0;
    for (auto & B : LocalDeviceBindingList) {
        auto D = xLocalDevice{
            ++IndexCounter,
            B,
        };  // add 1 first, 0 is reserved for "no device found"
        X_RUNTIME_ASSERT(B.BindAddress.Type == B.ExportAddress.Type);
        X_RUNTIME_ASSERT(B.BindAddress && !B.BindAddress.Port && B.ExportAddress && !B.ExportAddress.Port);
        if (B.ExportAddress.Is4()) {
            auto Key = std::to_array(B.ExportAddress.SA4);
            X_RUNTIME_ASSERT(Ipv4ToDeviceIndex.find(Key) == Ipv4ToDeviceIndex.end());
            Ipv4ToDeviceIndex[Key] = IndexCounter;
        } else if (B.ExportAddress.Is6()) {
            auto Key = std::to_array(B.ExportAddress.SA6);
            X_RUNTIME_ASSERT(Ipv6ToDeviceIndex.find(Key) == Ipv6ToDeviceIndex.end());
            Ipv6ToDeviceIndex[Key] = IndexCounter;
        } else {
            xel::FatalPrintf("invalid binding address");
        }
        LocalDeviceList.push_back(D);
    }

    return true;
}

void xLocalDeviceManager::Clean() {
    Reset(LocalDeviceList);
    Reset(Ipv4ToDeviceIndex);
    Reset(Ipv6ToDeviceIndex);
}

uint64_t xLocalDeviceManager::FindDeviceByExportAddress(const xNetAddress & Address) {
    assert(!Address.Port);
    if (Address.Is4()) {
        auto Iter = Ipv4ToDeviceIndex.find(std::to_array(Address.SA4));
        if (Iter == Ipv4ToDeviceIndex.end()) {
            return 0;
        }
        return Iter->second;
    } else if (Address.Is6()) {
        auto Iter = Ipv6ToDeviceIndex.find(std::to_array(Address.SA6));
        if (Iter == Ipv6ToDeviceIndex.end()) {
            return 0;
        }
        return Iter->second;
    }
    return 0;
}
