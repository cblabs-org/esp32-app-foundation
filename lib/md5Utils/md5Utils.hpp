/*
 * ESP32 App Foundation (c) 2026
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/md5Utils/md5Utils.hpp
 * Description: Utilities for MD5 calculation and base64-to-hex conversion.
 *
 * Exported Functions/Classes:
 * - Md5Utils: [Class] Wrapper class for MD5 generation operations.
 *   - calculateFileMD5(): Calculates the MD5 hash of a specified file on the LittleFS system.
 *   - base64ToHex(): Converts a base64 encoded MD5 hash into a hex string.
 */

#ifndef MD5_UTILS_HPP
#define MD5_UTILS_HPP

#ifndef NATIVE_BUILD
#include <Arduino.h>
#else
#include <string>
#include <cstdint>
#include <cstddef>
#ifndef HEX
#define HEX 16
#endif
// Provide String stub if not already available
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

/**
 * @brief Wrapper class for MD5 generation operations.
 */
class Md5Utils {
public:
    Md5Utils() = default;
    ~Md5Utils() = default;

    /**
     * @brief Calculates the MD5 hash of a specified file on the LittleFS system.
     * @param filePath The absolute path to the file on LittleFS (e.g. "/webgui.html").
     * @return A 32-character hexadecimal MD5 string, or empty if the file cannot be read.
     */
    String calculateFileMD5(const char* filePath);

    /**
     * @brief Converts a base64 encoded MD5 hash (such as those from GitHub headers) into a hex string.
     * @param base64Hash The base64 encoded string.
     * @return A 32-character hexadecimal MD5 string.
     */
    String base64ToHex(String base64Hash);
};

#endif // MD5_UTILS_HPP
