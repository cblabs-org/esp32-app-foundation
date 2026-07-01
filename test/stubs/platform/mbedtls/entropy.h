#pragma once
#include <cstddef>
typedef struct { int dummy; } mbedtls_entropy_context;
inline void mbedtls_entropy_init(mbedtls_entropy_context* ctx) {}
inline void mbedtls_entropy_free(mbedtls_entropy_context* ctx) {}
inline int mbedtls_entropy_func(void* data, unsigned char* output, size_t len) { return 0; }
