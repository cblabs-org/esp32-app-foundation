/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/otaDownloadEngine/OtaDownloadEngine.hpp
 * Description: GitHub-aware OTA firmware download engine. Handles the complete
 *              download→verify→flash pipeline: redirect-aware HTTP fetching,
 *              .sig signature file retrieval, ESP32 binary header validation,
 *              partition space checks, chunked streaming writes through the
 *              Update library, and IOtaVerifier-based cryptographic signature
 *              verification.
 *
 *              This is a utility class in lib/ — not a module. It is consumed
 *              by otaUpdateManager (WO3) which orchestrates the full OTA
 *              lifecycle. The engine accepts a WiFiClient& from the caller
 *              and never creates its own network client.
 *
 * Exported Functions/Classes:
 * - OtaProgressCallback: Function type for download progress notifications
 * - OtaDownloadEngine: Download engine class for GitHub firmware OTA
 *   - setVerifier():         Inject an IOtaVerifier for cryptographic checks
 *   - setProgressCallback(): Register a progress notification callback
 *   - setHttpTimeout():      Configure HTTP client timeout (default 8000ms)
 *   - fetchSignature():      Download .sig file following GitHub redirects
 *   - downloadAndFlash():    Download, verify, and flash a firmware binary
 *   - getLastError():        Retrieve the last OtaError code
 *   - getLastErrorString():  Retrieve a human-readable error description
 */

#pragma once

#include "../../include/interfaces/IOtaVerifier.hpp"
#include "OtaError.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

#ifndef NATIVE_BUILD
    #include <Arduino.h>
    // WiFiClient is a typedef for NetworkClient in the ESP32 Arduino framework.
    // Forward-declare the real class and replicate the typedef to avoid pulling
    // the full WiFi library into the LDF dependency graph from this header.
    class NetworkClient;
    typedef NetworkClient WiFiClient;
    // Forward declaration only — heavy HTTPClient header included in .cpp
    class HTTPClient;
#else
    // --- Native build stubs ---
    #include <string>
    #ifndef STRING_STUB_DEFINED
    #define STRING_STUB_DEFINED
    class String {
    public:
        String() {}
        String(const char* s) : _str(s ? s : "") {}
        String(const std::string& s) : _str(s) {}
        String(int value, int base) {
            if (base == 16) {
                char buf[20];
                snprintf(buf, sizeof(buf), "%x", value);
                _str = buf;
            } else {
                _str = std::to_string(value);
            }
        }
        const char* c_str() const { return _str.c_str(); }
        bool isEmpty() const { return _str.empty(); }
        size_t length() const { return _str.length(); }
        char operator[](unsigned int i) const { return _str[i]; }
        String& operator+=(char c) { _str += c; return *this; }
        String& operator+=(const String& s) { _str += s._str; return *this; }
        friend String operator+(const char* lhs, const String& rhs) {
            return String(lhs) + rhs;
        }
        String operator+(const String& rhs) const {
            String result;
            result._str = _str + rhs._str;
            return result;
        }
        operator std::string() const { return _str; }
    private:
        std::string _str;
    };
    #endif

    // Minimal WiFiClient stub for native compilation
    class WiFiClient {
    public:
        virtual ~WiFiClient() = default;
    };

    // Minimal HTTPClient stub for native compilation
    class HTTPClient {};
#endif

/**
 * @brief Callback type for reporting OTA download progress.
 * @param bytesWritten Number of bytes written so far.
 * @param totalSize    Total firmware size in bytes.
 */
using OtaProgressCallback = std::function<void(size_t bytesWritten, size_t totalSize)>;

/**
 * @brief GitHub-aware OTA firmware download engine.
 *
 * Handles redirect-following HTTP fetches from GitHub release assets,
 * .sig signature file retrieval, ESP32 binary header verification (0xE9),
 * partition space validation, chunked streaming flash writes, and
 * IOtaVerifier-based cryptographic signature verification.
 *
 * The engine does NOT create its own WiFiClient — the caller provides
 * a configured client (secure or insecure). The caller is also responsible
 * for calling verifier_->begin(publicKey) before invoking downloadAndFlash().
 */
class OtaDownloadEngine {
public:
    OtaDownloadEngine();
    ~OtaDownloadEngine() = default;

    /**
     * @brief Set the IOtaVerifier to use during download. Ownership is NOT transferred.
     * @param verifier Pointer to an IOtaVerifier instance, or nullptr to disable verification.
     */
    void setVerifier(IOtaVerifier* verifier);

    /**
     * @brief Set the progress callback invoked after each chunk is written.
     * @param cb Callback receiving (bytesWritten, totalSize).
     */
    void setProgressCallback(OtaProgressCallback cb);

    /**
     * @brief Set the HTTP client timeout in milliseconds.
     * @param timeoutMs Timeout value (default: 8000).
     */
    void setHttpTimeout(int timeoutMs);

    /**
     * @brief Download the signature file from the given URL, following redirects.
     *
     * Performs a redirect-aware HTTP GET (up to kMaxRedirects hops) to fetch
     * the detached .sig file from a GitHub release asset URL.
     *
     * @param client     Active WiFiClient (secure or insecure), provided by caller.
     * @param sigUrl     URL of the .sig file (GitHub API asset URL).
     * @param token      Optional GitHub access token for private repos.
     * @param sigBuf     Output buffer for the raw signature bytes.
     * @param sigBufSize Size of the output buffer.
     * @return Number of bytes read, or 0 on failure.
     */
    size_t fetchSignature(WiFiClient& client, const String& sigUrl,
                          const String& token, uint8_t* sigBuf, size_t sigBufSize);

    /**
     * @brief Download, verify, and flash the firmware binary.
     *
     * Orchestrates the complete binary download pipeline:
     * redirect-following fetch → content-length validation → partition space
     * check → header verification (0xE9) → chunked streaming write with
     * per-chunk verifier accumulation → signature verification → Update.end().
     *
     * @param client  Active WiFiClient (secure or insecure), provided by caller.
     * @param binUrl  URL of the .bin file (GitHub API asset URL).
     * @param token   Optional GitHub access token for private repos.
     * @param sigBuf  Previously-fetched signature bytes (from fetchSignature).
     * @param sigLen  Length of the signature.
     * @return OtaError::None on success, specific error otherwise.
     */
    OtaError downloadAndFlash(WiFiClient& client, const String& binUrl,
                              const String& token, const uint8_t* sigBuf, size_t sigLen);

    /**
     * @brief Get the last error code.
     * @return The OtaError from the most recent operation.
     */
    OtaError getLastError() const;

    /**
     * @brief Get human-readable string for the last error.
     * @return Null-terminated C-string describing the error.
     */
    const char* getLastErrorString() const;

private:
    IOtaVerifier*        verifier_    = nullptr;  // Optional verifier (not owned)
    OtaProgressCallback  progressCb_;             // Progress notification callback
    int                  httpTimeout_ = 8000;     // HTTP client timeout in ms
    OtaError             lastError_   = OtaError::None; // Most recent error

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
     * @param token  Optional GitHub access token.
     * @return HTTP status code on success, or -1 on redirect exhaustion/failure.
     */
    int followRedirects(WiFiClient& client, HTTPClient& http,
                        const String& url, const String& token);
};
