/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/platform/NativeBuildStubs.hpp
 * Description: Provides stub implementations of Arduino/ESP-IDF types for
 *              native desktop compilation and unit testing. All modules and
 *              libraries that need the Arduino String class under NATIVE_BUILD
 *              should include this header instead of defining their own inline
 *              stub. The STRING_STUB_DEFINED guard ensures only one definition
 *              is active regardless of include order.
 *
 * Exported Classes:
 * - String: Minimal Arduino-compatible String class backed by std::string.
 *   Provides: constructors, c_str(), isEmpty(), length(), operator+,
 *   operator+=, operator==, operator[], operator=, implicit std::string
 *   conversion, and integer-to-hex formatting (base 16).
 */

#pragma once

#ifdef NATIVE_BUILD

#include <string>
#include <cstdio>

#ifndef STRING_STUB_DEFINED
#define STRING_STUB_DEFINED

/**
 * @brief Minimal Arduino-compatible String stub for native desktop builds.
 *
 * Wraps std::string to satisfy compilation of framework modules and libraries
 * that use the Arduino String API. Only methods actually exercised by the
 * codebase are implemented — this is not a full Arduino String replacement.
 */
class String {
public:
    String() {}
    String(const char* s) : _str(s ? s : "") {}
    String(const std::string& s) : _str(s) {}

    /// @brief Integer-to-string constructor with optional hex base.
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
    int length() const { return static_cast<int>(_str.length()); }

    // --- Assignment ---
    String& operator=(const char* s) { _str = s ? s : ""; return *this; }

    // --- Concatenation ---
    String operator+(const char* s) const { return String(_str + s); }
    String operator+(const String& s) const { return String(_str + s._str); }
    String& operator+=(char c) { _str += c; return *this; }
    String& operator+=(const String& s) { _str += s._str; return *this; }

    friend String operator+(const char* lhs, const String& rhs) {
        return String(lhs) + rhs;
    }

    // --- Comparison ---
    bool operator==(const String& other) const { return _str == other._str; }
    bool operator==(const char* s) const { return _str == s; }

    // --- Element access ---
    char operator[](int i) const { return _str[i]; }

    // --- Conversion ---
    operator std::string() const { return _str; }

private:
    std::string _str;
};

#endif // STRING_STUB_DEFINED
#endif // NATIVE_BUILD
