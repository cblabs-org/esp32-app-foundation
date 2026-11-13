/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/OTADownloadEngine/OTAError.hpp
 * Description: Strongly-typed error enumeration for the OTA download pipeline.
 *              Maps every discrete failure mode — HTTP transport, partition
 *              validation, binary verification, and flash write — to a named
 *              constant with a corresponding human-readable string via
 *              otaErrorToString(). Platform-agnostic: no hardware includes.
 *
 * Exported Functions/Classes:
 * - OTAError: Enum class covering all OTA download/flash failure modes
 * - otaErrorToString(): Maps an OTAError value to a human-readable C-string
 */

#pragma once

#include <cstdint>

/**
 * @brief Strongly-typed error codes for the OTA download and flash pipeline.
 *
 * Negative values indicate errors; zero means success. Each code maps
 * to a specific failure in the HTTP fetch, binary validation, cryptographic
 * verification, or flash write stages.
 */
enum class OTAError : int8_t {
    None                    =   0,  // Success — no error
    HttpConnectionFailed    =  -1,  // TCP/TLS connection could not be established
    HttpWrongResponseCode   =  -2,  // Server returned an unexpected HTTP status code
    HttpFileNotFound        =  -3,  // Server returned 404 Not Found
    HttpForbidden           =  -4,  // Server returned 403 Forbidden
    ServerNoContentLength   =  -5,  // Server did not report Content-Length
    TooManyRedirects        =  -6,  // Exceeded maximum redirect hops (5)
    InsufficientSpace       =  -7,  // Firmware binary exceeds free sketch space
    NoOtaPartition          =  -8,  // No OTA partition available on the device
    BinaryHeaderInvalid     =  -9,  // First byte is not ESP32 magic 0xE9
    SignatureFetchFailed    = -10,  // Failed to download the .sig signature file
    SignatureVerifyFailed   = -11,  // Cryptographic signature verification failed
    UpdateBeginFailed       = -12,  // Update.begin() returned false
    UpdateWriteFailed       = -13,  // Update.write() wrote fewer bytes than expected
    UpdateEndFailed         = -14,  // Update.end() returned false
    StreamReadTimeout       = -15,  // Stream read timed out during chunked download
    BinaryVersionMismatch   = -16,  // Binary's embedded version doesn't match expected
};

/**
 * @brief Convert an OTAError value to a human-readable C-string.
 * @param err The error code to translate.
 * @return Null-terminated string describing the error. Never returns nullptr.
 */
const char* otaErrorToString(OTAError err);
