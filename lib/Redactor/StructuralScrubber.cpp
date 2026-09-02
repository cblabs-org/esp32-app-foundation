/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/Redactor/StructuralScrubber.cpp
 * Description: Zero-allocation structural scrubber for sensitive HTTP headers,
 *              JSON keys and query parameters in debug log output.
 *
 *              Matches on STRUCTURE rather than on value, so it catches secrets
 *              the device never registered — third-party bearer tokens,
 *              Set-Cookie values, session ids in responses — which literal
 *              needle matching is structurally blind to. It is also
 *              length-independent, so it covers tokens far longer than
 *              Redactor::MAX_SECRET_LEN (a JWT, for instance).
 *
 * Exported Functions/Classes:
 * - StructuralScrubber::sanitize(): Scans a buffer for known sensitive keys and masks their values.
 */

#include "StructuralScrubber.h"
#include <cstring>

namespace {

inline char lowerAscii(char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

/**
 * @brief Bounded, case-insensitive substring search.
 *
 * Hand-rolled rather than using strcasestr(), which is a GNU/BSD extension
 * rather than ISO C, and which requires NUL termination we must not assume.
 *
 * @return Pointer to the first match within [buf, buf+bufLen), or nullptr.
 */
char* findBoundedNoCase(char* buf, size_t bufLen, const char* needle, size_t needleLen) {
    if (needleLen == 0 || needleLen > bufLen) return nullptr;
    const char first = lowerAscii(needle[0]);
    const size_t last = bufLen - needleLen;
    for (size_t i = 0; i <= last; ++i) {
        if (lowerAscii(buf[i]) != first) continue;
        size_t j = 1;
        for (; j < needleLen; ++j) {
            if (lowerAscii(buf[i + j]) != lowerAscii(needle[j])) break;
        }
        if (j == needleLen) return buf + i;
    }
    return nullptr;
}

/// True for characters that sit between a key and its value.
inline bool isSeparator(char c) {
    return c == ' ' || c == '\t' || c == ':' || c == '=' || c == '"' || c == '\'';
}

/// True for characters that terminate a value. Deliberately excludes space, so
/// that "Authorization: Bearer <token>" masks the token and not just "Bearer".
inline bool isValueEnd(char c) {
    return c == '"' || c == '\'' || c == '\r' || c == '\n' || c == ',' || c == '&' || c == '}';
}

/**
 * @brief Masks the value following every occurrence of @p key within the buffer.
 * @param buffer Buffer to scrub in place.
 * @param bufLen Number of valid bytes in @p buffer.
 * @param key Key or header name to search for, case-insensitively.
 */
void scrubKeyValues(char* buffer, size_t bufLen, const char* key) {
    const size_t keyLen = std::strlen(key);
    char*  cursor    = buffer;
    size_t remaining = bufLen;

    while (char* match = findBoundedNoCase(cursor, remaining, key, keyLen)) {
        char* const bufEnd = cursor + remaining;
        char* valStart = match + keyLen;

        while (valStart < bufEnd && isSeparator(*valStart)) ++valStart;

        char* valEnd = valStart;
        while (valEnd < bufEnd && !isValueEnd(*valEnd)) ++valEnd;

        if (valEnd > valStart) {
            std::memset(valStart, '*', static_cast<size_t>(valEnd - valStart));
        }

        // Always advance past the key itself, so an empty value cannot stall the loop.
        char* next = (valEnd > match + keyLen) ? valEnd : (match + keyLen);
        remaining = static_cast<size_t>(bufEnd - next);
        cursor    = next;
    }
}

/// Header names and structured keys whose values must never reach the log.
/// "Authorization" also covers "Proxy-Authorization"; "Cookie" also covers
/// "Set-Cookie". Quoted JSON forms prevent "token" matching inside
/// "access_token", which would mask the wrong span.
constexpr const char* kSensitiveKeys[] = {
    // HTTP headers
    "Authorization",
    "Cookie",
    "X-API-Key",
    "X-Auth-Token",
    "X-Amz-Security-Token",
    // JSON keys
    "\"password\"",
    "\"passwd\"",
    "\"token\"",
    "\"access_token\"",
    "\"refresh_token\"",
    "\"id_token\"",
    "\"client_secret\"",
    "\"api_key\"",
    "\"apikey\"",
    "\"secret\"",
    // Query parameters and form-encoded bodies
    "api_key=",
    "apikey=",
    "access_token=",
    "token=",
    "password=",
    "client_secret=",
};

}  // namespace

void StructuralScrubber::sanitize(char* buffer, size_t bufferSize, bool isTruncated) {
    (void)isTruncated;  // masking already runs to the end of the buffer — conservative either way
    if (!buffer || bufferSize == 0) return;

    const size_t bufLen = strnlen(buffer, bufferSize);
    if (bufLen == 0) return;

    for (const char* key : kSensitiveKeys) {
        scrubKeyValues(buffer, bufLen, key);
    }
}
