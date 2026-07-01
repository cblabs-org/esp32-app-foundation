#pragma once
#include <stddef.h>
#define MBEDTLS_ECP_DP_SECP256R1 1
inline int mbedtls_ecp_gen_key(int grp_id, void* grp, int (*f_rng)(void *, unsigned char *, size_t), void *p_rng) { return 0; }
