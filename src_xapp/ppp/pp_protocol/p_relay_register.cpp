#include "./p_relay_register.hpp"

#include <xel/crypto_wrapper/md5.hpp>

void xPP_RelayDispatcherSlaveRegister::SerializeMembers() {
    assert(TimestampMS);
    char HashBuffer[256];
    int  len  = snprintf(HashBuffer, sizeof(HashBuffer), "%s%" PRIu64 "", "xPP_RelayDispatcherSlaveRegister@@", TimestampMS);
    auto Hash = Md5(HashBuffer, len);
    W(TimestampMS);
    W(std::string_view((const char *)Hash.data(), Hash.size()));
}

void xPP_RelayDispatcherSlaveRegister::DeserializeMembers() {
    auto Hash = std::string_view();
    R(TimestampMS);
    R(Hash);
    if (GetReader()->HasError()) {
        return;
    }
    char HashBuffer[256];
    int  len        = snprintf(HashBuffer, sizeof(HashBuffer), "%s%" PRIu64 "", "xPP_RelayDispatcherSlaveRegister@@", TimestampMS);
    auto SourceHash = Md5(HashBuffer, len);
    if (Hash != std::string_view((const char *)SourceHash.data(), SourceHash.size())) {
        GetReader()->SetError();
    }
}
