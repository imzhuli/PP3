#pragma once
#include <pp_common/relay.hpp>

struct xPP_RelayRegister final : xBinaryMessage {
    void SerializeMembers() override {
        W(RelayServerType);
        W(ExportDeviceSideAddress);
        W(ExportProxySideAddrfess);
    }
    void DeserializeMembers() override {
        R(RelayServerType);
        R(ExportDeviceSideAddress);
        R(ExportProxySideAddrfess);
    }

    eRelayServerType RelayServerType;
    xNetAddress      ExportDeviceSideAddress;
    xNetAddress      ExportProxySideAddrfess;
};

struct xPP_RelayRegisterResp final : xBinaryMessage {
    void SerializeMembers() override {
        W(ServerId);
    }
    void DeserializeMembers() override {
        R(ServerId);
    }
    uint64_t ServerId;
};

struct xPP_RelayInfoBroadcast final : xBinaryMessage {
    void SerializeMembers() override {
        W(ServerId);
        W(ExportDeviceSideAddress);
        W(ExportProxySideAddrfess);
    }
    void DeserializeMembers() override {
        R(ServerId);
        R(ExportDeviceSideAddress);
        R(ExportProxySideAddrfess);
    }

    uint64_t    ServerId;
    xNetAddress ExportDeviceSideAddress;
    xNetAddress ExportProxySideAddrfess;
};

struct xPP_RelayDispatcherSlaveRegister final : xBinaryMessage {
    void SerializeMembers() override;
    void DeserializeMembers() override;

    uint64_t TimestampMS;
};

struct xPP_RelayDispatcherSlaveRegisterResp final : xBinaryMessage {
    void SerializeMembers() override {
        W(Accepted);
    }
    void DeserializeMembers() override {
        R(Accepted);
    }

    bool Accepted;
};
