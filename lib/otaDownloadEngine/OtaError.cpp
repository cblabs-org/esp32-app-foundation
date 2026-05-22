/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/otaDownloadEngine/OtaError.cpp
 * Description: Implementation of otaErrorToString() — maps each OtaError enum
 *              value to a descriptive, human-readable C-string suitable for
 *              log output and diagnostic reporting.
 *
 * Exported Functions/Classes:
 * - otaErrorToString(): Switch-based lookup returning a static string per error code
 */

#include "OtaError.hpp"

/**
 * @brief Convert an OtaError value to a human-readable C-string.
 *
 * Uses a switch statement to map every enum value to a descriptive message.
 * Style matches the departures-board's getLastErrorString() output format.
 *
 * @param err The error code to translate.
 * @return Null-terminated string describing the error. Never returns nullptr.
 */
const char* otaErrorToString(OtaError err) {
    switch (err) {
        case OtaError::None:                    return "No error (success)";
        case OtaError::HttpConnectionFailed:    return "HTTP connection failed";
        case OtaError::HttpWrongResponseCode:   return "Wrong HTTP response code";
        case OtaError::HttpFileNotFound:        return "File Not Found (404)";
        case OtaError::HttpForbidden:           return "Forbidden (403)";
        case OtaError::ServerNoContentLength:   return "Server did not report size";
        case OtaError::TooManyRedirects:        return "Too many redirects (exceeded 5 hops)";
        case OtaError::InsufficientSpace:       return "Not enough space for firmware";
        case OtaError::NoOtaPartition:          return "No OTA partition found";
        case OtaError::BinaryHeaderInvalid:     return "Binary header verification failed (not 0xE9)";
        case OtaError::SignatureFetchFailed:    return "Signature file download failed";
        case OtaError::SignatureVerifyFailed:   return "Cryptographic signature verification failed";
        case OtaError::UpdateBeginFailed:       return "Update.begin() failed";
        case OtaError::UpdateWriteFailed:       return "Update.write() failed";
        case OtaError::UpdateEndFailed:         return "Update.end() failed";
        case OtaError::StreamReadTimeout:       return "Stream read timeout or disconnect";
    }
    return "Unknown OTA error";
}
