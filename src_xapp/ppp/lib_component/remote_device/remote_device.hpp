#pragma once
#include "../../abstract/device_abstract.hpp"

/*********************** */

struct xDeviceUdpChannelAbstract {
    virtual bool PostData(const void * DataPtr, size_t Data) = 0;
    virtual void Close()                                     = 0;
};

/************************** */

struct xDeviceCreateConnectionFuture : xFutureBase {
    xExpected<uint64_t /* ConnectionId */> Result = UnexpctedResult;
};

struct xDeviceCreateUdpChannelFuture : xFutureBase {
    xExpected<uint64_t /* UdpChannelId */> Result = UnexpctedResult;
};

struct xDeviceManagerAbstract {
    using xDeviceConectionEstablishedCallback = std::function<void(uint64_t ConnectionId, bool Connected)>;
    using xDeviceConectionClosedCallback      = std::function<void(uint64_t ConnectionId)>;
    using xDeviceConnectionDataCallback       = std::function<void(uint64_t ConnectionId, void * DataPtr, size_t DataSize)>;
    using xDeviceUdpChannelDataCallback       = std::function<void(uint64_t UdpChannelId, const xNetAddress & TargetAddress, void * DataPtr, size_t DataSize)>;

    struct xEventCallback {
        xDeviceConectionEstablishedCallback OnConnectionEstablished;
        xDeviceConectionClosedCallback      OnConnectionClosed;
        xDeviceConnectionDataCallback       OnConnectionData;
        xDeviceUdpChannelDataCallback       OnUdpChannelData;
    };

    virtual void SetDeviceEventCallback(const xEventCallback & Callbacks)                                                                                      = 0;
    virtual void CreateConnection(uint64_t SourceContextId, uint64_t DeviceId, const xel::xNetAddress & TargetAddress, xDeviceCreateConnectionFuture & Future) = 0;
    virtual void CreateConnection(uint64_t SourceContextId, uint64_t DeviceId, const std::string & TargetHost, xDeviceCreateConnectionFuture & Future)         = 0;
    virtual void DestroyConnection(uint64_t ConnectionId)                                                                                                      = 0;
    virtual void CreateUdpChannel(uint64_t SourceContextId, uint64_t DeviceId, xDeviceCreateUdpChannelFuture & Future)                                         = 0;
    virtual void DestroyUdpChannel(uint64_t UdpChannelId)                                                                                                      = 0;
    virtual void PostData(uint64_t ConnectionId, const void * DataPtr, size_t DataSize)                                                                        = 0;
    virtual void PostData(uint64_t UdpChannelId, const xNetAddress & TargetAddress, const void * DataPtr, size_t DataSize)                                     = 0;
};
