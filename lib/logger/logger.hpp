/*
 * ESP32 App Foundation (c) 2026
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/logger/logger.hpp
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
#define LOGGER_HPP

#ifndef NATIVE_BUILD
#include <Arduino.h>
#else
#include <stdint.h>
#include <stddef.h>
#include <string>
typedef std::string String;
#endif
#include <vector>

#ifndef PIO_UNIT_TESTING
#include "esp_log.h"
#else
#include <stdio.h>
#define ESP_LOGI(tag, fmt, ...) printf("I (%s): " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) printf("W (%s): " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, fmt, ...) printf("E (%s): " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) printf("D (%s): " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGV(tag, fmt, ...) printf("V (%s): " fmt "\n", tag, ##__VA_ARGS__)
#endif

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

    /**
     * @brief Logs a message securely, replacing any registered secrets with asterisks.
     * @param tag Subsystem or tag.
     * @param format Printf-style format string.
     */
    static void logSecure(const char* tag, const char* format, ...);

private:
    static std::vector<String> secrets; ///< Registry of sensitive strings

    /**
     * @brief Scans a char buffer and replaces any registered secrets with asterisks in-place.
     * @param buffer The character array to redact.
     */
    static void redactInPlace(char* buffer);
};

#endif // LOGGER_HPP
