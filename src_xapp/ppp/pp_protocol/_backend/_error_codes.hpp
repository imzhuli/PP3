#pragma once
#include <pp_common/_.hpp>

enum struct ePPB_Error : uint32_t {

    OK                     = 0,
    INVALID_PARAMETER      = 201,
    PROXY_NOT_FOUND        = 202,
    INVLID_PROTOCOL        = 203,
    NOT_LOGIN              = 401,
    INVALID_AUTHENTICATION = 403,
    UNSUPPORTED_FUNCTION   = 404,
    DEVICE_DROPPED         = 405,
    INTERNAL_SERVER_ERROR  = 500,

};
