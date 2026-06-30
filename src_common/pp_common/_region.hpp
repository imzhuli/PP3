#pragma once
#include "./_.hpp"

using xContinentId = uint16_t;
using xCountryId   = uint32_t;
using xStateId     = uint32_t;
using xCityId      = uint32_t;

static constexpr const xContinentId CID_UNSPECIFIC    = 0x00;
static constexpr const xContinentId CID_ASIA          = 0x01;
static constexpr const xContinentId CID_EUROP         = 0x02;
static constexpr const xContinentId CID_NORTH_AMERICA = 0x03;
static constexpr const xContinentId CID_SOUTH_AMERICA = 0x04;
static constexpr const xContinentId CID_AFRICA        = 0x05;
static constexpr const xContinentId CID_OCEANIA       = 0x06;
static constexpr const xContinentId CID_ANTARCTIC     = 0x07;

static constexpr const xContinentId WildContinentId = xContinentId(-1);

struct xGeoInfo {
    xCountryId CountryId;
    xStateId   StateId;
    xCityId    CityId;
};

struct xRegionInfo : xGeoInfo {
    std::string CountryName;
    std::string CityName;
    std::string ShortCityName;
};

extern xCountryId   CountryCodeToCountryId(const char * CC);
extern xContinentId GetContinentIdByCountry(xCountryId CountryId);
