#pragma once
#include "./_.hpp"

enum struct eRelayServerType : xServerGroup {
    UNSPECIFIED = 0,
    DEVICE      = 1,
    STATIC      = 2,
    THIRD       = 3,
    RELAY_TYPE_COUNT,
};
