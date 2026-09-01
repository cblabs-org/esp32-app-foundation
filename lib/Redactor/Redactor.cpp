/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/Redactor/Redactor.cpp
 * Description: Redactor implementation for sensitive string masking.
 *
 * Exported Functions/Classes:
 * - Redactor::registerSecret(): Adds a secret to the global redaction registry.
 * - Redactor::redact(): Scans a buffer and masks all registered secrets.
 */

#include "Redactor.h"
#include <cstring>
#include <cstdio>

namespace {
    class WriterLock {
    public:
        WriterLock(
#ifdef ESP_PLATFORM
            portMUX_TYPE& lock
#else
            std::mutex& lock
#endif
        ) : _lock(lock) {
#ifdef ESP_PLATFORM
            portENTER_CRITICAL(&_lock);
#else
            _lock.lock();
#endif
        }
        ~WriterLock() {
#ifdef ESP_PLATFORM
            portEXIT_CRITICAL(&_lock);
#else
            _lock.unlock();
#endif
        }
    private:
#ifdef ESP_PLATFORM
        portMUX_TYPE& _lock;
#else
        std::mutex& _lock;
#endif
    };

    char* findBounded(char* buf, size_t bufLen, const char* secret, size_t secretLen) {
        if (secretLen == 0 || secretLen > bufLen) return nullptr;
        char* end = buf + bufLen - secretLen;
        for (char* p = buf; p <= end; ++p) {
            p = static_cast<char*>(std::memchr(p, secret[0], (end - p) + 1));
            if (!p) break;
            if (std::memcmp(p, secret, secretLen) == 0) return p;
        }
        return nullptr;
    }
}

Redactor::Redactor() {
#ifdef ESP_PLATFORM
    _writerLock = portMUX_INITIALIZER_UNLOCKED;
#endif
    std::memset(_secrets, 0, sizeof(_secrets));
    std::memset(_secretLens, 0, sizeof(_secretLens));
}

RegisterResult Redactor::registerSecret(const char* secret) {
    if (!secret || secret[0] == '\0') return RegisterResult::Invalid;
    size_t len = std::strlen(secret);
    if (len < 4) return RegisterResult::TooShort;
    if (len >= MAX_SECRET_LEN) return RegisterResult::TooLong;

    WriterLock lock(
#ifdef ESP_PLATFORM
        _writerLock
#else
        _writerMutex
#endif
    );

    size_t count = _count.load(std::memory_order_relaxed);
    for (size_t i = 0; i < count; ++i) {
        if (_secretLens[i] == len && std::memcmp(_secrets[i], secret, len) == 0) {
            return RegisterResult::AlreadyPresent;
        }
    }

    if (count >= MAX_SECRETS) {
        return RegisterResult::Full;
    }

    // Insertion sort by descending length to prevent partial masking of nested secrets
    size_t insertIdx = count;
    while (insertIdx > 0 && _secretLens[insertIdx - 1] < len) {
        std::memcpy(_secrets[insertIdx], _secrets[insertIdx - 1], MAX_SECRET_LEN);
        _secretLens[insertIdx] = _secretLens[insertIdx - 1];
        insertIdx--;
    }

    std::memcpy(_secrets[insertIdx], secret, len);
    _secrets[insertIdx][len] = '\0';
    _secretLens[insertIdx] = len;

    _count.fetch_add(1, std::memory_order_release);
    return RegisterResult::Registered;
}

RegisterResult Redactor::registerSecret(const std::string& secret) {
    return registerSecret(secret.c_str());
}

void Redactor::redact(char* buffer, size_t bufferSize, bool isTruncated) {
    if (!buffer || bufferSize == 0) return;

    size_t count = _count.load(std::memory_order_acquire);
    if (count == 0) return;

    size_t bufLen = strnlen(buffer, bufferSize);
    if (bufLen == 0) return;

    for (size_t i = 0; i < count; ++i) {
        size_t secretLen = _secretLens[i];
        const char* secret = _secrets[i];
        if (secretLen == 0) continue;

        char* searchStart = buffer;
        size_t remaining = bufLen;
        while (char* match = findBounded(searchStart, remaining, secret, secretLen)) {
            std::memset(match, '*', secretLen);
            remaining -= (match - searchStart) + secretLen;
            searchStart = match + secretLen;
        }

        // Tail-prefix truncation check
        if (isTruncated) {
            size_t maxSuffixLen = (bufLen < secretLen - 1) ? bufLen : (secretLen - 1);
            for (size_t k = maxSuffixLen; k > 0; --k) {
                if (std::memcmp(buffer + bufLen - k, secret, k) == 0) {
                    std::memset(buffer + bufLen - k, '*', k);
                    break;
                }
            }
        }
    }
}

void Redactor::sanitize(char* buffer, size_t bufferSize, bool isTruncated) {
    redact(buffer, bufferSize, isTruncated);
}

size_t Redactor::secretCount() const {
    return _count.load(std::memory_order_acquire);
}

void Redactor::clearSecrets() {
    WriterLock lock(
#ifdef ESP_PLATFORM
        _writerLock
#else
        _writerMutex
#endif
    );
    
    std::memset(_secrets, 0, sizeof(_secrets));
    std::memset(_secretLens, 0, sizeof(_secretLens));
    _count.store(0, std::memory_order_release);
}
