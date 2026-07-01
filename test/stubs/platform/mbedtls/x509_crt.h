#pragma once
#include <cstddef>
typedef struct { int dummy; } mbedtls_x509_crt;
inline void mbedtls_x509_crt_init(mbedtls_x509_crt* crt) {}
inline void mbedtls_x509_crt_free(mbedtls_x509_crt* crt) {}
inline int mbedtls_x509_crt_parse(mbedtls_x509_crt* chain, const unsigned char* buf, size_t buflen) { return 0; }
