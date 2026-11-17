/*
 * ESP32 App Foundation (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/MD5Utils/md5Utils.cpp
 * Description: Implementation of MD5 calculation and base64-to-hex conversion utilities.
 */

#include "MD5Utils.hpp"
#include "platform/LogShim.h"

static constexpr const char* TAG = "MD5Utils";

#ifndef PIO_UNIT_TESTING
#include <LittleFS.h>
#include <mbedtls/md5.h>
#include <mbedtls/base64.h>
#endif

/**
 * @brief Calculates the MD5 hash of a specified file on the LittleFS system.
 * @param filePath The absolute path to the file on LittleFS (e.g. "/webgui.html").
 * @return A 32-character hexadecimal MD5 string, or empty if the file cannot be read.
 */
std::string MD5Utils::calculateFileMD5(const char* filePath) {
#ifndef PIO_UNIT_TESTING
    File file = LittleFS.open(filePath, "r");
    if (!file || file.isDirectory()) {
        ESP_LOGW(TAG, "calculateFileMD5: cannot open file '%s'", filePath);
        return std::string();
    }

    mbedtls_md5_context md5Ctx;
    mbedtls_md5_init(&md5Ctx);
    mbedtls_md5_starts(&md5Ctx);

    const size_t kReadBufSize = 512;
    uint8_t readBuf[kReadBufSize];
    while (file.available()) {
        size_t bytesRead = file.read(readBuf, kReadBufSize);
        mbedtls_md5_update(&md5Ctx, readBuf, bytesRead);
    }

    uint8_t digest[16];
    mbedtls_md5_finish(&md5Ctx, digest);
    mbedtls_md5_free(&md5Ctx);
    file.close();

    char hexOut[33];
    for (int i = 0; i < 16; i++) {
        sprintf(&hexOut[i * 2], "%02x", digest[i]);
    }
    hexOut[32] = '\0';

    return std::string(hexOut);
#else
    // Mock implementation for native testing
    return std::string("d41d8cd98f00b204e9800998ecf8427e"); // MD5 of empty string
#endif
}

/**
 * @brief Converts a base64 encoded hash into a hex string.
 * @param base64Hash The base64 encoded string.
 * @return A hexadecimal string representation of the decoded bytes.
 */
std::string MD5Utils::base64ToHex(const std::string& base64Hash) {
#ifndef PIO_UNIT_TESTING
    if (base64Hash.empty()) {
        ESP_LOGW(TAG, "base64ToHex: empty input string");
        return std::string();
    }

    // First call: determine output size
    size_t decodedLen = 0;
    int ret = mbedtls_base64_decode(
        nullptr, 0, &decodedLen,
        reinterpret_cast<const unsigned char*>(base64Hash.c_str()),
        base64Hash.length());

    if (ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL || decodedLen == 0) {
        ESP_LOGW(TAG, "base64ToHex: invalid base64 input");
        return std::string();
    }

    // Allocate decode buffer
    auto* decoded = new (std::nothrow) unsigned char[decodedLen];
    if (decoded == nullptr) {
        ESP_LOGE(TAG, "base64ToHex: allocation failed (%u bytes)", static_cast<unsigned>(decodedLen));
        return std::string();
    }

    // Second call: actual decode
    size_t actualLen = 0;
    ret = mbedtls_base64_decode(
        decoded, decodedLen, &actualLen,
        reinterpret_cast<const unsigned char*>(base64Hash.c_str()),
        base64Hash.length());

    if (ret != 0) {
        ESP_LOGW(TAG, "base64ToHex: decode failed (ret=%d)", ret);
        delete[] decoded;
        return std::string();
    }

    // Convert to hex string
    std::string hexResult;
    hexResult.reserve(actualLen * 2);
    for (size_t i = 0; i < actualLen; i++) {
        char hexBuf[3];
        snprintf(hexBuf, sizeof(hexBuf), "%02x", decoded[i]);
        hexResult += hexBuf;
    }

    delete[] decoded;
    return hexResult;
#else
    // Mock: return a fixed hex string for testing
    return std::string("d41d8cd98f00b204e9800998ecf8427e");
#endif
}
