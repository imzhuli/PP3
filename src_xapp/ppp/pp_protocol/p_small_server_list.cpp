#include "./p_small_server_list.hpp"

#include <xel/crypto_wrapper/md5.hpp>

void xPP_GetSmallServerListResp::SerializeMembers() {
    assert(ServerList && ServerListSize <= MAX_SMALL_SERVER_LIST_SIZE);
    auto Writer = GetWriter();
    auto Start  = Writer->GetCurrentPosition();
    W(VersionTimestampMS);
    W(ServerListSize);
    for (size_t I = 0; I < ServerListSize; ++I) {
        auto & Ref = (*ServerList)[I];
        W(Ref.ServerId);
        W(Ref.Address);
    }
    if (Writer->HasError()) {
        return;
    }
    auto End     = Writer->GetCurrentPosition();
    auto Digest  = xel::Md5(Start, End - Start);
    auto Digest1 = xel::Md5(Digest.data(), 12);
    W(std::string_view{ (const char *)Digest1.data(), 16 });
}

void xPP_GetSmallServerListResp::DeserializeMembers() {
    assert(ServerList);
    auto Reader = GetReader();
    auto Start  = Reader->GetCurrentPosition();
    R(VersionTimestampMS);
    R(ServerListSize);
    if (ServerListSize > MAX_SMALL_SERVER_LIST_SIZE) {
        Reader->SetError();
        return;
    }
    for (size_t I = 0; I < ServerListSize; ++I) {
        auto & Ref = (*ServerList)[I];
        R(Ref.ServerId);
        R(Ref.Address);
    }
    auto End = Reader->GetCurrentPosition();

    auto Checksum = std::string_view();
    R(Checksum);
    if (Reader->HasError()) {
        return;
    }

    auto Digest  = xel::Md5(Start, End - Start);
    auto Digest1 = xel::Md5(Digest.data(), 12);
    if (memcmp(Checksum.data(), Digest1.data(), 16)) {
        Reader->SetError();
    }
}
