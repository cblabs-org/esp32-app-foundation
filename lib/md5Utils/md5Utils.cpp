/*
 * ESP32 App Foundation (c) 2026
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/md5Utils/md5Utils.cpp
 * Description: Implementation of MD5 calculation and base64-to-hex conversion utilities.
 */

#include "md5Utils.hpp"

#ifndef PIO_UNIT_TESTING
#include <LittleFS.h>
#include <mbedtls/md5.h>
#endif

/**
 * @brief Calculates the MD5 hash of a specified file on the LittleFS system.
 * @param filePath The absolute path to the file on LittleFS (e.g. "/webgui.html").
 * @return A 32-character hexadecimal MD5 string, or empty if the file cannot be read.
 */
String Md5Utils::calculateFileMD5(const char* filePath) {
#ifndef PIO_UNIT_TESTING
    File file = LittleFS.open(filePath, "r");
    if (!file || file.isDirectory()) {
        return String();
    }

    mbedtls_md5_context ctx;
    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts(&ctx);

    const size_t bufferSize = 512;
    uint8_t buffer[bufferSize];
    while (file.available()) {
        size_t bytesRead = file.read(buffer, bufferSize);
        mbedtls_md5_update(&ctx, buffer, bytesRead);
    }

    uint8_t hash[16];
    mbedtls_md5_finish(&ctx, hash);
    mbedtls_md5_free(&ctx);
    file.close();

    char hexString[33];
    for (int i = 0; i < 16; i++) {
        sprintf(&hexString[i * 2], "%02x", hash[i]);
    }
    hexString[32] = '\0';

    return String(hexString);
#else
    // Mock implementation for native testing
    return String("d41d8cd98f00b204e9800998ecf8427e"); // MD5 of empty string
#endif
}

/**
 * @brief Converts a base64 encoded MD5 hash (such as those from GitHub headers) into a hex string.
 * @param base64Hash The base64 encoded string.
 * @return A 32-character hexadecimal MD5 string.
 */
String Md5Utils::base64ToHex(String base64Hash) {
    const char base64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int base64Value[256];
    for (int i = 0; i < 256; i++) base64Value[i] = -1;
    for (int i = 0; i < 64; i++) base64Value[(int)base64Table[i]] = i;

    int len = base64Hash.length();
    if (len == 0) return String();

    int padding = (base64Hash[len - 1] == '=') + ((len > 1 && base64Hash[len - 2] == '=') ? 1 : 0);
    int binarySize = (len * 3) / 4 - padding;
    
    if (binarySize <= 0) return String();

    uint8_t* binaryData = new uint8_t[binarySize];
    int index = 0, buffer = 0, bits = 0;
    
    for (int i = 0; i < len; i++) {
        int value = base64Value[(int)base64Hash[i]];
        if (value == -1) continue;
        buffer = (buffer << 6) | value;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (index < binarySize) {
                binaryData[index++] = (buffer >> bits) & 0xFF;
            }
        }
    }

    String hexHash = "";
    for (int i = 0; i < binarySize; i++) {
        if (binaryData[i] < 16) hexHash += '0';
        hexHash += String(binaryData[i], HEX);
    }

    delete[] binaryData;
    return hexHash;
}
