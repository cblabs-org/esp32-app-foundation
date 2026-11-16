/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/interfaces/IOTAVerifier.hpp
 * Description: Pure abstract interface for OTA firmware cryptographic
 *              verification. Implementors accumulate a hash over downloaded
 *              firmware chunks and verify the final digest against a
 *              detached signature. The public key is injected at runtime
 *              via begin(), enabling app-side key configuration through
 *              FrameworkConfig.
 *
 * Exported Classes:
 * - IOTAVerifier: Abstract base defining the OTA verification contract
 *   - begin():  Initialize the verifier with PEM-encoded public key material
 *   - update(): Feed a firmware chunk for incremental hash accumulation
 *   - verify(): Finalize hash and validate against a detached signature
 */

#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief Contract for OTA firmware cryptographic signature verification.
 *
 * Implementors accumulate a SHA-256 hash over firmware data via repeated
 * update() calls, then verify the accumulated digest against a detached
 * signature in verify(). The public key is provided at runtime through
 * begin(), decoupling the verifier from any hardcoded key material.
 *
 * Typical usage sequence:
 *   1. begin(publicKeyPem)   — load public key, initialise digest context
 *   2. update(chunk, len)    — called once per downloaded chunk
 *   3. verify(sig, sigLen)   — finalize and validate
 */
class IOTAVerifier {
public:
    /** @brief Destructor for IOTAVerifier. */
    virtual ~IOTAVerifier() = default;

    /**
     * @brief Initialize the verifier with the public key material.
     * @param publicKeyPem PEM-encoded public key string (null-terminated).
     * @return true if initialization succeeded.
     */
    virtual bool begin(const char* publicKeyPem) = 0;

    /**
     * @brief Feed a chunk of firmware data for hash accumulation.
     *
     * Called repeatedly during download, once per chunk. Must be a no-op
     * if begin() has not been called or returned false.
     *
     * @param data   Pointer to the chunk buffer.
     * @param length Number of bytes in the chunk.
     */
    virtual void update(const uint8_t* data, size_t length) = 0;

    /**
     * @brief Finalize the hash and verify against the provided signature.
     * @param signature       Pointer to the raw signature bytes.
     * @param signatureLength Length of the signature buffer.
     * @return true if the signature is valid for the accumulated hash.
     */
    virtual bool verify(const uint8_t* signature, size_t signatureLength) = 0;
};
