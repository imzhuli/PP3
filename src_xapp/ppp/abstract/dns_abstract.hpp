#pragma once
#include <pp_common/_.hpp>
#include <pp_common/future.hpp>

struct xDnsResult {
    xNetAddress A4;
    xNetAddress A6;
};

struct xDnsReultFuture : xFutureBase {
    xExpected<xDnsResult> Result = UnexpctedResult;
};

extern xDnsReultFuture * GetDnsResultFuture(const xFutureHandle & Handle);

struct xDnsServiceAbstract : xAbstract {
    virtual bool ResolveDns(const std::string_view & HostnameView, xDnsReultFuture & Future) = 0;
};
