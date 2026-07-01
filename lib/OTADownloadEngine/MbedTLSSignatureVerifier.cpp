/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/OTADownloadEngine/MbedTLSSignatureVerifier.cpp
 * Description: Implementation of the MbedTLSSignatureVerifier class.
 *              On hardware (ESP-IDF): full mbedtls SHA-256 hashing and
 *              public-key signature verification.
 *              On native (NATIVE_BUILD): stub implementations that compile
 *              cleanly but return false, enabling interface contract tests
 *              without crypto dependencies.
 *
 * Exported Functions/Classes:
 * - MbedTLSSignatureVerifier (implementation)
 *   - MbedTLSSignatureVerifier(): Initialise mbedtls contexts
 *   - ~MbedTLSSignatureVerifier(): Free mbedtls contexts
 *   - begin():  Parse public key and set up SHA-256 digest
 *   - update(): Feed chunk data into running hash
 *   - verify(): Finalize hash and validate signature
 */

#include "MbedTLSSignatureVerifier.hpp"
#include "platform/LogShim.h"

#include <cstring>

static constexpr const char* TAG = "MbedTLSSignatureVerifier";

// =============================================================================
// Hardware build — full mbedtls implementation
// =============================================================================

MbedTLSSignatureVerifier::MbedTLSSignatureVerifier() {
    mbedtls_pk_init(&pk_);
    mbedtls_md_init(&md_ctx_);
}

MbedTLSSignatureVerifier::~MbedTLSSignatureVerifier() {
    mbedtls_pk_free(&pk_);
    mbedtls_md_free(&md_ctx_);
}

/**
 * @brief Parse the PEM public key and initialise the SHA-256 digest context.
 *
 * The key is accepted as a parameter for runtime injection via
 * FrameworkConfig, rather than using a hardcoded global.
 */
bool MbedTLSSignatureVerifier::begin(const char* publicKeyPem) {
    // --- Guard: reject null or empty key material ---
    if (publicKeyPem == nullptr || publicKeyPem[0] == '\0') {
        ESP_LOGE(TAG, "begin() called with null or empty public key");
        return false;
    }

    // --- Step 1: Parse the PEM-encoded public key ---
    int ret = mbedtls_pk_parse_public_key(
        &pk_,
        reinterpret_cast<const unsigned char*>(publicKeyPem),
        strlen(publicKeyPem) + 1  // mbedtls requires the null terminator
    );
    if (ret != 0) {
        ESP_LOGE(TAG, "mbedtls_pk_parse_public_key failed: -0x%04X", -ret);
        return false;
    }

    // --- Step 2: Set up the SHA-256 message digest context ---
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == nullptr) {
        ESP_LOGE(TAG, "SHA-256 md_info not available");
        return false;
    }

    if (mbedtls_md_setup(&md_ctx_, md_info, 0) != 0) {
        ESP_LOGE(TAG, "mbedtls_md_setup failed");
        return false;
    }

    if (mbedtls_md_starts(&md_ctx_) != 0) {
        ESP_LOGE(TAG, "mbedtls_md_starts failed");
        return false;
    }

    initialized_ = true;
    return true;
}

void MbedTLSSignatureVerifier::update(const uint8_t* data, size_t length) {
    if (initialized_) {
        mbedtls_md_update(&md_ctx_, data, length);
    }
}

bool MbedTLSSignatureVerifier::verify(const uint8_t* signature, size_t signatureLength) {
    if (!initialized_) {
        return false;
    }

    // --- Finalize the SHA-256 digest ---
    unsigned char hash[32]; // SHA-256 produces 32 bytes
    if (mbedtls_md_finish(&md_ctx_, hash) != 0) {
        ESP_LOGE(TAG, "mbedtls_md_finish failed");
        return false;
    }

    // --- Verify the signature against the accumulated hash ---
    int ret = mbedtls_pk_verify(&pk_, MBEDTLS_MD_SHA256, hash, sizeof(hash),
                                signature, signatureLength);
    if (ret != 0) {
        ESP_LOGE(TAG, "mbedtls_pk_verify failed: -0x%04X", -ret);
        return false;
    }

    return true;
}

// =============================================================================
// Native build — stub implementations for interface contract testing
// =============================================================================
