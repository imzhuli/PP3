#pragma once
#include <pp_common/_.hpp>
#include <pp_common/_region.hpp>
#include <pp_common/future.hpp>

struct xRegionFuture final : xFutureBase {
    xRegionFuture();
    xExpected<xCountryId> CountryId = UnexpctedResult;
};

struct xRegionServiceAbstract : xAbstract {
    virtual void GetRegion(const xel::xNetAddress & NA, xRegionFuture & Future) = 0;
};
