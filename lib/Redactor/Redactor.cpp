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
 * - Redactor::describe(): Maps a RegisterResult to a static description.
 */

#include "Redactor.h"
#include <cstring>

namespace {

/**
 * @brief RAII writer lock. One definition, so no exit path can forget to unlock.
 *
 * On target this is a critical section, which is only legal because everything
 * inside it is a fixed-size memcpy — no allocation, no blocking calls.
 */
class WriterLock {
public:
    explicit WriterLock(
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
    WriterLock(const WriterLock&) = delete;
    WriterLock& operator=(const WriterLock&) = delete;
private:
#ifdef ESP_PLATFORM
    portMUX_TYPE& _lock;
#else
    std::mutex& _lock;
#endif
};

/**
 * @brief Bounded substring search. Unlike strstr(), does not rely on NUL termination.
 * @return Pointer to the first match within [buf, buf+bufLen), or nullptr.
 */
char* findBounded(char* buf, size_t bufLen, const char* secret, size_t secretLen) {
    if (secretLen == 0 || secretLen > bufLen) return nullptr;
    char* end = buf + bufLen - secretLen;
    for (char* p = buf; p <= end; ++p) {
        p = static_cast<char*>(std::memchr(p, secret[0], static_cast<size_t>(end - p) + 1));
        if (!p) break;
        if (std::memcmp(p, secret, secretLen) == 0) return p;
    }
    return nullptr;
}

}  // namespace

Redactor::Redactor() {
#ifdef ESP_PLATFORM
    _writerLock = portMUX_INITIALIZER_UNLOCKED;
#endif
    std::memset(_secrets, 0, sizeof(_secrets));
    std::memset(_secretLens, 0, sizeof(_secretLens));
}

const char* Redactor::describe(RegisterResult r) {
    switch (r) {
        case RegisterResult::Registered:     return "registered";
        case RegisterResult::AlreadyPresent: return "already registered";
        case RegisterResult::TooShort:       return "rejected: shorter than the minimum length";
        case RegisterResult::TooLong:        return "rejected: longer than the maximum secret length";
        case RegisterResult::Full:           return "rejected: redaction registry is full";
        case RegisterResult::Invalid:        return "rejected: null or empty";
    }
    return "unknown";
}

Redactor::RegisterResult Redactor::registerSecret(const char* secret) {
    if (!secret || secret[0] == '\0') return RegisterResult::Invalid;

    const size_t len = strnlen(secret, MAX_SECRET_LEN + 1);
    if (len < MIN_SECRET_LEN)  return RegisterResult::TooShort;
    if (len > MAX_SECRET_LEN) {
        _saturationCount.fetch_add(1, std::memory_order_relaxed);
        return RegisterResult::TooLong;
    }

    WriterLock lock(
#ifdef ESP_PLATFORM
        _writerLock
#else
        _writerMutex
#endif
    );

    const size_t count = _count.load(std::memory_order_relaxed);
    for (size_t i = 0; i < count; ++i) {
        if (_secretLens[i] == len && std::memcmp(_secrets[i], secret, len) == 0) {
            return RegisterResult::AlreadyPresent;
        }
    }

    if (count >= MAX_SECRETS) {
        _saturationCount.fetch_add(1, std::memory_order_relaxed);
        return RegisterResult::Full;
    }

    // APPEND ONLY. Published slots are never reordered or rewritten — the
    // lock-free read path depends on it. Descending-length scan order is
    // resolved per call in redact(), not by sorting shared storage.
    std::memcpy(_secrets[count], secret, len);
    _secrets[count][len] = '\0';
    _secretLens[count]   = len;

    // Release store publishes the fully-written slot to lock-free readers.
    _count.store(count + 1, std::memory_order_release);
    return RegisterResult::Registered;
}

Redactor::RegisterResult Redactor::registerSecret(const std::string& secret) {
    return registerSecret(secret.c_str());
}

void Redactor::redact(char* buffer, size_t bufferSize, bool isTruncated) {
    if (!buffer || bufferSize == 0) return;

    const size_t count = _count.load(std::memory_order_acquire);
    if (count == 0) return;

    const size_t bufLen = strnlen(buffer, bufferSize);
    if (bufLen == 0) return;

    // Resolve descending-length scan order into a stack-local index array.
    // Masking is destructive, so a shorter secret nested inside a longer one
    // must not be masked first — it would destroy the longer one's match.
    // Doing this per call keeps shared storage append-only (see the header).
    uint8_t order[MAX_SECRETS];
    for (size_t i = 0; i < count; ++i) order[i] = static_cast<uint8_t>(i);
    for (size_t i = 1; i < count; ++i) {
        const uint8_t key = order[i];
        size_t j = i;
        while (j > 0 && _secretLens[order[j - 1]] < _secretLens[key]) {
            order[j] = order[j - 1];
            --j;
        }
        order[j] = key;
    }

    for (size_t n = 0; n < count; ++n) {
        const size_t idx       = order[n];
        const size_t secretLen = _secretLens[idx];
        const char*  secret    = _secrets[idx];
        if (secretLen == 0) continue;

        char*  searchStart = buffer;
        size_t remaining   = bufLen;
        while (char* match = findBounded(searchStart, remaining, secret, secretLen)) {
            std::memset(match, '*', secretLen);
            remaining -= static_cast<size_t>(match - searchStart) + secretLen;
            searchStart = match + secretLen;
        }

        // Tail-prefix check: when content was cut to fit, the buffer may end
        // with a partial secret that findBounded cannot match. Mask the longest
        // suffix that is a prefix of this secret.
        if (isTruncated && secretLen > 1) {
            const size_t maxSuffix = (bufLen < secretLen - 1) ? bufLen : (secretLen - 1);
            for (size_t k = maxSuffix; k > 0; --k) {
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

bool Redactor::isFull() const {
    return _count.load(std::memory_order_acquire) >= MAX_SECRETS;
}

uint32_t Redactor::saturationCount() const {
    return _saturationCount.load(std::memory_order_relaxed);
}

void Redactor::clearSecrets() {
    WriterLock lock(
#ifdef ESP_PLATFORM
        _writerLock
#else
        _writerMutex
#endif
    );

    // Wipe the plaintext — do not leave secrets resident in RAM after a clear.
    std::memset(_secrets, 0, sizeof(_secrets));
    std::memset(_secretLens, 0, sizeof(_secretLens));
    _count.store(0, std::memory_order_release);
    _saturationCount.store(0, std::memory_order_relaxed);
}
