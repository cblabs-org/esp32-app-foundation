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
// Removed typedef to avoid clash
#ifndef STRING_STUB_DEFINED
#define STRING_STUB_DEFINED
class String {
public:
    String() {}
    String(const char* s) : _str(s ? s : "") {}
    String(const std::string& s) : _str(s) {}
    String(int value, int base) {
        if (base == 16) {
            char buf[20];
            snprintf(buf, sizeof(buf), "%x", value);
            _str = buf;
        } else {
            _str = std::to_string(value);
        }
    }
    const char* c_str() const { return _str.c_str(); }
    bool isEmpty() const { return _str.empty(); }
    int length() const { return _str.length(); }
    String operator+(const char* s) const { return String(_str + s); }
    String operator+(const String& s) const { return String(_str + s._str); }
    bool operator==(const String& other) const { return _str == other._str; }
    bool operator==(const char* s) const { return _str == s; }
    char operator[](int i) const { return _str[i]; }
    String& operator+=(char c) { _str += c; return *this; }
    String& operator+=(const String& s) { _str += s._str; return *this; }
    friend String operator+(const char* lhs, const String& rhs) {
        return String(lhs) + rhs;
    }
    operator std::string() const { return _str; }
private:
    std::string _str;
};
#endif
#endif
#include <vector>

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
