#include "../../abstract/region_abstract.hpp"
#include "./mmdb_wrapper.hpp"

class xRegionServiceMmdb final
    : public xRegionServiceAbstract
    , public xRaii {
public:
    xRegionServiceMmdb(const char * filename);
    ~xRegionServiceMmdb();

    void GetRegion(const xel::xNetAddress & NA, xRegionFuture & Future) override;

private:
    xMmdbWrapper Mmdb;
};
