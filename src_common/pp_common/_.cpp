#include "./_.hpp"

static constexpr const char * StaicSignSalt = "!#@SFas098xc()*&";

std::string ToLower(const std::string & Src) {
    auto Dst = Src;
    for (auto & C : Dst) {
        C = tolower(C);
    }
    return Dst;
}

std::ostream & operator<<(std::ostream & OS, const xNetAddress & Address) {
    OS << Address.ToString();
    return OS;
}

uint32_t HashString(const char * S) {
    uint32_t H = 0;
    while (auto C = *(S++)) {
        H = H * 33 + C;
    }
    return H;
}

uint32_t HashString(const char * S, size_t Len) {
    uint32_t H = 0;
    for (size_t I = 0; I < Len; ++I) {
        H = H * 33 + S[I];
    }
    return H;
}

uint32_t HashString(const std::string_view & S) {
    return HashString(S.data(), S.size());
}

std::string AppSign(uint64_t Timestamp, const std::string & SecretKey, const void * DataPtr, size_t Size) {
    auto TS     = std::to_string(Timestamp);
    auto Source = StaicSignSalt + TS + SecretKey + std::string((const char *)DataPtr, Size);
    auto D      = xel::Sha256(Source.data(), Source.size());
    return TS + ":" + StrToHex(D.data(), D.size());
}

bool ValidateAppSign(const std::string & Sign, const std::string & SecretKey, const void * DataPtr, size_t Size) {
    auto Segs = Split(Sign, ":");
    if (Segs.size() != 2) {
        return false;
    }
    uint64_t TimestampMS = (uint64_t)std::strtoumax(Segs[0].c_str(), nullptr, 10);
    uint64_t TimeDiff    = std::abs(SignedDiff(TimestampMS, xel::GetTimestampMS()));
    if (TimeDiff >= 60'000) {
        return false;
    }
    auto Source = StaicSignSalt + Segs[0] + SecretKey + std::string((const char *)DataPtr, Size);
    auto D      = xel::Sha256(Source.data(), Source.size());

    return StrToHex(D.data(), D.size()) == Segs[1];
}

uint32_t ExtractIndexFromServerId(uint64_t ServerId) {
    return High32(ServerId >> 12);
}

///////////////////
