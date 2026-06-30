#pragma once
#include <pp_common/_.hpp>
#include <pp_common/backend.hpp>

class xPPB_BackendAuthByUserPass final : public xBinaryMessage {
public:
    void SerializeMembers() override {
        W(UserPass);
        W(ClientIp);
    }
    void DeserializeMembers() override {
        R(UserPass);
        R(ClientIp);
    }

public:
    std::string_view UserPass;
    xNetAddress      ClientIp;
};

class xPPB_BackendAuthByUserPassResp final : public xBinaryMessage {
public:
    void SerializeMembers() override {
        W(ErrorCode);
        if (!ErrorCode) {
            W(TypeFlags, AuthId, ContinentId, CountryId, StateId, CityId, Duration, Random);
            W(AutoChangeIpOnDeviceOffline, EnableUdp, Ipv6Prefered);
            W(PoolFlags, ServerToken, ExportIp);
            W(Redirect, AuthIdForThirdPartyResource);
        }
        if (xBinaryMessageWriter::HasError()) {
            X_DEBUG_PRINTF("HasError");
        }
    }
    void DeserializeMembers() override {
        R(ErrorCode);
        if (!ErrorCode) {
            R(TypeFlags, AuthId, ContinentId, CountryId, StateId, CityId, Duration, Random);
            R(AutoChangeIpOnDeviceOffline, EnableUdp, Ipv6Prefered);
            R(PoolFlags, ServerToken, ExportIp);
            R(Redirect, AuthIdForThirdPartyResource);
        }
        if (xBinaryMessageReader::HasError()) {
            X_DEBUG_PRINTF("HasError");
        }
    }

    std::string ToString() const;

    static constexpr const uint16_t FLAG_DEVICE  = 0x01;
    static constexpr const uint16_t FLAG_STATIC  = 0x02;
    static constexpr const uint16_t FLAG_NOLIMIT = 0x04;

public:
    uint32_t     ErrorCode;
    uint16_t     TypeFlags;
    xAuthId      AuthId;
    xContinentId ContinentId;
    xCountryId   CountryId;
    xStateId     StateId;
    xCityId      CityId;
    uint32_t     Duration;  // min, 0 for change ip every time
    uint32_t     Random;
    bool         AutoChangeIpOnDeviceOffline;
    bool         EnableUdp;
    bool         Ipv6Prefered;
    std::string  PoolFlags;
    std::string  ServerToken;
    xNetAddress  ExportIp;
    std::string  Redirect;
    uint32_t     AuthIdForThirdPartyResource;
};
