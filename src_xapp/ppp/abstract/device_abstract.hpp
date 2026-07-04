#pragma once
#include <pp_common/_region.hpp>
#include <pp_common/device.hpp>
#include <pp_common/future.hpp>

/************************** */

struct xDeviceReference {
    uint64_t RelayServerId;
    uint64_t RelaySideDeviceId;
};

struct xAcquireDeviceFuture : xFutureBase {
    xExpected<xDeviceReference> Result = UnexpctedResult;
};

struct xDeviceRequest {
    xDeviceFlag Flag;
    uint64_t    LastDeviceId;
    union {
        xNetAddress ExportAddress;
        xGeoInfo    GeoInfo;
    } Condition;
    xDeviceSelectionStrategy Strategy;
};

struct xDeviceLocatorServiceAbstract : xAbstract {
    virtual void AcquireDevice(const xDeviceRequest & Request, xAcquireDeviceFuture & Future) = 0;
};
