#include "./command.hpp"

#include <mbedtls/aes.h>
#include <mbedtls/base64.h>

std::vector<ubyte> Encrypt(const void * Data, size_t DataSize, const std::string & AesKey) {
    assert(AesKey.size() == 128);

    auto Blocks        = (DataSize + 16) / 16;
    auto Length        = Blocks * 16;
    auto R             = std::vector<ubyte>(Length * 2);
    auto Dst           = R.data();
    auto Src           = Dst + Length;
    auto Padding       = Src + DataSize;
    auto PaddingLength = Length - DataSize;
    memcpy(Src, Data, DataSize);
    for (size_t i = 0; i < PaddingLength; ++i) {
        *Padding++ = (uint8_t)PaddingLength;
    }
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_enc(&aes_ctx, (const ubyte *)AesKey.data(), 128);
    auto G = xScopeGuard([&] { mbedtls_aes_free(&aes_ctx); });

    for (size_t i = 0; i < Blocks; ++i) {
        mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, Src, Dst);
        Src += 16;
        Dst += 16;
    }

    R.resize(Length);
    return R;
}

std::vector<ubyte> Decrypt(const void * Data, size_t DataSize, const std::string & AesKey) {
    assert(AesKey.size() == 128);

    if (!DataSize || DataSize % 16) {
        return {};
    }

    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_dec(&aes_ctx, (const ubyte *)AesKey.data(), AesKey.size());
    auto G = xScopeGuard([&] { mbedtls_aes_free(&aes_ctx); });

    auto R      = std::vector<ubyte>(DataSize);
    auto Blocks = DataSize / 16;
    auto Src    = (const ubyte *)Data;
    auto Dst    = R.data();
    for (size_t i = 0; i < Blocks; ++i) {
        mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_DECRYPT, Src, Dst);
        Src += 16;
        Dst += 16;
    }
    auto padding = (size_t)(R[DataSize - 1]);
    if (DataSize < padding) {
        return {};
    }
    R.resize(DataSize - padding);
    return R;
}
