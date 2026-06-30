#include "./dns_abstract.hpp"

xDnsReultFuture * GetDnsResultFuture(const xFutureHandle & Handle) {
    return Handle.Get<xDnsReultFuture>();
}