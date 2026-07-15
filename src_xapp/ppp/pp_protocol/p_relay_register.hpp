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