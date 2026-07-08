#include "./mmdb_wrapper.hpp"

#include <iostream>
#include <vector>
#include <xel/core/core_min.hpp>
#include <xel/core/optional.hpp>
#include <xel/core/string.hpp>
#include <xel/network/net_address.hpp>

using namespace std;
using namespace xel;

static auto MMDBFilename = "./test_assets/mmdb";
static auto TestAddress  = xNetAddress::Parse("175.178.22.69");

xHolder<xMmdbWrapper> Holder;

// void CountCN4() {
//     size_t Total = 0;
//     auto   Addr  = xNetAddress::Make4();
//     for (unsigned i = 1; i <= 223; ++i) {
//         Addr.SA4[0]   = i;
//         size_t Total0 = 0;
//         for (unsigned j = 0; j < 256; ++j) {
//             Addr.SA4[1]   = j;
//             size_t Total1 = 0;
//             for (unsigned k = 0; k < 256; ++k) {
//                 Addr.SA4[2] = k;
//                 for (unsigned l = 1; l < 256; ++l) {
//                     Addr.SA4[3] = l;
//                     if (Addr.IsPrivate() || Addr.IsLoopback()) {
//                         continue;
//                     }
//                     if (TestIp4CN(Addr)) {
//                         ++Total0;
//                         ++Total1;
//                         ++Total;
//                     }
//                 }
//             }
//         }

//         cout << "0 Total0 set: " << i << "-> " << Total0 << endl;
//     }
// }

int main(int, char **) {
    X_SCOPE_GUARD([] { Holder.CreateValue(MMDBFilename); }, [] { Holder.Destroy(); });
    auto R = Holder->GetCountry(TestAddress);
    if (!R) {
        cout << "country not found" << endl;
    } else {
        constexpr const auto China = xIsoCountryName{ 'C', 'N' };
        cout << "country is China ? " << YN(*R == China) << endl;
    }
    return 0;
}
