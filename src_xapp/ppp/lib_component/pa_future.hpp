#pragma once
#include "../abstract/auth_abstract.hpp"
#include "../abstract/device_abstract.hpp"
#include "../abstract/pa_abstract.hpp"
#include "../abstract/relay_abstract.hpp"

#include <expected>
#include <pp_common/future.hpp>

struct xPA_AuthFuture final : xAuthResultFuture {
    struct xPA_ClientConnection * ClientConnection = nullptr;
};

struct xPA_AcquireDeviceFuture final : xAcquireDeviceFuture {
    struct xPA_ClientConnection * ClientConnection = nullptr;
};

struct xPA_AcquireDeviceConnectionFuture final : xRelayCreateConnectionFuture {
    struct xPA_ClientConnection * ClientConnection = nullptr;
};

struct xPA_AcquireDeviceUdpChannelFuture final : xRelayCreateUdpChannelFuture {
    struct xPA_ClientConnection * ClientConnection = nullptr;
};
