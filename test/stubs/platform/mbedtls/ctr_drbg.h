#pragma once
#include <cstddef>
typedef struct { int dummy; } mbedtls_ctr_drbg_context;
inline void mbedtls_ctr_drbg_init(mbedtls_ctr_drbg_context* ctx) {}
inline void mbedtls_ctr_drbg_free(mbedtls_ctr_drbg_context* ctx) {}
inline int mbedtls_ctr_drbg_seed(mbedtls_ctr_drbg_context* ctx, int (*f_entropy)(void*, unsigned char*, size_t), void* p_entropy, const unsigned char* custom, size_t len) { return 0; }
inline int mbedtls_ctr_drbg_random(void* p_rng, unsigned char* output, size_t output_len) { return 0; }
