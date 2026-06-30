#include "./_region.hpp"

xCountryId CountryCodeToCountryId(const char * CC) {
    X_RUNTIME_ASSERT(CC);
    auto len = strlen(CC);
    if (len != 2) {
        return 0;
    }
    return (static_cast<uint32_t>(CC[0]) << 8) + (static_cast<uint32_t>(CC[1]));
}

xContinentId GetContinentIdByCountry(xCountryId CountryId) {
    return 0;
}
