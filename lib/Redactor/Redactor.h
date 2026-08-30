/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/Redactor/Redactor.h
 * Description: In-place secret redaction utility for sensitive data protection.
 *
 * Exported Functions/Classes:
 * - Redactor: Static utility class for registering secrets and redacting them from buffers.
 *   - registerSecret(): Adds a string to the redaction list.
 *   - redact(): Scans a character buffer and replaces registered secrets with asterisks.
 */
#ifndef REDACTOR_H
#define REDACTOR_H

#include <Arduino.h>
#include <vector>
#include <string>

/**
 * @brief Static utility class for registering sensitive strings and redacting them from buffers.
 *
 * Thread safety contract: registerSecret() must be called only during
 * single-threaded initialization (before FreeRTOS scheduler starts or from
 * setup()). After init, redact() is safe to call from any task — the secrets
 * vector is read-only at that point. A late registerSecret() call concurrent
 * with redact() would race on the underlying std::vector.
 */
class Redactor {
public:
    /**
     * @brief Registers a sensitive string to be redacted from all future output.
     * @param secret The plaintext string that should not appear in outputs.
     */
    static void registerSecret(const String& secret);

    /**
     * @brief Registers a sensitive string to be redacted from all future output.
     * @param secret The plaintext std::string that should not appear in outputs.
     */
    static void registerSecret(const std::string& secret);

    /**
     * @brief Registers a sensitive C-string to be redacted from all future output.
     * @param secret The null-terminated C-string that should not appear in outputs.
     */
    static void registerSecret(const char* secret);

    /**
     * @brief Scans a char buffer and replaces any registered secrets with asterisks in-place.
     * @param buffer The character array to redact.
     * @param bufferSize The maximum capacity of the buffer.
     */
    static void redact(char* buffer, size_t bufferSize);

private:
    static std::vector<std::string> _secrets; ///< Registry of sensitive strings
};

#endif // REDACTOR_H
