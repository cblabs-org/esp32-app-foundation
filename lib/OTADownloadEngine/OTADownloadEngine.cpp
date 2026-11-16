/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/OTADownloadEngine/OTADownloadEngine.cpp
 * Description: Implementation of the OTA Download Engine — redirect-aware
 *              OTA binary downloader with signature fetch, ESP32 header
 *              verification, partition validation, chunked streaming flash
 *              writes, and IOTAVerifier cryptographic verification.
 *
 *              On hardware: full implementation using WiFiClient, HTTPClient,
 *              and the ESP32 Update library.
 *              On native: stub implementations that return safe failure values,
 *              enabling unit tests for the interface contract without hardware.
 *
 * Exported Functions/Classes:
 * - OTADownloadEngine: [Class implementation]
 *   - setVerifier():         Store verifier pointer
 *   - setProgressCallback(): Store progress callback
 *   - setHttpTimeout():      Store timeout value
 *   - fetchSignature():      Redirect-aware .sig download (hardware) / stub (native)
 *   - downloadAndFlash():    Full download→verify→flash pipeline (hardware) / stub (native)
 *   - getLastError():        Return last OTAError
 *   - getLastErrorString():  Return human-readable error string
 *   - followRedirects():     Internal redirect follower (hardware) / stub (native)
 */

#include "OTADownloadEngine.hpp"
#include "platform/LogShim.h"
#include "DeepStackExecutor.h"

static constexpr const char* TAG = "OTADownloadEngine";

// =============================================================================
// Common implementations (both hardware and native)
// =============================================================================

/**
 * @brief Default constructor — initializes all members to safe defaults.
 */
OTADownloadEngine::OTADownloadEngine()
    : _verifier(nullptr)
    , _progressCb(nullptr)
    , _progressCtx(nullptr)
    , _httpTimeout(8000)
    , _lastError(OTAError::None)
{
}

/**
 * @brief Set the IOTAVerifier to use during download. Ownership is NOT transferred.
 * @param verifier Pointer to an IOTAVerifier instance, or nullptr to disable verification.
 */
void OTADownloadEngine::setVerifier(IOTAVerifier* verifier) {
    _verifier = verifier;
}

/**
 * @brief Set the progress callback invoked after each chunk is written.
 * @param cb Callback receiving (bytesWritten, totalSize).
 */
void OTADownloadEngine::setProgressCallback(OTAProgressCallback cb, void* context) {
    _progressCb = cb;
    _progressCtx = context;
}

/**
 * @brief Set the HTTP client timeout in milliseconds.
 * @param timeoutMs Timeout value (default: 8000).
 */
void OTADownloadEngine::setHttpTimeout(int timeoutMs) {
    _httpTimeout = timeoutMs;
}

/**
 * @brief Set the device identity for HTTP User-Agent header in download requests.
 *
 * Formats: "<sanitised-name>/<version> (ESP32-http-Update)"
 * If name or version is nullptr or empty, defaults to "ESP32-http-Update".
 *
 * @param name    Sanitised device name (e.g. "weather-station").
 * @param version Firmware version string (e.g. "0.1.0-dev.5" or BUILD_VERSION).
 */
void OTADownloadEngine::setDeviceIdentity(const char* name, const char* version) {
    if (name && name[0] != '\0' && version && version[0] != '\0') {
        snprintf(_userAgent, sizeof(_userAgent), "%s/%s (ESP32-http-Update)", name, version);
    } else {
        strlcpy(_userAgent, "ESP32-http-Update", sizeof(_userAgent));
    }
}

/**
 * @brief Get the last error code.
 * @return The OTAError from the most recent operation.
 */
OTAError OTADownloadEngine::getLastError() const {
    return _lastError;
}

/**
 * @brief Get human-readable string for the last error.
 * @return Null-terminated C-string describing the error.
 */
const char* OTADownloadEngine::getLastErrorString() const {
    return otaErrorToString(_lastError);
}

/**
 * @brief Set the expected firmware version for pre-flash validation.
 *
 * Stores the version string (stripping any leading 'v'/'V' prefix) into
 * the fixed-size _expectedVersion buffer. When set, downloadAndFlash()
 * will validate the binary's esp_app_desc_t.version field against this
 * string before any flash writes occur.
 *
 * @param version  Expected version string, or nullptr/empty to disable.
 */
void OTADownloadEngine::setExpectedVersion(const char* version) {
    if (version && version[0] != '\0') {
        // Strip leading 'v' or 'V' — BUILD_VERSION omits it
        const char* v = version;
        if (v[0] == 'v' || v[0] == 'V') {
            v++;
        }
        strncpy(_expectedVersion, v, sizeof(_expectedVersion) - 1);
        _expectedVersion[sizeof(_expectedVersion) - 1] = '\0';
    } else {
        _expectedVersion[0] = '\0';
    }
}


// =============================================================================
// Hardware implementation — full OTA download engine
// =============================================================================

#include <WiFiClient.h>
#include <esp_app_format.h>
#include <esp_app_desc.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Esp.h>
#include "esp_task_wdt.h"

/**
 * @brief Follow HTTP redirects and return the final response code.
 *
 * Loops up to kMaxRedirects times. Each iteration opens a new HTTP request
 * with download headers (User-Agent, Cache-Control, Accept,
 * Authorization) and disables auto-redirect to
 * manually follow 301/302/307/308 responses via the Location header.
 *
 * Uses HTTP/1.0 to avoid chunked transfer encoding complications on
 * constrained devices.
 *
 * @param client Active WiFiClient reference.
 * @param http   HTTPClient instance to use for requests.
 * @param url    Initial URL to fetch.
 * @param token  Optional GitHub access token.
 * @return HTTP status code on success, or -1 on redirect exhaustion/failure.
 */
int OTADownloadEngine::followRedirects(WiFiClient& client, HTTPClient& http,
                                        const std::string& url, const std::string& token) {
    std::string currentUrl = url;
    String initialHost = extractHost(url);

    for (int hop = 0; hop < kMaxRedirects; hop++) {
        // --- Begin HTTP request for this hop ---
        if (!http.begin(client, String(currentUrl.c_str()))) {
            ESP_LOGE(TAG, "http.begin() failed for URL: %s", currentUrl.c_str());
            return -1;
        }

        // Configure headers for GitHub release asset API
        http.useHTTP10(true);
        http.setTimeout(_httpTimeout);
        http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
        http.setUserAgent(_userAgent);
        http.addHeader("Cache-Control", "no-cache");
        http.addHeader("Accept", "application/octet-stream");

        String currentHost = extractHost(currentUrl);
        if (!token.empty() && currentHost.length() > 0 && currentHost == initialHost) {
            http.addHeader("Authorization", String(("Bearer " + token).c_str()));
        }

        int code = http.GET();

        // --- Check for redirect responses ---
        if (code == HTTP_CODE_MOVED_PERMANENTLY  || code == HTTP_CODE_FOUND ||
            code == HTTP_CODE_TEMPORARY_REDIRECT || code == HTTP_CODE_PERMANENT_REDIRECT) {
            String newUrl = http.getLocation();
            http.end();

            if (newUrl.length() == 0) {
                ESP_LOGE(TAG, "Redirect response but empty Location header at hop %d", hop + 1);
                return -1;
            }

            ESP_LOGD(TAG, "Redirect hop %d/%d → %s", hop + 1, kMaxRedirects, newUrl.c_str());
            currentUrl = newUrl.c_str();
            continue;
        }

        // --- Non-redirect response: return the code with connection open ---
        return code;
    }

    // Exhausted all redirect hops
    ESP_LOGE(TAG, "Exceeded maximum redirect hops (%d)", kMaxRedirects);
    return -1;
}

/**
 * @brief Download the signature file from the given URL, following redirects.
 *
 * Uses followRedirects() to navigate GitHub's asset CDN redirects, then reads
 * the response body into the caller's buffer with a timeout-guarded loop.
 *
 * @param client     Active WiFiClient (secure or insecure), provided by caller.
 * @param sigUrl     URL of the .sig file (GitHub API asset URL).
 * @param token      Optional GitHub access token for private repos.
 * @param sigBuf     Output buffer for the raw signature bytes.
 * @param sigBufSize Size of the output buffer.
 * @return Number of bytes read, or 0 on failure.
 */
size_t OTADownloadEngine::fetchSignature(WiFiClient& client, const std::string& sigUrl,
                                          const std::string& token, uint8_t* sigBuf, size_t sigBufSize) {
    ESP_LOGI(TAG, "Fetching signature from: %s", sigUrl.c_str());
    HTTPClient http;

    int code = followRedirects(client, http, sigUrl, token);
    int contentLen = http.getSize();

    // --- Validate HTTP response ---
    if (code != HTTP_CODE_OK || contentLen <= 0 || static_cast<size_t>(contentLen) > sigBufSize) {
        ESP_LOGE(TAG, "Signature fetch failed — HTTP %d, Content-Length: %d", code, contentLen);
        http.end();
        _lastError = OTAError::SignatureFetchFailed;
        return 0;
    }

    // --- Read signature bytes with timeout guard ---
    WiFiClient* tcp = http.getStreamPtr();
    size_t bytesRead = 0;
    unsigned long timeout = millis() + _httpTimeout;

    while (bytesRead < static_cast<size_t>(contentLen) && millis() < timeout) {
        if (tcp->available()) {
            int r = tcp->readBytes(sigBuf + bytesRead, contentLen - bytesRead);
            bytesRead += r;
            timeout = millis() + _httpTimeout; // Reset timeout on progress
        } else {
            delay(1); // Yield to RTOS scheduler
        }
    }

    http.end();

    if (bytesRead != static_cast<size_t>(contentLen)) {
        ESP_LOGE(TAG, "Signature read incomplete — got %u of %d bytes", (unsigned)bytesRead, contentLen);
        _lastError = OTAError::SignatureFetchFailed;
        return 0;
    }

    ESP_LOGI(TAG, "Signature fetched successfully (%u bytes)", (unsigned)bytesRead);
    return bytesRead;
}

/**
 * @brief Download, verify, and flash the firmware binary.
 *
 * Orchestrates the complete download pipeline:
 * 1. Follow redirects to reach the binary asset
 * 2. Map HTTP error codes to OTAError values
 * 3. Validate content-length and partition space
 * 4. Verify ESP32 binary header magic byte (0xE9)
 * 5. Stream chunks through Update.write() with per-chunk verifier accumulation
 * 6. Verify cryptographic signature post-download
 * 7. Finalize the update with Update.end()
 *
 * @param client  Active WiFiClient (secure or insecure), provided by caller.
 * @param binUrl  URL of the .bin file (GitHub API asset URL).
 * @param token   Optional GitHub access token for private repos.
 * @param sigBuf  Previously-fetched signature bytes (from fetchSignature).
 * @param sigLen  Length of the signature.
 * @return OTAError::None on success, specific error otherwise.
 */
OTAError OTADownloadEngine::downloadAndFlash(WiFiClient& client, const std::string& binUrl,
                                              const std::string& token, const uint8_t* sigBuf, size_t sigLen) {
    ESP_LOGI(TAG, "Beginning firmware download from: %s", binUrl.c_str());
    HTTPClient http;

    // --- Step 1: Follow redirects to reach the binary ---
    int code = followRedirects(client, http, binUrl, token);

    // --- Step 2: Map HTTP error codes ---
    if (code <= 0) {
        ESP_LOGE(TAG, "HTTP connection failed (code: %d)", code);
        http.end();
        _lastError = OTAError::HttpConnectionFailed;
        return _lastError;
    }

    if (code == HTTP_CODE_NOT_FOUND) {
        ESP_LOGE(TAG, "Binary not found — server returned 404");
        http.end();
        _lastError = OTAError::HttpFileNotFound;
        return _lastError;
    }

    if (code == HTTP_CODE_FORBIDDEN) {
        ESP_LOGE(TAG, "Access forbidden — server returned 403");
        http.end();
        _lastError = OTAError::HttpForbidden;
        return _lastError;
    }

    if (code != HTTP_CODE_OK) {
        ESP_LOGE(TAG, "Unexpected HTTP response code: %d", code);
        http.end();
        _lastError = OTAError::HttpWrongResponseCode;
        return _lastError;
    }

    // --- Step 3: Validate content-length ---
    int contentLen = http.getSize();
    if (contentLen <= 0) {
        ESP_LOGE(TAG, "Server did not report Content-Length");
        http.end();
        _lastError = OTAError::ServerNoContentLength;
        return _lastError;
    }

    ESP_LOGI(TAG, "Binary size reported: %d bytes", contentLen);

    // --- Step 4: Check partition space ---
    int freeSpace = ESP.getFreeSketchSpace();
    if (freeSpace == 0) {
        ESP_LOGE(TAG, "No OTA partition available");
        http.end();
        _lastError = OTAError::NoOtaPartition;
        return _lastError;
    }

    if (contentLen > freeSpace) {
        ESP_LOGE(TAG, "Insufficient space — need %d bytes, have %d", contentLen, freeSpace);
        http.end();
        _lastError = OTAError::InsufficientSpace;
        return _lastError;
    }

    ESP_LOGI(TAG, "Partition space OK: %d free, %d needed", freeSpace, contentLen);

    // --- Step 5: Read first chunk and validate binary header ---
    WiFiClient* tcp = http.getStreamPtr();
    tcp->setTimeout(15000); // 15s — generous for flash write stalls and TLS record gaps
    delay(200); // Brief delay to ensure stream buffer is populated

    // Static buffer — keeps 1 KB off the task stack. Safe because only one
    // OTA download runs at a time. Located in BSS (DRAM), not PSRAM, so it
    // remains accessible during SPI flash write operations.
    static uint8_t buf[kChunkSize];

    // Read the first chunk — contains the image header + app descriptor
    int firstChunkLen = 0;
    unsigned long readDeadline = millis() + 15000;
    while (firstChunkLen == 0 && millis() < readDeadline) {
        if (tcp->available()) {
            size_t bytesToRead = (static_cast<size_t>(contentLen) < kChunkSize)
                ? static_cast<size_t>(contentLen) : kChunkSize;
            firstChunkLen = tcp->readBytes(buf, bytesToRead);
        } else {
            delay(10);
        }
    }

    if (firstChunkLen <= 0) {
        ESP_LOGE(TAG, "Failed to read first chunk — stream timeout");
        http.end();
        _lastError = OTAError::StreamReadTimeout;
        return _lastError;
    }

    // --- Step 5a: Validate ESP32 binary magic byte ---
    if (buf[0] != 0xE9) {
        ESP_LOGE(TAG, "Binary header invalid — expected 0xE9, got 0x%02X", buf[0]);
        http.end();
        _lastError = OTAError::BinaryHeaderInvalid;
        return _lastError;
    }

    // --- Step 5b: Pre-flash binary version validation (safety gate) ---
    // The esp_app_desc_t sits at byte offset 32 (after esp_image_header_t +
    // esp_image_segment_header_t) and is 256 bytes. Total: 288 bytes needed.
    // kChunkSize is 1024, so this is always available in the first chunk.
    static constexpr size_t kAppDescOffset = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t);

    if (_expectedVersion[0] != '\0' &&
        static_cast<size_t>(firstChunkLen) >= kAppDescOffset + sizeof(esp_app_desc_t)) {

        const auto* appDesc = reinterpret_cast<const esp_app_desc_t*>(buf + kAppDescOffset);

        if (appDesc->magic_word != ESP_APP_DESC_MAGIC_WORD) {
            ESP_LOGW(TAG, "App descriptor magic word invalid (0x%08X) — skipping version check",
                     (unsigned)appDesc->magic_word);
        } else {
            // Null-terminate safely for comparison (version field is char[32])
            char binaryVersion[32];
            memcpy(binaryVersion, appDesc->version, sizeof(binaryVersion));
            binaryVersion[sizeof(binaryVersion) - 1] = '\0';

            // Strip leading 'v'/'V' — ESP-IDF's PROJECT_VER (from git tags)
            // often includes it, but _expectedVersion already has it stripped.
            const char* cmpVersion = binaryVersion;
            if (cmpVersion[0] == 'v' || cmpVersion[0] == 'V') {
                cmpVersion++;
            }

            if (strncmp(cmpVersion, _expectedVersion, sizeof(binaryVersion)) != 0) {
                ESP_LOGE(TAG, "SAFETY ABORT: Binary version mismatch — binary contains '%s', expected '%s'",
                         cmpVersion, _expectedVersion);
                http.end();
                _lastError = OTAError::BinaryVersionMismatch;
                return _lastError;
            }
            ESP_LOGI(TAG, "Binary version validated: '%s' matches expected", cmpVersion);
        }
    } else if (_expectedVersion[0] != '\0') {
        ESP_LOGW(TAG, "First chunk too small for app descriptor (%d bytes) — skipping version check",
                 firstChunkLen);
    }

    // --- Step 6: Begin Update ---
    // Ensure no screenshots or other deep stack jobs are mid-flight.
    DeepStackExecutor::awaitAllIdle();

    if (!Update.begin(contentLen)) {
        ESP_LOGE(TAG, "Update.begin() failed for size %d", contentLen);
        http.end();
        _lastError = OTAError::UpdateBeginFailed;
        return _lastError;
    }

    // --- Step 7: Warn if no verifier set ---
    if (_verifier == nullptr) {
        ESP_LOGW(TAG, "No IOTAVerifier set — proceeding WITHOUT cryptographic verification");
    }

    // --- Step 8: Report initial progress ---
    if (_progressCb) {
        _progressCb(_progressCtx, 0, static_cast<size_t>(contentLen));
    }

    ESP_LOGI(TAG, "Streaming firmware (%d bytes) in %u-byte chunks...", contentLen, (unsigned)kChunkSize);

    // --- Step 8b: Write the already-buffered first chunk ---
    if (_verifier != nullptr) {
        _verifier->update(buf, firstChunkLen);
    }
    if (Update.write(buf, firstChunkLen) != static_cast<size_t>(firstChunkLen)) {
        ESP_LOGE(TAG, "Update.write() failed on first chunk");
        Update.abort();
        http.end();
        _lastError = OTAError::UpdateWriteFailed;
        return _lastError;
    }

    size_t totalWritten = static_cast<size_t>(firstChunkLen);

    if (_progressCb) {
        _progressCb(_progressCtx, totalWritten, static_cast<size_t>(contentLen));
    }

    // --- Step 9: Chunked streaming write for remaining data ---
    while (totalWritten < static_cast<size_t>(contentLen)) {
        size_t remaining = static_cast<size_t>(contentLen) - totalWritten;
        size_t bytesToRead = (remaining < kChunkSize) ? remaining : kChunkSize;

        // Wait for data with retry — TLS record boundaries can cause
        // brief gaps where no decrypted data is practical
        int bytesRead = 0;
        readDeadline = millis() + 15000; // 15s hard deadline

        while (bytesRead == 0 && millis() < readDeadline) {
            if (tcp->available()) {
                bytesRead = tcp->readBytes(buf, bytesToRead);
            } else {
                delay(10); // yield to WiFi/TCP stack
            }
        }

        if (bytesRead <= 0) {
            ESP_LOGE(TAG, "Stream read timeout at %u/%d bytes (%.1f%%)",
                     (unsigned)totalWritten, contentLen,
                     (totalWritten * 100.0f) / contentLen);
            Update.abort();
            http.end();
            _lastError = OTAError::StreamReadTimeout;
            return _lastError;
        }

        // Feed chunk to cryptographic verifier if present
        if (_verifier != nullptr) {
            _verifier->update(buf, bytesRead);
        }

        // Write chunk to OTA partition
        if (Update.write(buf, bytesRead) != static_cast<size_t>(bytesRead)) {
            ESP_LOGE(TAG, "Update.write() failed at offset %u", (unsigned)totalWritten);
            Update.abort();
            http.end();
            _lastError = OTAError::UpdateWriteFailed;
            return _lastError;
        }

        totalWritten += bytesRead;

        // Yield to network stack between chunks
        delay(1);

        // Report progress
        if (_progressCb) {
            _progressCb(_progressCtx, totalWritten, static_cast<size_t>(contentLen));
        }

        // Debug milestone logging at 25% intervals
        size_t quarter = static_cast<size_t>(contentLen) / 4;
        if (quarter > 0 && totalWritten % quarter < kChunkSize) {
            ESP_LOGD(TAG, "Progress: %u/%d bytes (%d%%)",
                     (unsigned)totalWritten, contentLen,
                     static_cast<int>((totalWritten * 100) / contentLen));
        }
    }

    ESP_LOGI(TAG, "Download complete — %u bytes written to flash", (unsigned)totalWritten);

    // --- Step 10: Cryptographic signature verification ---
    if (_verifier != nullptr) {
        // Fail closed: verifier is set, signature MUST be present
        if (sigBuf == nullptr || sigLen == 0) {
            ESP_LOGE(TAG, "SECURITY ABORT: Verifier configured but no signature provided — aborting");
            Update.abort();
            http.end();
            _lastError = OTAError::SignatureFetchFailed;
            return _lastError;
        }
        if (!_verifier->verify(sigBuf, sigLen)) {
            ESP_LOGE(TAG, "Cryptographic signature verification FAILED — update discarded securely");
            Update.abort();
            http.end();
            _lastError = OTAError::SignatureVerifyFailed;
            return _lastError;
        }
        ESP_LOGI(TAG, "Cryptographic signature verification PASSED");
    } else {
        ESP_LOGW(TAG, "No OTA public key — signature verification SKIPPED (dev mode)");
    }

    // --- Step 11: Finalize update ---
    if (!Update.end()) {
        ESP_LOGE(TAG, "Update.end() failed");
        http.end();
        _lastError = OTAError::UpdateEndFailed;
        return _lastError;
    }

    http.end();
    ESP_LOGI(TAG, "OTA update finalized successfully");
    _lastError = OTAError::None;
    return _lastError;
}



String OTADownloadEngine::extractHost(const std::string& urlStr) {
    size_t idx = urlStr.find("://");
    if (idx == std::string::npos) return "";
    size_t start = idx + 3;
    size_t end = urlStr.find("/", start);
    if (end == std::string::npos) end = urlStr.length();
    return String(urlStr.substr(start, end - start).c_str());
}
