#pragma once
#include <cstdint>
#include <cstddef>
typedef struct { int dummy; } mbedtls_pk_context;
#define MBEDTLS_RSA_PKCS_V15 0
#ifndef MBEDTLS_MD_SHA256
#define MBEDTLS_MD_SHA256 1
#endif
#define MBEDTLS_PK_ECKEY 1
typedef void mbedtls_pk_info_t;

inline const mbedtls_pk_info_t* mbedtls_pk_info_from_type(int type) { return nullptr; }
inline int mbedtls_pk_setup(mbedtls_pk_context* ctx, const mbedtls_pk_info_t* info) { return 0; }
inline void* mbedtls_pk_ec(mbedtls_pk_context& ctx) { return nullptr; }
inline int mbedtls_pk_write_key_der(mbedtls_pk_context *ctx, unsigned char *buf, size_t size) { return 0; }

inline void mbedtls_pk_init(mbedtls_pk_context* ctx) {}
inline int mbedtls_pk_parse_public_key(mbedtls_pk_context* ctx, const unsigned char* key, size_t keylen) { return 0; }
inline int mbedtls_pk_verify(mbedtls_pk_context* ctx, int md_alg, const unsigned char* hash, size_t hash_len, const unsigned char* sig, size_t sig_len) { return 0; }
inline void mbedtls_pk_free(mbedtls_pk_context* ctx) {}
