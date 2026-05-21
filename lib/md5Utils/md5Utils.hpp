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

#include <Arduino.h>

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
