/*
 * ESP32 App Foundation (c) 2026
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/deviceCrypto/deviceCrypto.cpp
 * Description: Generic cryptographic utility module providing AES-256-CBC 
 *              encryption and decryption bounded to a persistent hardware key.
 */

#include "deviceCrypto.hpp"
#include "../logger/logger.hpp"
#include "platform/LogShim.h"
#include <string.h>

static const char* TAG = "DeviceCrypto";

#ifndef PIO_UNIT_TESTING
#include <Preferences.h>
#include <mbedtls/aes.h>
#include <mbedtls/base64.h>
#include <mbedtls/md.h>
#include <esp_random.h>
#include <esp_mac.h>
#else
// Mock includes for native testing
#include <stdlib.h>
#include <time.h>
#endif

/**
 * @brief Mounts the NVS partition to extract the master encryption key.
 *        If no key is configured, uses hardware MAC key derivation to generate and save one.
 */
void DeviceCrypto::begin() {
#ifndef PIO_UNIT_TESTING
    // --- Step 1: Open Secure Persistence ---
    Preferences prefs;
    prefs.begin("secrets", false);
    
    // --- Step 2: Validate or Generate Hardware Root Key ---
    if (prefs.getBytesLength("aes_key") != 32) {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);

        // Derive a 256-bit key from MAC address using SHA256
        mbedtls_md_context_t ctx;
        mbedtls_md_init(&ctx);
        mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
        mbedtls_md_starts(&ctx);

        // Combine MAC address and some random bytes as salt
        mbedtls_md_update(&ctx, mac, 6);
        uint32_t salt = esp_random();
        mbedtls_md_update(&ctx, (const unsigned char*)&salt, sizeof(salt));
        mbedtls_md_finish(&ctx, aesKey);
        mbedtls_md_free(&ctx);

        prefs.putBytes("aes_key", aesKey, 32);
        LOG_SECURE("CRYPTO", "Generated and firmly committed new MAC-derived 256-bit AES Hardware Key.");
    } else {
        prefs.getBytes("aes_key", aesKey, 32);
        LOG_SECURE("CRYPTO", "Mounted existing persistent AES Master Key.");
    }
    prefs.end();
#else
    // Mock implementation for native testing
    srand(time(NULL));
    for (int i = 0; i < 32; i++) {
        aesKey[i] = (uint8_t)(rand() % 256);
    }
    LOG_SECURE("CRYPTO", "[MOCK] Generated new 256-bit AES Hardware Key.");
#endif
    keyLoaded = true;
}

/**
 * @brief Padds a payload Buffer to adhere to the 16-byte CBC block size requirement.
 * @param buffer Raw pointer to the array holding the payload.
 * @param originalLen Native string length.
 * @param paddedLen Modulo 16 target padding length.
 */
void DeviceCrypto::applyPKCS7Padding(uint8_t* buffer, size_t originalLen, size_t paddedLen) {
    uint8_t paddingValue = paddedLen - originalLen;
    for (size_t i = originalLen; i < paddedLen; i++) {
        buffer[i] = paddingValue;
    }
}

/**
 * @brief Removes padding post-decryption.
 * @param buffer Raw pointer to the padded decrypt stream.
 * @param paddedLen Stream total length.
 * @return Decoded valid string span.
 */
size_t DeviceCrypto::removePKCS7Padding(const uint8_t* buffer, size_t paddedLen) {
    if (paddedLen == 0 || paddedLen % 16 != 0) return 0;
    uint8_t paddingValue = buffer[paddedLen - 1];
    
    if (paddingValue == 0 || paddingValue > 16) return paddedLen;
    
    for (size_t i = 0; i < paddingValue; i++) {
        if (buffer[paddedLen - 1 - i] != paddingValue) {
            return paddedLen;
        }
    }
    return paddedLen - paddingValue;
}

/**
 * @brief Encrypts plaintext into a base64 encoded string using AES-256-CBC.
 * @param plaintext The raw string you wish to secure.
 * @param inputLen Length of the plaintext.
 * @param outLen Output parameter to capture the ciphertext length.
 * @return Managed unique pointer to Base64-encoded string containing IV + Ciphertext.
 */
std::unique_ptr<char[]> DeviceCrypto::encrypt(const char* plaintext, size_t inputLen, size_t* outLen) {
    if (!keyLoaded || inputLen == 0) {
        ESP_LOGE(TAG, "encrypt() failed: %s", !keyLoaded ? "key not loaded" : "empty input");
        if (outLen) *outLen = 0;
        return std::unique_ptr<char[]>();
    }

    LOG_SECURE("CRYPTO", "Executing AES-256-CBC Encryption. Plaintext payload size: %zu bytes", inputLen);

#ifndef PIO_UNIT_TESTING
    size_t paddedLen = inputLen + (16 - (inputLen % 16));
    size_t totalLen = 16 + paddedLen;
    uint8_t* buffer = (uint8_t*)malloc(totalLen);
    if (!buffer) {
        ESP_LOGE(TAG, "encrypt() failed: memory allocation error (buffer size: %zu)", totalLen);
        if (outLen) *outLen = 0;
        return std::unique_ptr<char[]>();
    }
    
    for (int i = 0; i < 16; i++) {
        buffer[i] = (uint8_t)(esp_random() % 256);
    }
    
    memcpy(buffer + 16, plaintext, inputLen);
    applyPKCS7Padding(buffer + 16, inputLen, paddedLen);
    
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, aesKey, 256);
    
    uint8_t processingIv[16];
    memcpy(processingIv, buffer, 16);
    
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, paddedLen, processingIv, buffer + 16, buffer + 16);
    mbedtls_aes_free(&aes);

    size_t b64Len = 0;
    mbedtls_base64_encode(nullptr, 0, &b64Len, buffer, totalLen);
    
    std::unique_ptr<char[]> result;
    if (b64Len > 0) {
        char* b64Buffer = new (std::nothrow) char[b64Len + 1];
        if (b64Buffer) {
            if (mbedtls_base64_encode((unsigned char*)b64Buffer, b64Len, &b64Len, buffer, totalLen) == 0) {
                b64Buffer[b64Len] = '\0';
                result.reset(b64Buffer);
                if (outLen) *outLen = b64Len;
                LOG_SECURE("CRYPTO", "Encryption completed securely. Export Base64 size: %zu bytes", b64Len);
            } else {
                ESP_LOGE(TAG, "encrypt() failed: base64 encoding error");
                delete[] b64Buffer;
                if (outLen) *outLen = 0;
            }
        } else {
            ESP_LOGE(TAG, "encrypt() failed: base64 buffer allocation error (size: %zu)", b64Len + 1);
            if (outLen) *outLen = 0;
        }
    } else {
        ESP_LOGE(TAG, "encrypt() failed: base64 output length is zero");
        if (outLen) *outLen = 0;
    }
    
    free(buffer);
    return result;
#else
    // Mock implementation returning dummy Base64 string for native testing
    const char* mockResult = "TW9ja0VuY3J5cHRlZEJhc2U2NA==";
    size_t mockLen = strlen(mockResult);
    char* b64Buffer = new (std::nothrow) char[mockLen + 1];
    if (b64Buffer) {
        strcpy(b64Buffer, mockResult);
        if (outLen) *outLen = mockLen;
        return std::unique_ptr<char[]>(b64Buffer);
    }
    if (outLen) *outLen = 0;
    return std::unique_ptr<char[]>();
#endif
}

/**
 * @brief Decrypts a base64 encoded ciphertext string back into plaintext.
 * @param ciphertext The base64-encoded char buffer.
 * @param inputLen Length of the base64 ciphertext.
 * @param outLen Output parameter to capture the exact plaintext length.
 * @return Managed unique pointer to processed plaintext, or nullptr if decryption failed.
 */
std::unique_ptr<char[]> DeviceCrypto::decrypt(const char* ciphertext, size_t inputLen, size_t* outLen) {
    if (!keyLoaded || inputLen == 0) {
        ESP_LOGE(TAG, "decrypt() failed: %s", !keyLoaded ? "key not loaded" : "empty input");
        if (outLen) *outLen = 0;
        return std::unique_ptr<char[]>();
    }
    
    LOG_SECURE("CRYPTO", "Decoding inbound Base64 Ciphertext. Payload size: %zu bytes", inputLen);

#ifndef PIO_UNIT_TESTING
    size_t decOutLen = 0;
    mbedtls_base64_decode(nullptr, 0, &decOutLen, (const unsigned char*)ciphertext, inputLen);
    
    if (decOutLen <= 16) {
        ESP_LOGE(TAG, "decrypt() failed: decoded data too short (length: %zu, need >16)", decOutLen);
        if (outLen) *outLen = 0;
        return std::unique_ptr<char[]>();
    }

    uint8_t* buffer = (uint8_t*)malloc(decOutLen);
    if (!buffer) {
        ESP_LOGE(TAG, "decrypt() failed: memory allocation error (size: %zu)", decOutLen);
        if (outLen) *outLen = 0;
        return std::unique_ptr<char[]>();
    }
    
    if (mbedtls_base64_decode(buffer, decOutLen, &decOutLen, (const unsigned char*)ciphertext, inputLen) != 0) {
        ESP_LOGE(TAG, "decrypt() failed: base64 decode error");
        free(buffer);
        if (outLen) *outLen = 0;
        return std::unique_ptr<char[]>();
    }
    
    size_t ciphertextLen = decOutLen - 16;
    if (ciphertextLen % 16 != 0) {
        ESP_LOGE(TAG, "decrypt() failed: ciphertext length %zu is not block-aligned", ciphertextLen);
        free(buffer);
        if (outLen) *outLen = 0;
        return std::unique_ptr<char[]>();
    }

    uint8_t iv[16];
    memcpy(iv, buffer, 16);
    
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, aesKey, 256);
    
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, ciphertextLen, iv, buffer + 16, buffer + 16);
    mbedtls_aes_free(&aes);

    size_t unpaddedLen = removePKCS7Padding(buffer + 16, ciphertextLen);
    
    std::unique_ptr<char[]> result;
    if (unpaddedLen > 0 && unpaddedLen <= ciphertextLen) {
        char* plaintextBuf = new (std::nothrow) char[unpaddedLen + 1];
        if (plaintextBuf) {
            memcpy(plaintextBuf, buffer + 16, unpaddedLen);
            plaintextBuf[unpaddedLen] = '\0';
            result.reset(plaintextBuf);
            if (outLen) *outLen = unpaddedLen;
            LOG_SECURE("CRYPTO", "Decryption verified. Extracted plaintext configuration length: %zu bytes", unpaddedLen);
        } else {
            ESP_LOGE(TAG, "decrypt() failed: plaintext buffer allocation error");
            if (outLen) *outLen = 0;
        }
    } else {
        LOG_SECURE("CRYPTO", "Padding validation failed post-decryption. Key mismatch or stream corrupted!");
        if (outLen) *outLen = 0;
    }
    
    free(buffer);
    return result;
#else
    // Mock implementation returning dummy plaintext string for native testing
    const char* mockResult = "MockDecryptedString";
    size_t mockLen = strlen(mockResult);
    char* plaintextBuf = new (std::nothrow) char[mockLen + 1];
    if (plaintextBuf) {
        strcpy(plaintextBuf, mockResult);
        if (outLen) *outLen = mockLen;
        return std::unique_ptr<char[]>(plaintextBuf);
    }
    if (outLen) *outLen = 0;
    return std::unique_ptr<char[]>();
#endif
}
