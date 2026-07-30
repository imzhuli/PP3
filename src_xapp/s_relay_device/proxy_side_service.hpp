#pragma once
#include <abstract/relay_abstract.hpp>

class xRelayProxySideService final
    : public xRaii {
public:
    static constexpr const size_t MAX_PROXY_SIDE_CONNECTION_COUNT = 10'0000;

public:
    xRelayProxySideService(const xNetAddress & BindAddress);
    ~xRelayProxySideService();
    void Tick(uint64_t NowMS);

private:
    xTcpService TcpService;
};