#include "./region_mmdb.hpp"

xRegionServiceMmdb::xRegionServiceMmdb(const char * filename)
    : Mmdb(filename) {
    if (!xRaii::IsReady(Mmdb)) {
        return;
    }
    SetRaiiReady();
}

xRegionServiceMmdb::~xRegionServiceMmdb() {
}

void xRegionServiceMmdb::GetRegion(const xel::xNetAddress & NA, xRegionFuture & Future) {
    auto R = Mmdb.GetCountry(NA);
    Future.SetReady();
    if (!R) {
        return;
    }
    Future.CountryId = CountryCodeToCountryId(*R);
}