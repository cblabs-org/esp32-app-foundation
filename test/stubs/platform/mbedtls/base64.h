#pragma once
#include <cstddef>
#include <cstdint>

#ifndef MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL
#define MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL -0x002A
#endif
#ifndef MBEDTLS_ERR_BASE64_INVALID_CHARACTER
#define MBEDTLS_ERR_BASE64_INVALID_CHARACTER -0x002C
#endif

namespace mbedtls_stub_detail {

inline int base64_decode_char(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return (c - 'a') + 26;
    if (c >= '0' && c <= '9') return (c - '0') + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

inline bool is_base64_ws(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

} // namespace mbedtls_stub_detail

inline int mbedtls_base64_encode(unsigned char* dst, size_t dlen, size_t* olen,
                                 const unsigned char* src, size_t slen) {
    if (slen == 0) {
        if (olen != nullptr) *olen = 0;
        return 0;
    }
    if (src == nullptr) {
        if (olen != nullptr) *olen = 0;
        return MBEDTLS_ERR_BASE64_INVALID_CHARACTER;
    }

    size_t encLen = (slen / 3U + (slen % 3U != 0U ? 1U : 0U)) * 4U;
    if (dst == nullptr || dlen < encLen) {
        if (olen != nullptr) *olen = encLen + 1U;
        return MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL;
    }

    static constexpr const char kTable[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    size_t i = 0;
    size_t j = 0;
    while (i + 3U <= slen) {
        uint32_t v = (static_cast<uint32_t>(src[i]) << 16) |
                     (static_cast<uint32_t>(src[i + 1U]) << 8) |
                     static_cast<uint32_t>(src[i + 2U]);
        dst[j++] = static_cast<unsigned char>(kTable[(v >> 18) & 0x3FU]);
        dst[j++] = static_cast<unsigned char>(kTable[(v >> 12) & 0x3FU]);
        dst[j++] = static_cast<unsigned char>(kTable[(v >> 6) & 0x3FU]);
        dst[j++] = static_cast<unsigned char>(kTable[v & 0x3FU]);
        i += 3U;
    }

    size_t rem = slen - i;
    if (rem == 1U) {
        uint32_t v = static_cast<uint32_t>(src[i]) << 16;
        dst[j++] = static_cast<unsigned char>(kTable[(v >> 18) & 0x3FU]);
        dst[j++] = static_cast<unsigned char>(kTable[(v >> 12) & 0x3FU]);
        dst[j++] = '=';
        dst[j++] = '=';
    } else if (rem == 2U) {
        uint32_t v = (static_cast<uint32_t>(src[i]) << 16) |
                     (static_cast<uint32_t>(src[i + 1U]) << 8);
        dst[j++] = static_cast<unsigned char>(kTable[(v >> 18) & 0x3FU]);
        dst[j++] = static_cast<unsigned char>(kTable[(v >> 12) & 0x3FU]);
        dst[j++] = static_cast<unsigned char>(kTable[(v >> 6) & 0x3FU]);
        dst[j++] = '=';
    }

    if (olen != nullptr) *olen = encLen;
    if (dlen > encLen) dst[encLen] = '\0';
    return 0;
}

inline int mbedtls_base64_decode(unsigned char* dst, size_t dlen, size_t* olen,
                                 const unsigned char* src, size_t slen) {
    if (slen == 0) {
        if (olen != nullptr) *olen = 0;
        return 0;
    }
    if (src == nullptr) {
        if (olen != nullptr) *olen = 0;
        return MBEDTLS_ERR_BASE64_INVALID_CHARACTER;
    }

    size_t nonWsCount = 0;
    size_t padCount = 0;
    int last4[4] = {0, 0, 0, 0};

    for (size_t idx = 0; idx < slen; ++idx) {
        unsigned char c = src[idx];
        if (mbedtls_stub_detail::is_base64_ws(c)) {
            continue;
        }
        if (c == '=') {
            padCount++;
            if (padCount > 2U) {
                if (olen != nullptr) *olen = 0;
                return MBEDTLS_ERR_BASE64_INVALID_CHARACTER;
            }
            last4[nonWsCount % 4U] = 0;
            nonWsCount++;
        } else {
            if (padCount > 0U) {
                if (olen != nullptr) *olen = 0;
                return MBEDTLS_ERR_BASE64_INVALID_CHARACTER;
            }
            int val = mbedtls_stub_detail::base64_decode_char(c);
            if (val < 0) {
                if (olen != nullptr) *olen = 0;
                return MBEDTLS_ERR_BASE64_INVALID_CHARACTER;
            }
            last4[nonWsCount % 4U] = val;
            nonWsCount++;
        }
    }

    if (nonWsCount == 0U) {
        if (olen != nullptr) *olen = 0;
        return 0;
    }
    if ((nonWsCount % 4U) != 0U) {
        if (olen != nullptr) *olen = 0;
        return MBEDTLS_ERR_BASE64_INVALID_CHARACTER;
    }
    if (padCount == 1U && (last4[2] & 0x03) != 0) {
        if (olen != nullptr) *olen = 0;
        return MBEDTLS_ERR_BASE64_INVALID_CHARACTER;
    }
    if (padCount == 2U && (last4[1] & 0x0F) != 0) {
        if (olen != nullptr) *olen = 0;
        return MBEDTLS_ERR_BASE64_INVALID_CHARACTER;
    }

    size_t decLen = (nonWsCount / 4U) * 3U - padCount;
    if (dst == nullptr || dlen < decLen) {
        if (olen != nullptr) *olen = decLen;
        return MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL;
    }

    size_t outIdx = 0;
    int q[4] = {0, 0, 0, 0};
    int qLen = 0;
    int qPad = 0;

    for (size_t idx = 0; idx < slen; ++idx) {
        unsigned char c = src[idx];
        if (mbedtls_stub_detail::is_base64_ws(c)) {
            continue;
        }
        if (c == '=') {
            q[qLen++] = 0;
            qPad++;
        } else {
            q[qLen++] = mbedtls_stub_detail::base64_decode_char(c);
        }

        if (qLen == 4) {
            dst[outIdx++] = static_cast<unsigned char>((q[0] << 2) | (q[1] >> 4));
            if (qPad < 2) {
                dst[outIdx++] = static_cast<unsigned char>(((q[1] & 0x0F) << 4) | (q[2] >> 2));
            }
            if (qPad == 0) {
                dst[outIdx++] = static_cast<unsigned char>(((q[2] & 0x03) << 6) | q[3]);
            }
            qLen = 0;
            qPad = 0;
        }
    }

    if (olen != nullptr) *olen = decLen;
    return 0;
}
