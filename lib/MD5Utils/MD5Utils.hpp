/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/MD5Utils/MD5Utils.hpp
 * Description: Utilities for MD5 calculation and base64-to-hex conversion.
 *
 * Exported Functions/Classes:
 * - MD5Utils: [Class] Wrapper class for MD5 generation operations.
 *   - calculateFileMD5(): Calculates the MD5 hash of a specified file on the LittleFS system.
 *   - base64ToHex(): Converts a base64 encoded hash into a hex string.
 */

#ifndef MD5_UTILS_HPP
#define MD5_UTILS_HPP

#include <string>
#include <cstdint>
#include <cstddef>

#include <Arduino.h>

/**
 * @brief Wrapper class for MD5 generation operations.
 */
class MD5Utils {
public:
    MD5Utils() = default;
    ~MD5Utils() = default;

    /**
     * @brief Calculates the MD5 hash of a specified file on the LittleFS system.
     * @param filePath The absolute path to the file on LittleFS (e.g. "/webgui.html").
     * @return A 32-character hexadecimal MD5 string, or empty if the file cannot be read.
     */
    std::string calculateFileMD5(const char* filePath);

    /**
     * @brief Converts a base64 encoded hash into a hex string.
     * @param base64Hash The base64 encoded string.
     * @return A hexadecimal string representation of the decoded bytes.
     */
    std::string base64ToHex(const std::string& base64Hash);
};

#endif // MD5_UTILS_HPP
