#include "./_region.hpp"

xCountryId CountryCodeToCountryId(const xIsoCountryName & CountryName) {
    return (static_cast<uint32_t>(MakeUnsigned(CountryName[0])) << 8) + (static_cast<uint32_t>(MakeUnsigned(CountryName[1])));
}

xContinentId GetContinentIdByCountry(xCountryId CountryId) {
    return 0;
}
