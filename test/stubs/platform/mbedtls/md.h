#pragma once
#include <cstdint>
#include <cstddef>
typedef struct { int dummy; } mbedtls_md_context_t;
typedef struct { int dummy; } mbedtls_md_info_t;
#define MBEDTLS_MD_SHA256 1

inline const mbedtls_md_info_t* mbedtls_md_info_from_type(int type) { static mbedtls_md_info_t i; return &i; }
inline void mbedtls_md_init(mbedtls_md_context_t* ctx) {}
inline int mbedtls_md_setup(mbedtls_md_context_t* ctx, const mbedtls_md_info_t* md_info, int hmac) { return 0; }
inline int mbedtls_md_starts(mbedtls_md_context_t* ctx) { return 0; }
inline int mbedtls_md_update(mbedtls_md_context_t* ctx, const unsigned char* input, size_t ilen) { return 0; }
inline int mbedtls_md_finish(mbedtls_md_context_t* ctx, unsigned char* output) { return 0; }
inline void mbedtls_md_free(mbedtls_md_context_t* ctx) {}
inline int mbedtls_md(const mbedtls_md_info_t* md_info, const unsigned char* input, size_t ilen, unsigned char* output) { return 0; }
