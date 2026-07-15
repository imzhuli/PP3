#include "../ppp/lib_component/address_challenge/address_challenge.hpp"

#include <pp_common/service_runtime.hpp>

int main(int argc, char ** argv) {
    X_VAR xServiceEnvironmentGuard(argc, argv, ServiceConsoleLogger);
    auto  ConfigFilename = ServiceEnvironment.DefaultConfigFilePath;

    auto Lines     = xel::FileToLines(ConfigFilename);
    auto Addresses = std::vector<xNetAddress>();
    for (auto & L : Lines) {
        L = Trim(L);
        if (L.empty()) {
            continue;
        }
        auto A = xel::xNetAddress::Parse(Trim(L));
        if (!A || !A.Port) {
            Logger->F("InvalidAddressLine: %s", L.c_str());
        }
        Addresses.push_back(A);
    }
    auto Service = xAddressChallengeService(Steal(Addresses));

    while (ServiceRunState) {
        ServiceUpdateOnce();
    }

    return 0;
}
