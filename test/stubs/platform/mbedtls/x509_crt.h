#pragma once
#include <cstddef>
typedef struct { int dummy; } mbedtls_x509_crt;
typedef struct { int dummy; } mbedtls_x509write_cert;
inline void mbedtls_x509_crt_init(mbedtls_x509_crt* crt) {}

inline void mbedtls_x509write_crt_init(mbedtls_x509write_cert* crt) {}
inline void mbedtls_x509write_crt_free(mbedtls_x509write_cert* crt) {}
inline void mbedtls_x509write_crt_set_subject_key(mbedtls_x509write_cert *crt, void *key) {}
inline void mbedtls_x509write_crt_set_issuer_key(mbedtls_x509write_cert *crt, void *key) {}
inline int mbedtls_x509write_crt_set_subject_name(mbedtls_x509write_cert *crt, const char *dir_name) { return 0; }
inline int mbedtls_x509write_crt_set_issuer_name(mbedtls_x509write_cert *crt, const char *dir_name) { return 0; }
inline int mbedtls_x509write_crt_set_serial_raw(mbedtls_x509write_cert *crt, unsigned char *serial, size_t serial_len) { return 0; }
inline int mbedtls_x509write_crt_set_validity(mbedtls_x509write_cert *crt, const char *not_before, const char *not_after) { return 0; }
inline void mbedtls_x509write_crt_set_md_alg(mbedtls_x509write_cert *crt, int md_alg) {}
inline void mbedtls_x509write_crt_set_version(mbedtls_x509write_cert *crt, int version) {}
#define MBEDTLS_X509_CRT_VERSION_3 2
inline int mbedtls_x509write_crt_set_extension(mbedtls_x509write_cert *crt, const char *oid, size_t oid_len, int critical, const unsigned char *val, size_t val_len) { return 0; }
inline int mbedtls_x509write_crt_der(mbedtls_x509write_cert *crt, unsigned char *buf, size_t size, int (*f_rng)(void *, unsigned char *, size_t), void *p_rng) { return 0; }

inline void mbedtls_x509_crt_free(mbedtls_x509_crt* crt) {}
inline int mbedtls_x509_crt_parse(mbedtls_x509_crt* chain, const unsigned char* buf, size_t buflen) { return 0; }
