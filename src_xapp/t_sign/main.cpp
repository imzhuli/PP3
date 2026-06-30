#include <ada.h>

#include <pp_common/_.hpp>
#include <xel/crypto_wrapper/sha.hpp>

std::string SignPath(const std::string & Key, const std::string & Secure, uint64_t Timestamp, const std::string & Path) {
    auto Src  = Key + std::to_string(Timestamp) + Path;
    auto Hash = xel::HmacSha256(Src.data(), Src.size(), Secure.data(), Secure.size());
    auto Sign = xel::StrToHexLower(Hash.data(), Hash.size());
    return Sign;
}

int main(int argc, char ** argv) {
    auto CL = xCommandLine{
        argc,
        argv,
        {
            { 'k', "key", "key", true },
            { 's', "secret", "secret", true },
            { 'p', "path", "path", true },
        },
    };

    auto Now  = GetUnixTimestamp();
    auto Key  = "myip"s;
    auto Sec  = "myip"s;
    auto Path = "/proxy/static/manifest.json"s;

    auto OptKey = CL["key"];
    auto OptSec = CL["secret"];
    if (OptKey) {
        Key = *OptKey;
    }
    if (OptSec) {
        Sec = *OptSec;
    }

    auto ExPathOpt = CL["path"];
    if (true || !ExPathOpt) {
        auto sign    = SignPath(Key, Sec, Now, Path);
        auto request = "http://54.205.125.114:8002/proxy/static/manifest.json?app_key=" + Key + "&timestamp=" + std::to_string(Now) + "&sign=" + sign;
        printf("%s\n", request.c_str());
        // return 0;
    }

    auto ExPath = *ExPathOpt;
    auto Url    = ada::parse(ExPath);

    auto ExSign  = SignPath(Key, Sec, Now, std::string(Url->get_pathname()));
    auto Request = ExPath + "?app_key=" + Key + "&timestamp=" + std::to_string(Now) + "&sign=" + ExSign;
    cout << Request << endl;

    return 0;
}
