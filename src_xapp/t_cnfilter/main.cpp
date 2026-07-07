#include <iostream>
#include <vector>
#include <xel/core/core_min.hpp>
#include <xel/core/optional.hpp>
#include <xel/core/string.hpp>
#include <xel/network/net_address.hpp>
//
#include <maxminddb.h>

using namespace std;
using namespace xel;

static const char * MMDBFilename = "./test_assets/mmdb";
static const char * TestAddress  = "175.178.22.69";

static auto MMDB = MMDB_s{};

static void SkipArray(const MMDB_entry_data_list_s *& Entry);
static void SkipMap(const MMDB_entry_data_list_s *& Entry);

std::string_view ExtractStringView(const MMDB_entry_data_list_s * Entry) {
    X_RUNTIME_ASSERT(Entry);
    auto & Data = Entry->entry_data;
    X_RUNTIME_ASSERT(Data.type == MMDB_DATA_TYPE_UTF8_STRING);
    return { Data.utf8_string, Data.data_size };
}

void SkipArray(const MMDB_entry_data_list_s *& Entry) {
    const auto & EntryData = Entry->entry_data;
    X_RUNTIME_ASSERT(EntryData.type == MMDB_DATA_TYPE_ARRAY);
    for (auto I = decltype(EntryData.data_size)(0); I < EntryData.data_size; ++I) {
        Entry          = Entry->next;
        auto & SubData = Entry->entry_data;
        switch (SubData.type) {
            case MMDB_DATA_TYPE_ARRAY: {
                cout << "skip array in array" << endl;
                SkipArray(Entry);
            } break;
            case MMDB_DATA_TYPE_MAP: {
                cout << "skip map in array" << endl;
                SkipMap(Entry);
            } break;
            default: {
                cout << "skip value in array" << endl;
            } break;
        }
    }
}

void SkipMap(const MMDB_entry_data_list_s *& Entry) {
    const auto & EntryData = Entry->entry_data;
    X_RUNTIME_ASSERT(EntryData.type == MMDB_DATA_TYPE_MAP);
    for (auto I = decltype(EntryData.data_size)(0); I < EntryData.data_size; ++I) {
        Entry          = Entry->next;  // skip-key
        Entry          = Entry->next;
        auto & SubData = Entry->entry_data;
        switch (SubData.type) {
            case MMDB_DATA_TYPE_ARRAY: {
                SkipArray(Entry);
            } break;
            case MMDB_DATA_TYPE_MAP: {
                SkipMap(Entry);
            } break;
            default: {
            } break;
        }
    }
}

xOptional<std::string> ExtractPathValue(const MMDB_entry_data_list_s *& Entry, std::string_view * Paths, size_t PathCount) {
    if (!Entry) {
        return {};
    }
    const auto & EntryData = Entry->entry_data;
    if (PathCount == 0) {  // final stage: has to be value,
        return ExtractStringView(Entry);
    }
    X_RUNTIME_ASSERT(EntryData.type == MMDB_DATA_TYPE_MAP);
    for (auto I = decltype(EntryData.data_size)(0); I < EntryData.data_size; ++I) {
        Entry          = Entry->next;  // skip-key
        auto Key       = ExtractStringView(Entry);
        Entry          = Entry->next;
        auto & SubData = Entry->entry_data;
        if (Key != *Paths) {
            switch (SubData.type) {
                case MMDB_DATA_TYPE_ARRAY: {
                    SkipArray(Entry);
                } break;
                case MMDB_DATA_TYPE_MAP: {
                    SkipMap(Entry);
                } break;
                default: {
                } break;
            }
            continue;
        }
        return ExtractPathValue(Entry, ++Paths, --PathCount);
    }
    return {};
}

bool TestIp4CN(const xNetAddress & Address) {
    sockaddr_in Sin;
    Address.Dump(&Sin);

    int  mmdb_error;
    auto result = MMDB_lookup_sockaddr(&MMDB, (sockaddr *)&Sin, &mmdb_error);
    if (mmdb_error != MMDB_SUCCESS) {
        cerr << "mmdb_error: " << MMDB_strerror(mmdb_error) << endl;
        return false;
    }
    if (!result.found_entry) {
        return false;
    }

    auto entry_data_list = (MMDB_entry_data_list_s *)nullptr;
    int  status          = MMDB_get_entry_data_list(&result.entry, &entry_data_list);
    if (status != MMDB_SUCCESS) {
        cerr << "failed to get entry data list: " << MMDB_strerror(status) << endl;
        return false;
    }
    X_SCOPE_GUARD([=] { MMDB_free_entry_data_list(entry_data_list); });

    if (NULL != entry_data_list) {
        // MMDB_dump_entry_data_list(stdout, entry_data_list, 2);
    }
    auto PathList = std::vector<std::string_view>{
        "country", "iso_code"
    };
    auto V = ExtractPathValue(XR((const MMDB_entry_data_list_s *)(entry_data_list)), PathList.data(), PathList.size());
    if (V) {
        return *V == "CN"sv;
    }
    return false;
}

bool TestIp4CN(const char * AddressStr) {
    return TestIp4CN(xNetAddress::Parse(AddressStr));
}

void CountCN4() {
    size_t Total = 0;
    auto   Addr  = xNetAddress::Make4();
    for (unsigned i = 1; i <= 223; ++i) {
        Addr.SA4[0]   = i;
        size_t Total0 = 0;
        for (unsigned j = 0; j < 256; ++j) {
            Addr.SA4[1]   = j;
            size_t Total1 = 0;
            for (unsigned k = 0; k < 256; ++k) {
                Addr.SA4[2] = k;
                for (unsigned l = 1; l < 256; ++l) {
                    Addr.SA4[3] = l;
                    if (Addr.IsPrivate() || Addr.IsLoopback()) {
                        continue;
                    }
                    if (TestIp4CN(Addr)) {
                        ++Total0;
                        ++Total1;
                        ++Total;
                    }
                }
            }
        }

        cout << "0 Total0 set: " << i << "-> " << Total0 << endl;
    }
}

int main(int, char **) {
    X_SCOPE_GUARD([] { X_RUNTIME_ASSERT(MMDB_SUCCESS == MMDB_open(MMDBFilename, MMDB_MODE_MMAP, &MMDB)); }, [] { MMDB_close(&MMDB); });
    cout << "Address: " << TestAddress << ", IsCN: " << YN(TestIp4CN(TestAddress)) << endl;

    CountCN4();
    return 0;
}
