/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/otaDownloadEngine/MbedTLSSignatureVerifier.hpp
 * Description: Default IOtaVerifier implementation backed by the mbedtls
 *              cryptographic library. Accumulates a SHA-256 digest over
 *              firmware chunks and validates the final hash against a
 *              detached RSA/ECDSA signature using a PEM-encoded public key.
 *
 *              On hardware builds (ESP-IDF), the full mbedtls implementation
 *              is used. On native builds (NATIVE_BUILD), stub implementations
 *              are provided that compile cleanly but return failure, allowing
 *              interface contract tests to run without crypto dependencies.
 *
 * Exported Classes:
 * - MbedTLSSignatureVerifier : public IOtaVerifier
 *   - begin():  Parse PEM public key and initialise SHA-256 digest context
 *   - update(): Accumulate chunk data into the running hash
 *   - verify(): Finalize digest and validate against the provided signature
 */

#pragma once

#include "../../include/interfaces/IOtaVerifier.hpp"

#ifndef NATIVE_BUILD
    #include <mbedtls/md.h>
    #include <mbedtls/pk.h>
#endif

/**
 * @brief IOtaVerifier implementation using mbedtls for SHA-256 hashing
 *        and public-key signature verification.
 *
 * Wraps mbedtls_pk and mbedtls_md contexts with RAII lifecycle management.
 * The public key is injected via begin() rather than being hardcoded,
 * enabling per-device or per-app key configuration through FrameworkConfig.
 */
class MbedTLSSignatureVerifier : public IOtaVerifier {
public:
    MbedTLSSignatureVerifier();
    ~MbedTLSSignatureVerifier() override;

    // Non-copyable, non-movable (owns mbedtls context handles)
    MbedTLSSignatureVerifier(const MbedTLSSignatureVerifier&) = delete;
    MbedTLSSignatureVerifier& operator=(const MbedTLSSignatureVerifier&) = delete;

    /**
     * @brief Initialize the verifier with a PEM-encoded public key.
     * @param publicKeyPem Null-terminated PEM string for the public key.
     * @return true if key parsing and digest setup both succeeded.
     */
    bool begin(const char* publicKeyPem) override;

    /**
     * @brief Feed firmware chunk data into the running SHA-256 hash.
     * @param data   Pointer to the chunk buffer.
     * @param length Number of bytes in this chunk.
     */
    void update(const uint8_t* data, size_t length) override;

    /**
     * @brief Finalize the SHA-256 hash and verify against the signature.
     * @param signature       Pointer to the raw signature bytes.
     * @param signatureLength Length of the signature buffer.
     * @return true if the signature is cryptographically valid.
     */
    bool verify(const uint8_t* signature, size_t signatureLength) override;

private:
#ifndef NATIVE_BUILD
    mbedtls_pk_context   pk_;     // Public key context
    mbedtls_md_context_t md_ctx_; // Message digest (SHA-256) context
#endif
    bool initialized_ = false;    // Guard: true only after successful begin()
};
