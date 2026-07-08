#pragma once
#include <maxminddb.h>

#include <pp_common/_.hpp>

using xIsoCountryName = std::array<char, 2>;

class xMmdbWrapper final : xNonCopyable {
public:
    xMmdbWrapper(const char * filename);
    ~xMmdbWrapper();
    operator bool() const { return NoError; }

    xOptional<xIsoCountryName> GetCountry(const xNetAddress & Address) const;

private:
    bool   NoError = false;
    MMDB_s MMDB    = {};
};
