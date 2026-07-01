#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <cstring>
#include <new>

typedef struct { std::string buffer; } mbedtls_md_context_t;
typedef struct { int dummy; } mbedtls_md_info_t;
#define MBEDTLS_MD_SHA256 1

inline const mbedtls_md_info_t* mbedtls_md_info_from_type(int type) { static mbedtls_md_info_t i; return &i; }
inline void mbedtls_md_init(mbedtls_md_context_t* ctx) { ctx->buffer.clear(); }
inline int mbedtls_md_setup(mbedtls_md_context_t* ctx, const mbedtls_md_info_t* md_info, int hmac) { return 0; }
inline int mbedtls_md_starts(mbedtls_md_context_t* ctx) { ctx->buffer.clear(); return 0; }
inline int mbedtls_md_update(mbedtls_md_context_t* ctx, const unsigned char* input, size_t ilen) { 
    if (ilen > 0) ctx->buffer.append((const char*)input, ilen);
    return 0; 
}
inline int mbedtls_md_finish(mbedtls_md_context_t* ctx, unsigned char* out) { 
    size_t len = ctx->buffer.length();
    const uint8_t* data = (const uint8_t*)ctx->buffer.data();
    
    // Portable SHA-256 implementation
    static const uint32_t H0[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
    };
    static const uint32_t K[64] = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
        0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
        0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
        0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
        0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
        0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
        0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
        0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
        0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
        0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
        0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
        0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
        0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
        0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
    };

    auto rotr = [](uint32_t x, uint32_t n) -> uint32_t {
        return (x >> n) | (x << (32u - n));
    };

    uint32_t h[8];
    memcpy(h, H0, sizeof(h));

    size_t padLen = len + 1;
    while (padLen % 64 != 56) padLen++;
    padLen += 8;

    uint8_t* msg = new (std::nothrow) uint8_t[padLen]();
    if (msg == nullptr) return -1;
    memcpy(msg, data, len);
    msg[len] = 0x80;

    uint64_t bitLen = (uint64_t)len * 8u;
    for (int i = 0; i < 8; i++) {
        msg[padLen - 8 + i] = (uint8_t)(bitLen >> (56 - i * 8));
    }

    for (size_t block = 0; block < padLen; block += 64) {
        uint32_t w[64] = {0};
        for (int i = 0; i < 16; i++) {
            w[i] = ((uint32_t)msg[block + i*4]     << 24) |
                   ((uint32_t)msg[block + i*4 + 1] << 16) |
                   ((uint32_t)msg[block + i*4 + 2] <<  8) |
                   ((uint32_t)msg[block + i*4 + 3]);
        }
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = rotr(w[i-15], 7) ^ rotr(w[i-15], 18) ^ (w[i-15] >> 3);
            uint32_t s1 = rotr(w[i-2],  17) ^ rotr(w[i-2],  19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];

        for (int i = 0; i < 64; i++) {
            uint32_t S1    = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch    = (e & f) ^ (~e & g);
            uint32_t temp1 = hh + S1 + ch + K[i] + w[i];
            uint32_t S0    = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t maj   = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;

            hh = g; g = f; f = e; e = d + temp1;
            d  = c; c = b; b = a; a = temp1 + temp2;
        }

        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

    delete[] msg;

    for (int i = 0; i < 8; i++) {
        out[i*4]     = (uint8_t)(h[i] >> 24);
        out[i*4 + 1] = (uint8_t)(h[i] >> 16);
        out[i*4 + 2] = (uint8_t)(h[i] >>  8);
        out[i*4 + 3] = (uint8_t)(h[i]);
    }

    return 0; 
}
inline void mbedtls_md_free(mbedtls_md_context_t* ctx) { ctx->buffer.clear(); }
inline int mbedtls_md(const mbedtls_md_info_t* md_info, const unsigned char* input, size_t ilen, unsigned char* output) { 
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_update(&ctx, input, ilen);
    mbedtls_md_finish(&ctx, output);
    return 0;
}
