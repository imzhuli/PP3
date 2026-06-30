#pragma once
#include "./_.hpp"

enum struct eBlockAccountReason : uint8_t {
    BANDWITH_LIMIT   = 0,
    CONNECTION_LIMIT = 1,
    BLACK_LIST_LIMIT = 2,
};
