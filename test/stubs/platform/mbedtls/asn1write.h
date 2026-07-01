#pragma once
#include <stddef.h>
#define MBEDTLS_ASN1_CONTEXT_SPECIFIC 0x80
#define MBEDTLS_ASN1_CONSTRUCTED 0x20
#define MBEDTLS_ASN1_SEQUENCE 0x10

inline int mbedtls_asn1_write_raw_buffer(unsigned char **p, unsigned char *start, const unsigned char *buf, size_t size) { return size; }
inline int mbedtls_asn1_write_len(unsigned char **p, unsigned char *start, size_t len) { return 1; }
inline int mbedtls_asn1_write_tag(unsigned char **p, unsigned char *start, unsigned char tag) { return 1; }
