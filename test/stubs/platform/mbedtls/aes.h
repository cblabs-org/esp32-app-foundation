#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

#ifndef MBEDTLS_AES_ENCRYPT
#define MBEDTLS_AES_ENCRYPT 1
#endif
#ifndef MBEDTLS_AES_DECRYPT
#define MBEDTLS_AES_DECRYPT 0
#endif
#ifndef MBEDTLS_ERR_AES_INVALID_KEY_LENGTH
#define MBEDTLS_ERR_AES_INVALID_KEY_LENGTH -0x0020
#endif
#ifndef MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH
#define MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH -0x0022
#endif

typedef struct {
    uint8_t key[32];
    unsigned int keybits;
    int mode;
    bool valid;
} mbedtls_aes_context;

namespace mbedtls_aes_stub_detail {

inline uint64_t load_le64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= static_cast<uint64_t>(p[i]) << (i * 8);
    }
    return v;
}

inline void store_le64(uint8_t* p, uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        p[i] = static_cast<uint8_t>((v >> (i * 8)) & 0xFFU);
    }
}

inline uint64_t rotl64(uint64_t x, unsigned int n) {
    n &= 63U;
    return (n == 0U) ? x : ((x << n) | (x >> (64U - n)));
}

inline uint64_t feistel_f(uint64_t x, uint64_t rk, unsigned int round) {
    uint64_t z = x ^ rk ^ (0x9E3779B97F4A7C15ULL * static_cast<uint64_t>(round + 1U));
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z = z ^ (z >> 31) ^ rotl64(z, 17U);
    return z;
}

inline uint64_t round_key(const uint64_t k[4], unsigned int round) {
    return k[round & 3U] ^ rotl64(k[(round + 1U) & 3U], (round * 7U + 13U) & 63U);
}

inline void encrypt_block(const uint8_t keyBytes[32], const uint8_t in[16], uint8_t out[16]) {
    uint64_t k[4];
    for (int i = 0; i < 4; ++i) {
        k[i] = load_le64(keyBytes + i * 8);
    }
    uint64_t l = load_le64(in);
    uint64_t r = load_le64(in + 8);
    for (unsigned int round = 0; round < 16U; ++round) {
        uint64_t nextL = r;
        uint64_t nextR = l ^ feistel_f(r, round_key(k, round), round);
        l = nextL;
        r = nextR;
    }
    store_le64(out, l);
    store_le64(out + 8, r);
}

inline void decrypt_block(const uint8_t keyBytes[32], const uint8_t in[16], uint8_t out[16]) {
    uint64_t k[4];
    for (int i = 0; i < 4; ++i) {
        k[i] = load_le64(keyBytes + i * 8);
    }
    uint64_t l = load_le64(in);
    uint64_t r = load_le64(in + 8);
    for (int round = 15; round >= 0; --round) {
        unsigned int ur = static_cast<unsigned int>(round);
        uint64_t prevR = l;
        uint64_t prevL = r ^ feistel_f(l, round_key(k, ur), ur);
        l = prevL;
        r = prevR;
    }
    store_le64(out, l);
    store_le64(out + 8, r);
}

} // namespace mbedtls_aes_stub_detail

inline void mbedtls_aes_init(mbedtls_aes_context* ctx) {
    if (ctx == nullptr) return;
    std::memset(ctx->key, 0, sizeof(ctx->key));
    ctx->keybits = 0;
    ctx->mode = -1;
    ctx->valid = false;
}

inline void mbedtls_aes_free(mbedtls_aes_context* ctx) {
    if (ctx == nullptr) return;
    volatile uint8_t* p = reinterpret_cast<volatile uint8_t*>(ctx->key);
    for (size_t i = 0; i < sizeof(ctx->key); ++i) {
        p[i] = 0;
    }
    ctx->keybits = 0;
    ctx->mode = -1;
    ctx->valid = false;
}

inline int mbedtls_aes_setkey_enc(mbedtls_aes_context* ctx, const unsigned char* key, unsigned int keybits) {
    if (ctx == nullptr || key == nullptr) return MBEDTLS_ERR_AES_INVALID_KEY_LENGTH;
    if (keybits != 128U && keybits != 192U && keybits != 256U) return MBEDTLS_ERR_AES_INVALID_KEY_LENGTH;
    std::memset(ctx->key, 0, sizeof(ctx->key));
    std::memcpy(ctx->key, key, keybits / 8U);
    ctx->keybits = keybits;
    ctx->mode = MBEDTLS_AES_ENCRYPT;
    ctx->valid = true;
    return 0;
}

inline int mbedtls_aes_setkey_dec(mbedtls_aes_context* ctx, const unsigned char* key, unsigned int keybits) {
    if (ctx == nullptr || key == nullptr) return MBEDTLS_ERR_AES_INVALID_KEY_LENGTH;
    if (keybits != 128U && keybits != 192U && keybits != 256U) return MBEDTLS_ERR_AES_INVALID_KEY_LENGTH;
    std::memset(ctx->key, 0, sizeof(ctx->key));
    std::memcpy(ctx->key, key, keybits / 8U);
    ctx->keybits = keybits;
    ctx->mode = MBEDTLS_AES_DECRYPT;
    ctx->valid = true;
    return 0;
}

inline int mbedtls_aes_crypt_cbc(mbedtls_aes_context* ctx, int mode, size_t length,
                                 unsigned char iv[16], const unsigned char* input, unsigned char* output) {
    if (ctx == nullptr || !ctx->valid || iv == nullptr || input == nullptr || output == nullptr) {
        return MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH;
    }
    if ((length % 16U) != 0U) {
        return MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH;
    }
    if ((mode != MBEDTLS_AES_ENCRYPT && mode != MBEDTLS_AES_DECRYPT) || ctx->mode != mode) {
        return MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH;
    }

    for (size_t off = 0; off < length; off += 16U) {
        if (mode == MBEDTLS_AES_ENCRYPT) {
            uint8_t block[16];
            for (size_t i = 0; i < 16U; ++i) {
                block[i] = input[off + i] ^ iv[i];
            }
            mbedtls_aes_stub_detail::encrypt_block(ctx->key, block, output + off);
            std::memcpy(iv, output + off, 16U);
        } else {
            uint8_t cipherBlock[16];
            std::memcpy(cipherBlock, input + off, 16U);
            uint8_t plainBlock[16];
            mbedtls_aes_stub_detail::decrypt_block(ctx->key, cipherBlock, plainBlock);
            for (size_t i = 0; i < 16U; ++i) {
                output[off + i] = plainBlock[i] ^ iv[i];
            }
            std::memcpy(iv, cipherBlock, 16U);
        }
    }
    return 0;
}
