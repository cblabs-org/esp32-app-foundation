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

Redactor::Redactor() {
#ifdef ESP_PLATFORM
    _writerLock = portMUX_INITIALIZER_UNLOCKED;
#endif
}

bool Redactor::registerSecret(const char* secret) {
    if (!secret || secret[0] == '\0') return false;
    size_t len = std::strlen(secret);
    if (len < 4) return false;

#ifdef ESP_PLATFORM
    portENTER_CRITICAL(&_writerLock);
#else
    std::lock_guard<std::mutex> lock(_writerMutex);
#endif

    size_t count = _count.load(std::memory_order_relaxed);
    for (size_t i = 0; i < count; ++i) {
        if (_secrets[i] == secret) {
#ifdef ESP_PLATFORM
            portEXIT_CRITICAL(&_writerLock);
#endif
            return true;
        }
    }

    if (count >= MAX_SECRETS) {
#ifdef ESP_PLATFORM
        portEXIT_CRITICAL(&_writerLock);
#endif
        std::printf("Redactor warning: MAX_SECRETS capacity reached. Secret rejected.\n");
        return false;
    }

    _secrets[count] = secret;
    _count.fetch_add(1, std::memory_order_release);

#ifdef ESP_PLATFORM
    portEXIT_CRITICAL(&_writerLock);
#endif
    return true;
}

bool Redactor::registerSecret(const std::string& secret) {
    return registerSecret(secret.c_str());
}

void Redactor::redact(char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) return;

    size_t count = _count.load(std::memory_order_acquire);
    if (count == 0) return;

    size_t bufLen = strnlen(buffer, bufferSize);
    if (bufLen == 0) return;

    for (size_t i = 0; i < count; ++i) {
        const std::string& secret = _secrets[i];
        size_t secretLen = secret.length();
        if (secretLen == 0) continue;

        char* searchStart = buffer;
        while (char* match = std::strstr(searchStart, secret.c_str())) {
            std::memset(match, '*', secretLen);
            searchStart = match + secretLen;
        }
    }
}

void Redactor::sanitize(char* buffer, size_t bufferSize) {
    redact(buffer, bufferSize);
}

size_t Redactor::secretCount() const {
    return _count.load(std::memory_order_acquire);
}

void Redactor::clearSecrets() {
#ifdef ESP_PLATFORM
    portENTER_CRITICAL(&_writerLock);
#else
    std::lock_guard<std::mutex> lock(_writerMutex);
#endif
    
    _count.store(0, std::memory_order_release);

#ifdef ESP_PLATFORM
    portEXIT_CRITICAL(&_writerLock);
#endif
}
