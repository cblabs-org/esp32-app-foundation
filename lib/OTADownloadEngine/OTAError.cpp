/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/OTADownloadEngine/OTAError.cpp
 * Description: Implementation of otaErrorToString() — maps each OTAError enum
 *              value to a descriptive, human-readable C-string suitable for
 *              log output and diagnostic reporting.
 *
 * Exported Functions/Classes:
 * - otaErrorToString(): Switch-based lookup returning a static string per error code
 */

#include "OTAError.hpp"

/**
 * @brief Convert an OTAError value to a human-readable C-string.
 *
 * Uses a switch statement to map every enum value to a descriptive message.
 * Style matches the departures-board's getLastErrorString() output format.
 *
 * @param err The error code to translate.
 * @return Null-terminated string describing the error. Never returns nullptr.
 */
const char* otaErrorToString(OTAError err) {
    switch (err) {
        case OTAError::None:                    return "No error (success)";
        case OTAError::HttpConnectionFailed:    return "HTTP connection failed";
        case OTAError::HttpWrongResponseCode:   return "Wrong HTTP response code";
        case OTAError::HttpFileNotFound:        return "File Not Found (404)";
        case OTAError::HttpForbidden:           return "Forbidden (403)";
        case OTAError::ServerNoContentLength:   return "Server did not report size";
        case OTAError::TooManyRedirects:        return "Too many redirects (exceeded 5 hops)";
        case OTAError::InsufficientSpace:       return "Not enough space for firmware";
        case OTAError::NoOtaPartition:          return "No OTA partition found";
        case OTAError::BinaryHeaderInvalid:     return "Binary header verification failed (not 0xE9)";
        case OTAError::SignatureFetchFailed:    return "Signature file download failed";
        case OTAError::SignatureVerifyFailed:   return "Cryptographic signature verification failed";
        case OTAError::UpdateBeginFailed:       return "Update.begin() failed";
        case OTAError::UpdateWriteFailed:       return "Update.write() failed";
        case OTAError::UpdateEndFailed:         return "Update.end() failed";
        case OTAError::StreamReadTimeout:       return "Stream read timeout or disconnect";
    }
    return "Unknown OTA error";
}
