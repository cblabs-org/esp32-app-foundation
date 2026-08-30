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
#include <string.h>

std::vector<std::string> Redactor::_secrets;

void Redactor::registerSecret(const std::string& secret) {
    if (secret.empty()) return;
    for (const auto& s : _secrets) {
        if (s == secret) return;
    }
    _secrets.push_back(secret);
}

void Redactor::registerSecret(const String& secret) {
    if (secret.length() == 0) return;
    registerSecret(std::string(secret.c_str()));
}

void Redactor::registerSecret(const char* secret) {
    if (secret) registerSecret(std::string(secret));
}

void Redactor::redact(char* buffer, size_t bufferSize) {
    if (_secrets.empty() || buffer == nullptr || bufferSize == 0) return;

    std::string message(buffer);
    bool modified = false;

    for (const auto& secret : _secrets) {
        size_t index = message.find(secret);
        while (index != std::string::npos) {
            std::string replacement(secret.length(), '*');
            message.replace(index, secret.length(), replacement);
            modified = true;
            index = message.find(secret, index + secret.length());
        }
    }

    if (modified) {
        strncpy(buffer, message.c_str(), bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
}
