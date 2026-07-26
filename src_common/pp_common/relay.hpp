#pragma once
#include "./_.hpp"

static constexpr const size_t MAX_RELAY_SERVER_LIST_SIZE = 20'0000;

enum struct eRelayServerType : xServerGroup {
    UNSPECIFIED = 0,
    DEVICE      = 1,
    STATIC      = 2,
    THIRD       = 3,
    RELAY_TYPE_COUNT,
};
