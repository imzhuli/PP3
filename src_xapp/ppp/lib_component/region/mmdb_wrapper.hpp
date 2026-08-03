#pragma once
#include <maxminddb.h>

#include <pp_common/_region.hpp>

class xMmdbWrapper final : public xRaii {
public:
    xMmdbWrapper(const char * filename);
    ~xMmdbWrapper();

    xOptional<xIsoCountryName> GetCountry(const xNetAddress & Address) const;

private:
    MMDB_s MMDB = {};
};
