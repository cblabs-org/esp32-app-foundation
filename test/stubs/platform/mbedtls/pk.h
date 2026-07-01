#pragma once
#include <cstdint>
#include <cstddef>
typedef struct { int dummy; } mbedtls_pk_context;
#define MBEDTLS_RSA_PKCS_V15 0
#ifndef MBEDTLS_MD_SHA256
#define MBEDTLS_MD_SHA256 1
#endif

inline void mbedtls_pk_init(mbedtls_pk_context* ctx) {}
inline int mbedtls_pk_parse_public_key(mbedtls_pk_context* ctx, const unsigned char* key, size_t keylen) { return 0; }
inline int mbedtls_pk_verify(mbedtls_pk_context* ctx, int md_alg, const unsigned char* hash, size_t hash_len, const unsigned char* sig, size_t sig_len) { return 0; }
inline void mbedtls_pk_free(mbedtls_pk_context* ctx) {}
