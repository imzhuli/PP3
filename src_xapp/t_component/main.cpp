#include <lib_component/local_device/local_device_manager.hpp>

int main(int, char **) {

    auto M = xLocalDeviceManager();

    auto AL = std::vector<xLocalDeviceBinding>{
        { xNetAddress::Parse("127.0.0.1"), xNetAddress::Parse("7.7.7.7") },
        { xNetAddress::Parse("127.0.0.1"), xNetAddress::Parse("8.8.8.8") },
    };

    X_RESOURCE_GUARD_ASSERTED(M, AL);

    cout << M.FindDeviceByExportAddress(xNetAddress::Parse("8.8.8.8")) << endl;

    return 0;
}
