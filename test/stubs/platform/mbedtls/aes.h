#pragma once
#include <cstdint>
#include <cstddef>
typedef struct { int dummy; } mbedtls_aes_context;
#define MBEDTLS_AES_ENCRYPT 1
#define MBEDTLS_AES_DECRYPT 0

inline void mbedtls_aes_init(mbedtls_aes_context* ctx) {}
inline int mbedtls_aes_setkey_enc(mbedtls_aes_context* ctx, const unsigned char* key, unsigned int keybits) { return 0; }
inline int mbedtls_aes_setkey_dec(mbedtls_aes_context* ctx, const unsigned char* key, unsigned int keybits) { return 0; }
inline int mbedtls_aes_crypt_cbc(mbedtls_aes_context* ctx, int mode, size_t length, unsigned char iv[16], const unsigned char* input, unsigned char* output) { return 0; }
inline void mbedtls_aes_free(mbedtls_aes_context* ctx) {}
