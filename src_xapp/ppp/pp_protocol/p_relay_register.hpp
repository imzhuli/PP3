#pragma once
#include <pp_common/relay.hpp>

struct xPP_RelayRegister final : xBinaryMessage {
    void SerializeMembers() override {
        W(RelayServerType);
        W(ExportDeviceEntryAddress);
        W(ExportProxyEntryAddress);
    }
    void DeserializeMembers() override {
        R(RelayServerType);
        R(ExportDeviceEntryAddress);
        R(ExportProxyEntryAddress);
    }

    eRelayServerType RelayServerType;
    xNetAddress      ExportDeviceEntryAddress;
    xNetAddress      ExportProxyEntryAddress;
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
        W(Type);
        W(ServerId);
        W(ExportDeviceEntryAddress);
        W(ExportProxyEntryAddress);
    }
    void DeserializeMembers() override {
        R(Type);
        R(ServerId);
        R(ExportDeviceEntryAddress);
        R(ExportProxyEntryAddress);
    }

    eRelayServerType Type;
    uint64_t         ServerId;
    xNetAddress      ExportDeviceEntryAddress;
    xNetAddress      ExportProxyEntryAddress;
};

struct xPP_RelayInfoLost final : xBinaryMessage {
    void SerializeMembers() override {
        W(ServerId);
    }
    void DeserializeMembers() override {
        R(ServerId);
    }

    uint64_t ServerId;
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
