#pragma once
#include <maxminddb.h>

#include <pp_common/_.hpp>

using xIsoCountryName = std::array<char, 2>;

class xMmdbWrapper : xNonCopyable {
public:
    xMmdbWrapper(const char * filename);
    ~xMmdbWrapper();

    xOptional<xIsoCountryName> GetCountry(const xNetAddress & Address) const;

private:
    bool   Inited = false;
    MMDB_s MMDB   = {};
};
