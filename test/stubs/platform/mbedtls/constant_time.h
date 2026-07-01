#pragma once
#include <cstddef>
#include <cstring>
inline int mbedtls_ct_memcmp(const void* a, const void* b, size_t n) { return memcmp(a, b, n); }
