#include "./mmdb_wrapper.hpp"

xMmdbWrapper::xMmdbWrapper(const char * filename) {
    if (MMDB_SUCCESS != MMDB_open(filename, MMDB_MODE_MMAP, &MMDB)) {
        return;
    }
    NoError = true;
}

xMmdbWrapper::~xMmdbWrapper() {
    if (!NoError) {
        return;
    }
    MMDB_close(&MMDB);
}

xOptional<xIsoCountryName> xMmdbWrapper::GetCountry(const xNetAddress & Address) const {
    sockaddr_in Sin;
    Address.Dump(&Sin);

    int  mmdb_error;
    auto result = MMDB_lookup_sockaddr(&MMDB, (sockaddr *)&Sin, &mmdb_error);
    if (mmdb_error != MMDB_SUCCESS) {
        cerr << "mmdb_error: " << MMDB_strerror(mmdb_error) << endl;
        return {};
    }
    if (!result.found_entry) {
        return {};
    }

    MMDB_entry_data_s
         entry_data;
    auto status = MMDB_get_value(&result.entry, &entry_data, "country", "iso_code", nullptr);
    if (status != MMDB_SUCCESS || entry_data.type != MMDB_DATA_TYPE_UTF8_STRING || entry_data.data_size != 2) {
        return {};
    }
    auto & NameRef = entry_data.utf8_string;
    return xIsoCountryName{ NameRef[0], NameRef[1] };
}
