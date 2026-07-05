#pragma once
#include "../../abstract/device_locator_abstract.hpp"
#include "../../abstract/device_manager_abstract.hpp"
#include "./local_device.hpp"

#include <absl/container/flat_hash_map.h>

#include <xel/core/view.hpp>

class xLocalDeviceManager
/* : public xDeviceManagerAbstract */ {
public:
    bool Init(const std::vector<xLocalDeviceBinding> & LocalDeviceBindingList);
    void Clean();

public:
    /**
     * @brief
     *
     * @param Address
     * @return uint64_t DeviceId (aka device index)
     */
    uint64_t FindDeviceByExportAddress(const xNetAddress & Address);

private:
    struct xLocalDevice {
        uint64_t            Index;  // as DeviceId
        xLocalDeviceBinding BindingInfo;
    };

    std::vector<xLocalDevice>                            LocalDeviceList;
    absl::flat_hash_map<std::array<ubyte, 4>, uint64_t>  Ipv4ToDeviceIndex;
    absl::flat_hash_map<std::array<ubyte, 16>, uint64_t> Ipv6ToDeviceIndex;
};
