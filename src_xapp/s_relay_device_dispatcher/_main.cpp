#include <lib_component/server_id_client.hpp>
#include <pp_common/service_runtime.hpp>

static xNetAddress xSmallServerListAddress;

static auto IdClient          = xServerIdClient();
static auto ProducerService   = xTcpService();
static auto DispatcherService = xTcpService();

int main(int argc, char ** argv) {
    X_VAR xServiceEnvironmentGuard(argc, argv);
    auto  CL = ServiceEnvironment.LoadConfig();

    return 0;
}
