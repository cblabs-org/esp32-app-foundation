/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/OTADownloadEngine/OTADownloadEngine.cpp
 * Description: Implementation of the OTA Download Engine — redirect-aware
 *              GitHub binary downloader with signature fetch, ESP32 header
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

static constexpr const char* TAG = "OTADownloadEngine";

// =============================================================================
// Common implementations (both hardware and native)
// =============================================================================

/**
 * @brief Default constructor — initializes all members to safe defaults.
 */
OTADownloadEngine::OTADownloadEngine()
    : verifier_(nullptr)
    , progressCb_(nullptr)
    , progressCtx_(nullptr)
    , httpTimeout_(8000)
    , lastError_(OTAError::None)
{
}

/**
 * @brief Set the IOTAVerifier to use during download. Ownership is NOT transferred.
 * @param verifier Pointer to an IOTAVerifier instance, or nullptr to disable verification.
 */
void OTADownloadEngine::setVerifier(IOTAVerifier* verifier) {
    verifier_ = verifier;
}

/**
 * @brief Set the progress callback invoked after each chunk is written.
 * @param cb Callback receiving (bytesWritten, totalSize).
 */
void OTADownloadEngine::setProgressCallback(OTAProgressCallback cb, void* context) {
    progressCb_ = cb;
    progressCtx_ = context;
}

/**
 * @brief Set the HTTP client timeout in milliseconds.
 * @param timeoutMs Timeout value (default: 8000).
 */
void OTADownloadEngine::setHttpTimeout(int timeoutMs) {
    httpTimeout_ = timeoutMs;
}

/**
 * @brief Get the last error code.
 * @return The OTAError from the most recent operation.
 */
OTAError OTADownloadEngine::getLastError() const {
    return lastError_;
}

/**
 * @brief Get human-readable string for the last error.
 * @return Null-terminated C-string describing the error.
 */
const char* OTADownloadEngine::getLastErrorString() const {
    return otaErrorToString(lastError_);
}

/**
 * @brief Set the expected firmware version for pre-flash validation.
 *
 * Stores the version string (stripping any leading 'v'/'V' prefix) into
 * the fixed-size expectedVersion_ buffer. When set, downloadAndFlash()
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
        strncpy(expectedVersion_, v, sizeof(expectedVersion_) - 1);
        expectedVersion_[sizeof(expectedVersion_) - 1] = '\0';
    } else {
        expectedVersion_[0] = '\0';
    }
}


#ifndef NATIVE_BUILD
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
 * with GitHub-appropriate headers (User-Agent, Cache-Control, Accept,
 * Authorization, X-GitHub-Api-Version) and disables auto-redirect to
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

    for (int hop = 0; hop < kMaxRedirects; hop++) {
        // --- Begin HTTP request for this hop ---
        if (!http.begin(client, String(currentUrl.c_str()))) {
            ESP_LOGE(TAG, "http.begin() failed for URL: %s", currentUrl.c_str());
            return -1;
        }

        // Configure headers for GitHub release asset API
        http.useHTTP10(true);
        http.setTimeout(httpTimeout_);
        http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
        http.setUserAgent("ESP32-http-Update");
        http.addHeader("Cache-Control", "no-cache");
        http.addHeader("Accept", "application/octet-stream");
        if (!token.empty()) {
            http.addHeader("Authorization", String(("Bearer " + token).c_str()));
        }
        http.addHeader("X-GitHub-Api-Version", "2022-11-28");

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
        lastError_ = OTAError::SignatureFetchFailed;
        return 0;
    }

    // --- Read signature bytes with timeout guard ---
    WiFiClient* tcp = http.getStreamPtr();
    size_t bytesRead = 0;
    unsigned long timeout = millis() + httpTimeout_;

    while (bytesRead < static_cast<size_t>(contentLen) && millis() < timeout) {
        if (tcp->available()) {
            int r = tcp->readBytes(sigBuf + bytesRead, contentLen - bytesRead);
            bytesRead += r;
            timeout = millis() + httpTimeout_; // Reset timeout on progress
        } else {
            delay(1); // Yield to RTOS scheduler
        }
    }

    http.end();

    if (bytesRead != static_cast<size_t>(contentLen)) {
        ESP_LOGE(TAG, "Signature read incomplete — got %u of %d bytes", (unsigned)bytesRead, contentLen);
        lastError_ = OTAError::SignatureFetchFailed;
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
        lastError_ = OTAError::HttpConnectionFailed;
        return lastError_;
    }

    if (code == HTTP_CODE_NOT_FOUND) {
        ESP_LOGE(TAG, "Binary not found — server returned 404");
        http.end();
        lastError_ = OTAError::HttpFileNotFound;
        return lastError_;
    }

    if (code == HTTP_CODE_FORBIDDEN) {
        ESP_LOGE(TAG, "Access forbidden — server returned 403");
        http.end();
        lastError_ = OTAError::HttpForbidden;
        return lastError_;
    }

    if (code != HTTP_CODE_OK) {
        ESP_LOGE(TAG, "Unexpected HTTP response code: %d", code);
        http.end();
        lastError_ = OTAError::HttpWrongResponseCode;
        return lastError_;
    }

    // --- Step 3: Validate content-length ---
    int contentLen = http.getSize();
    if (contentLen <= 0) {
        ESP_LOGE(TAG, "Server did not report Content-Length");
        http.end();
        lastError_ = OTAError::ServerNoContentLength;
        return lastError_;
    }

    ESP_LOGI(TAG, "Binary size reported: %d bytes", contentLen);

    // --- Step 4: Check partition space ---
    int freeSpace = ESP.getFreeSketchSpace();
    if (freeSpace == 0) {
        ESP_LOGE(TAG, "No OTA partition available");
        http.end();
        lastError_ = OTAError::NoOtaPartition;
        return lastError_;
    }

    if (contentLen > freeSpace) {
        ESP_LOGE(TAG, "Insufficient space — need %d bytes, have %d", contentLen, freeSpace);
        http.end();
        lastError_ = OTAError::InsufficientSpace;
        return lastError_;
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
        lastError_ = OTAError::StreamReadTimeout;
        return lastError_;
    }

    // --- Step 5a: Validate ESP32 binary magic byte ---
    if (buf[0] != 0xE9) {
        ESP_LOGE(TAG, "Binary header invalid — expected 0xE9, got 0x%02X", buf[0]);
        http.end();
        lastError_ = OTAError::BinaryHeaderInvalid;
        return lastError_;
    }

    // --- Step 5b: Pre-flash binary version validation (safety gate) ---
    // The esp_app_desc_t sits at byte offset 32 (after esp_image_header_t +
    // esp_image_segment_header_t) and is 256 bytes. Total: 288 bytes needed.
    // kChunkSize is 1024, so this is always available in the first chunk.
    static constexpr size_t kAppDescOffset = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t);

    if (expectedVersion_[0] != '\0' &&
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
            // often includes it, but expectedVersion_ already has it stripped.
            const char* cmpVersion = binaryVersion;
            if (cmpVersion[0] == 'v' || cmpVersion[0] == 'V') {
                cmpVersion++;
            }

            if (strncmp(cmpVersion, expectedVersion_, sizeof(binaryVersion)) != 0) {
                ESP_LOGE(TAG, "SAFETY ABORT: Binary version mismatch — binary contains '%s', expected '%s'",
                         cmpVersion, expectedVersion_);
                http.end();
                lastError_ = OTAError::BinaryVersionMismatch;
                return lastError_;
            }
            ESP_LOGI(TAG, "Binary version validated: '%s' matches expected", cmpVersion);
        }
    } else if (expectedVersion_[0] != '\0') {
        ESP_LOGW(TAG, "First chunk too small for app descriptor (%d bytes) — skipping version check",
                 firstChunkLen);
    }

    // --- Step 6: Begin Update ---
    if (!Update.begin(contentLen)) {
        ESP_LOGE(TAG, "Update.begin() failed for size %d", contentLen);
        http.end();
        lastError_ = OTAError::UpdateBeginFailed;
        return lastError_;
    }

    // --- Step 7: Warn if no verifier set ---
    if (verifier_ == nullptr) {
        ESP_LOGW(TAG, "No IOTAVerifier set — proceeding WITHOUT cryptographic verification");
    }

    // --- Step 8: Report initial progress ---
    if (progressCb_) {
        progressCb_(progressCtx_, 0, static_cast<size_t>(contentLen));
    }

    ESP_LOGI(TAG, "Streaming firmware (%d bytes) in %u-byte chunks...", contentLen, (unsigned)kChunkSize);

    // --- Step 8b: Write the already-buffered first chunk ---
    if (verifier_ != nullptr) {
        verifier_->update(buf, firstChunkLen);
    }
    if (Update.write(buf, firstChunkLen) != static_cast<size_t>(firstChunkLen)) {
        ESP_LOGE(TAG, "Update.write() failed on first chunk");
        Update.abort();
        http.end();
        lastError_ = OTAError::UpdateWriteFailed;
        return lastError_;
    }

    size_t totalWritten = static_cast<size_t>(firstChunkLen);

    if (progressCb_) {
        progressCb_(progressCtx_, totalWritten, static_cast<size_t>(contentLen));
    }

    // --- Step 9: Chunked streaming write for remaining data ---
    while (totalWritten < static_cast<size_t>(contentLen)) {
        size_t remaining = static_cast<size_t>(contentLen) - totalWritten;
        size_t bytesToRead = (remaining < kChunkSize) ? remaining : kChunkSize;

        // Wait for data with retry — TLS record boundaries can cause
        // brief gaps where no decrypted data is immediately available
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
            lastError_ = OTAError::StreamReadTimeout;
            return lastError_;
        }

        // Feed chunk to cryptographic verifier if present
        if (verifier_ != nullptr) {
            verifier_->update(buf, bytesRead);
        }

        // Write chunk to OTA partition
        if (Update.write(buf, bytesRead) != static_cast<size_t>(bytesRead)) {
            ESP_LOGE(TAG, "Update.write() failed at offset %u", (unsigned)totalWritten);
            Update.abort();
            http.end();
            lastError_ = OTAError::UpdateWriteFailed;
            return lastError_;
        }

        totalWritten += bytesRead;

        // Yield to network stack between chunks
        delay(1);

        // Report progress
        if (progressCb_) {
            progressCb_(progressCtx_, totalWritten, static_cast<size_t>(contentLen));
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
    if (verifier_ != nullptr) {
        // Fail closed: verifier is set, signature MUST be present
        if (sigBuf == nullptr || sigLen == 0) {
            ESP_LOGE(TAG, "SECURITY ABORT: Verifier configured but no signature provided — aborting");
            Update.abort();
            http.end();
            lastError_ = OTAError::SignatureFetchFailed;
            return lastError_;
        }
        if (!verifier_->verify(sigBuf, sigLen)) {
            ESP_LOGE(TAG, "Cryptographic signature verification FAILED — update discarded securely");
            Update.abort();
            http.end();
            lastError_ = OTAError::SignatureVerifyFailed;
            return lastError_;
        }
        ESP_LOGI(TAG, "Cryptographic signature verification PASSED");
    } else {
        ESP_LOGW(TAG, "No OTA public key — signature verification SKIPPED (dev mode)");
    }

    // --- Step 11: Finalize update ---
    if (!Update.end()) {
        ESP_LOGE(TAG, "Update.end() failed");
        http.end();
        lastError_ = OTAError::UpdateEndFailed;
        return lastError_;
    }

    http.end();
    ESP_LOGI(TAG, "OTA update finalized successfully");
    lastError_ = OTAError::None;
    return lastError_;
}


#else
// =============================================================================
// Native build stubs — minimal implementations for unit test compilation
// =============================================================================

/**
 * @brief Native stub — redirect following is not available without hardware.
 * @return Always returns -1 (failure).
 */
int OTADownloadEngine::followRedirects(WiFiClient& client, HTTPClient& http,
                                        const std::string& url, const std::string& token) {
    (void)client; (void)http; (void)url; (void)token;
    ESP_LOGD(TAG, "followRedirects() stub — native build");
    return -1;
}

/**
 * @brief Native stub — signature fetch is not available without hardware.
 * @return Always returns 0 (no bytes read).
 */
size_t OTADownloadEngine::fetchSignature(WiFiClient& client, const std::string& sigUrl,
                                          const std::string& token, uint8_t* sigBuf, size_t sigBufSize) {
    (void)client; (void)sigUrl; (void)token; (void)sigBuf; (void)sigBufSize;
    ESP_LOGD(TAG, "fetchSignature() stub — native build");
    lastError_ = OTAError::HttpConnectionFailed;
    return 0;
}

/**
 * @brief Native stub — download and flash is not available without hardware.
 * @return Always returns OTAError::HttpConnectionFailed.
 */
OTAError OTADownloadEngine::downloadAndFlash(WiFiClient& client, const std::string& binUrl,
                                              const std::string& token, const uint8_t* sigBuf, size_t sigLen) {
    (void)client; (void)binUrl; (void)token; (void)sigBuf; (void)sigLen;
    ESP_LOGD(TAG, "downloadAndFlash() stub — native build");
    lastError_ = OTAError::HttpConnectionFailed;
    return lastError_;
}

#endif // NATIVE_BUILD
