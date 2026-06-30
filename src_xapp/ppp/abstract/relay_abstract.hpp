#pragma once

#include <pp_common/_common.hpp>
#include <pp_common/device.hpp>
#include <pp_common/future.hpp>

struct xRelayAbstractConnection;
struct xRelayAbstractUdpChannel;

struct xRelayCreateConnectionFuture : xFutureBase {
    xExpected<uint64_t> Result = UnexpctedResult;
};

struct xRelayCreateUdpChannelFuture : xFutureBase {
    xExpected<uint64_t> Result = UnexpctedResult;
};

struct xRelayAbstractService
    : xAbstract
    , xNonCopyable {
    virtual void CreateConnection(uint64_t RelayServerId, uint64_t DeviceId, uint64_t PASideConnectionId, const std::string_view & TargetHostnameView, uint16_t TargetPort, xRelayCreateConnectionFuture & Future) = 0;
    virtual void CreateConnection(uint64_t RelayServerId, uint64_t DeviceId, uint64_t PASideConnectionId, const xNetAddress & TargetAddress, xRelayCreateConnectionFuture & Future)                                = 0;
    virtual void PostData(uint64_t RelayServerId, uint64_t ConnectionlId, const void * Payload, size_t PayloadSize)                                                                                                = 0;
    virtual void DestroyConnection(uint64_t RelayServerId, uint64_t ConnectionId)                                                                                                                                  = 0;
    //
    virtual void CreateUdpChannel(uint64_t RelayServerId, uint64_t DeviceId, uint64_t PASideUdpChannelId, xRelayCreateUdpChannelFuture & Future)           = 0;
    virtual void KeepUdpChannelAlive(uint64_t RelayServerId, uint64_t UdpChannelId)                                                                        = 0;
    virtual void PostData(uint64_t RelayServerId, uint64_t UdpChannelId, const xel::xNetAddress & TargetAddress, const void * Payload, size_t PayloadSize) = 0;  // udp channel
    virtual void DestroyUdpChannel(uint64_t RelayServerId, uint64_t UdpChannelId)                                                                          = 0;
};
