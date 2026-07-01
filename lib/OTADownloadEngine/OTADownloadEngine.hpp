/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/OTADownloadEngine/OTADownloadEngine.hpp
 * Description: OTA firmware download engine. Handles the complete
 *              download→verify→flash pipeline: redirect-aware HTTP fetching,
 *              .sig signature file retrieval, ESP32 binary header validation,
 *              partition space checks, chunked streaming writes through the
 *              Update library, and IOTAVerifier-based cryptographic signature
 *              verification.
 *
 *              This is a utility class in lib/ — not a module. It is consumed
 *              by OTAUpdateManager (WO3) which orchestrates the full OTA
 *              lifecycle. The engine accepts a WiFiClient& from the caller
 *              and never creates its own network client.
 *
 * Exported Functions/Classes:
 * - OTAProgressCallback: Function type for download progress notifications
 * - OTADownloadEngine: Download engine class for OTA firmware updates
 *   - setVerifier():         Inject an IOTAVerifier for cryptographic checks
 *   - setProgressCallback(): Register a progress notification callback
 *   - setHttpTimeout():      Configure HTTP client timeout (default 8000ms)
 *   - fetchSignature():      Download .sig file following HTTP redirects
 *   - downloadAndFlash():    Download, verify, and flash a firmware binary
 *   - getLastError():        Retrieve the last OTAError code
 *   - getLastErrorString():  Retrieve a human-readable error description
 */

#pragma once

#include "../../include/interfaces/IOTAVerifier.hpp"
#include "OTAError.hpp"

#include <cstddef>
#include <cstdint>
#include <string>


    #include <Arduino.h>
    // WiFiClient is a typedef for NetworkClient in the ESP32 Arduino framework.
    // Forward-declare the real class and replicate the typedef to avoid pulling
    // the full WiFi library into the LDF dependency graph from this header.
    #include <WiFiClient.h>
    typedef NetworkClient WiFiClient;
    // Forward declaration only — heavy HTTPClient header included in .cpp
    class HTTPClient;

/**
 * @brief Callback type for reporting OTA download progress.
 *
 * Static function pointer — replaces std::function to avoid hidden heap
 * allocations from lambda captures (GRAVE-05 compliance).
 *
 * @param context      Opaque pointer forwarded from setProgressCallback().
 * @param bytesWritten Number of bytes written so far.
 * @param totalSize    Total firmware size in bytes.
 */
using OTAProgressCallback = void(*)(void* context, size_t bytesWritten, size_t totalSize);

/**
 * @brief OTA firmware download engine.
 *
 * Handles redirect-following HTTP fetches from release asset URLs,
 * .sig signature file retrieval, ESP32 binary header verification (0xE9),
 * partition space validation, chunked streaming flash writes, and
 * IOTAVerifier-based cryptographic signature verification.
 *
 * The engine does NOT create its own WiFiClient — the caller provides
 * a configured client (secure or insecure). The caller is also responsible
 * for calling verifier_->begin(publicKey) before invoking downloadAndFlash().
 */
class OTADownloadEngine {
public:
    OTADownloadEngine();
    ~OTADownloadEngine() = default;

    /**
     * @brief Set the IOTAVerifier to use during download. Ownership is NOT transferred.
     * @param verifier Pointer to an IOTAVerifier instance, or nullptr to disable verification.
     */
    void setVerifier(IOTAVerifier* verifier);

    /**
     * @brief Set the progress callback invoked after each chunk is written.
     * @param cb      Static function pointer receiving (context, bytesWritten, totalSize).
     * @param context Opaque pointer forwarded to the callback as its first argument.
     */
    void setProgressCallback(OTAProgressCallback cb, void* context = nullptr);

    /**
     * @brief Set the HTTP client timeout in milliseconds.
     * @param timeoutMs Timeout value (default: 8000).
     */
    void setHttpTimeout(int timeoutMs);

    /**
     * @brief Set the expected firmware version for pre-flash validation.
     *
     * If set, downloadAndFlash() will read the esp_app_desc_t from the
     * first chunk of the binary and compare its version field against
     * this string. On mismatch, the download is aborted before any
     * flash writes occur.
     *
     * @param version  The expected BUILD_VERSION string (e.g. "v0.0.11-dev.13").
     *                 A leading 'v' or 'V' is automatically stripped.
     *                 Pass nullptr or empty string to disable the check.
     */
    void setExpectedVersion(const char* version);

    /**
     * @brief Download the signature file from the given URL, following redirects.
     *
     * Performs a redirect-aware HTTP GET (up to kMaxRedirects hops) to fetch
     * the detached .sig file from a release asset URL.
     *
     * @param client     Active WiFiClient (secure or insecure), provided by caller.
     * @param sigUrl     URL of the .sig file (HTTPS asset URL).
     * @param token      Optional access token for private release repos.
     * @param sigBuf     Output buffer for the raw signature bytes.
     * @param sigBufSize Size of the output buffer.
     * @return Number of bytes read, or 0 on failure.
     */
    size_t fetchSignature(WiFiClient& client, const std::string& sigUrl,
                          const std::string& token, uint8_t* sigBuf, size_t sigBufSize);

    /**
     * @brief Download, verify, and flash the firmware binary.
     *
     * Orchestrates the complete binary download pipeline:
     * redirect-following fetch → content-length validation → partition space
     * check → header verification (0xE9) → chunked streaming write with
     * per-chunk verifier accumulation → signature verification → Update.end().
     *
     * @param client  Active WiFiClient (secure or insecure), provided by caller.
     * @param binUrl  URL of the .bin file (HTTPS asset URL).
     * @param token   Optional access token for private release repos.
     * @param sigBuf  Previously-fetched signature bytes (from fetchSignature).
     * @param sigLen  Length of the signature.
     * @return OTAError::None on success, specific error otherwise.
     */
    OTAError downloadAndFlash(WiFiClient& client, const std::string& binUrl,
                              const std::string& token, const uint8_t* sigBuf, size_t sigLen);

    /**
     * @brief Get the last error code.
     * @return The OTAError from the most recent operation.
     */
    OTAError getLastError() const;

    /**
     * @brief Get human-readable string for the last error.
     * @return Null-terminated C-string describing the error.
     */
    const char* getLastErrorString() const;

private:
    IOTAVerifier*        verifier_    = nullptr;  // Optional verifier (not owned)
    OTAProgressCallback  progressCb_  = nullptr;  // Progress notification callback
    void*                progressCtx_ = nullptr;  // Opaque context for progressCb_
    int                  httpTimeout_ = 8000;     // HTTP client timeout in ms
    OTAError             lastError_   = OTAError::None; // Most recent error
    char                 expectedVersion_[32] = {};     // Expected version for pre-flash validation

    static constexpr int kMaxRedirects = 5;       // Maximum redirect hops before failure
    static constexpr size_t kChunkSize = 1024;    // Streaming read/write buffer size

    /**
     * @brief Follow HTTP redirects and return the final response code.
     *
     * Loops up to kMaxRedirects times, extracting the Location header on
     * 301/302/307/308 responses. On a non-redirect response, returns the
     * HTTP status code with the HTTPClient left in a connected state.
     *
     * @param client Active WiFiClient reference.
     * @param http   HTTPClient instance to use for requests.
     * @param url    Initial URL to fetch.
     * @param token  Optional access token.
     * @return HTTP status code on success, or -1 on redirect exhaustion/failure.
     */
    int followRedirects(WiFiClient& client, HTTPClient& http,
                        const std::string& url, const std::string& token);
};
