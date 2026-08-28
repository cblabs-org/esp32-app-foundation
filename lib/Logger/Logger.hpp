/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/Logger/Logger.hpp
 * Description: ESP-IDF based logging utility with redaction support for sensitive information.
 *
 * Exported Functions/Classes:
 * - Logger: Static utility class for system-wide logging.
 *   - registerSecret(): Adds a string to the redaction list.
 *   - logSecure(): Securely logs a formatted string using ESP_LOGI and redacting secrets.
 *
 * Macros:
 * - LOG_SECURE(tag, fmt, ...): Secure wrapper for ESP_LOGI to redact secrets.
 */
#ifndef LOGGER_HPP
#define LOGGER_HPP                                          ///< Logger Hpp

#include <Arduino.h>
#include <vector>
#include <string>

#include "platform/LogShim.h"

#define LOG_SECURE(tag, fmt, ...) Logger::logSecure(tag, fmt, ##__VA_ARGS__)

/**
 * @brief Static utility class for system-wide logging with redaction support.
 */
class Logger {
public:
    /**
     * @brief Registers a sensitive string to be redacted from all future log output.
     * @param secret The plaintext string that should not appear in the logs.
     */
    static void registerSecret(const String& secret);
    /** @brief Register secret. */
    static void registerSecret(const std::string& secret);
    /** @brief Register secret. */
    static void registerSecret(const char* secret);

    /**
     * @brief Logs a message securely, replacing any registered secrets with asterisks.
     * @param tag Subsystem or tag.
     * @param format Printf-style format string.
     */
    static void logSecure(const char* tag, const char* format, ...);

private:
    static std::vector<std::string> secrets; ///< Registry of sensitive strings

    /**
     * @brief Scans a char buffer and replaces any registered secrets with asterisks in-place.
     * @param buffer The character array to redact.
     */
    static void redactInPlace(char* buffer);
};

#endif // LOGGER_HPP
