/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/otaUpdateManager/otaUpdateManager.cpp
 * Description: Implementation of the full OTA orchestrator. Preserves all legacy
 *              direct-upload functionality (beginUpdate/writeStream/endUpdate) and
 *              adds the GitHub-based checkForUpdate/performUpdate pipeline with
 *              FreeRTOS task offloading, IOtaVerifier-based cryptographic signature
 *              verification, and EventBus progress publishing.
 *
 * Exported Classes:
 * - otaUpdateManager: [Class implementation]
 *   - begin():            Store Application reference for dependency access
 *   - checkForUpdate():   Query GitHub for new releases via GithubClient
 *   - performUpdate():    Launch the download→verify→flash FreeRTOS task
 *   - performUpdateTask(): Execute the full OTA pipeline (runs in task context)
 *   - otaTaskWrapper():   Static entry point for FreeRTOS xTaskCreate
 *   - publishProgress():  Publish OtaProgressEvent to the EventBus
 *   - getState():         Return the current OTA state machine state
 *   - getLastOtaError():  Return the last error string
 *   - beginUpdate():      Legacy: initiate direct OTA with size and MD5
 *   - writeStream():      Legacy: stream data to the flash partition
 *   - endUpdate():        Legacy: finalize the direct OTA update
 *   - markAppValid():     Bless the running firmware
 *   - rollbackFirmware(): Revert to the previous firmware partition
 *   - registerRoutes():   Register REST API route handlers
 */

#include "otaUpdateManager.hpp"
#include "platform/LogShim.h"

static const char* TAG = "OtaUpdateManager";

// ---------------------------------------------------------------------------
// Platform-conditional includes
// Hardware headers are excluded from the native GoogleTest build.
// Module includes go in the .cpp ONLY (behind NATIVE_BUILD guard) to
// respect the Hollywood Principle — no cross-module includes in headers.
// ---------------------------------------------------------------------------
#ifndef NATIVE_BUILD
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <WiFiClientSecure.h>
#include "../application/Application.hpp"
#include "../../lib/githubClient/githubClient.hpp"
#include "../../lib/otaDownloadEngine/OtaDownloadEngine.hpp"
#include "../../lib/otaDownloadEngine/MbedTLSSignatureVerifier.hpp"
#else
#include <ArduinoJson.h>
#include "../application/Application.hpp"
#endif

// ---------------------------------------------------------------------------
// Global instance — REMOVED (Application now owns _otaManager)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

otaUpdateManager::otaUpdateManager() {}

// ---------------------------------------------------------------------------
// New Orchestrator API
// ---------------------------------------------------------------------------

/**
 * @brief Initialize the OTA manager with dependencies from the Application.
 *
 * Stores the Application reference for later access to EventBus, config,
 * and other framework services.
 */
void otaUpdateManager::begin(Application& app) {
    app_ = &app;
    ESP_LOGI(TAG, "OTA manager initialized with Application reference");
}

/**
 * @brief Check GitHub for a newer firmware version.
 *
 * Guards: must be in Idle state and have a configured otaGithubRepo.
 * On hardware: creates a GithubClient, queries the latest release, and
 * returns true if a new release is available. On native: always returns false.
 */
bool otaUpdateManager::checkForUpdate() {
    // Guard: must be initialized
    if (app_ == nullptr) {
        ESP_LOGE(TAG, "checkForUpdate() called before begin() — returning false");
        return false;
    }

    // Guard: must be in Idle state
    if (state_ != OtaState::Idle) {
        ESP_LOGW(TAG, "checkForUpdate() called while state != Idle — returning false");
        return false;
    }

    // Guard: must have a configured GitHub repo
    if (app_->getConfig().otaGithubRepo == nullptr) {
        ESP_LOGW(TAG, "checkForUpdate() — no otaGithubRepo configured — returning false");
        return false;
    }

    state_ = OtaState::Checking;
    ESP_LOGI(TAG, "Checking GitHub for firmware updates...");

#ifndef NATIVE_BUILD
    // Create GithubClient with repo and optional token from config
    const FrameworkConfig& cfg = app_->getConfig();
    GithubClient github(cfg.otaGithubRepo,
                        cfg.otaGithubToken ? cfg.otaGithubToken : "");

    bool success = github.getLatestRelease();
    if (!success) {
        ESP_LOGE(TAG, "GitHub release check failed: %s", github.getLastError());
        state_ = OtaState::Idle;
        return false;
    }

    ESP_LOGI(TAG, "Latest release: %s (%d assets)", github.releaseId.c_str(), github.releaseAssets);

    // For now, always report update available if a release was found.
    // A proper version comparison can be added later.
    state_ = OtaState::Idle;
    return true;
#else
    // Native builds cannot perform real GitHub checks
    state_ = OtaState::Idle;
    return false;
#endif
}

/**
 * @brief Start the full OTA download + verify + flash pipeline.
 *
 * Launches a FreeRTOS task and returns immediately. The task runs
 * performUpdateTask() which handles the complete pipeline. On native
 * builds, sets state to Error (no FreeRTOS available).
 */
void otaUpdateManager::performUpdate() {
    // Guard: must be initialized
    if (app_ == nullptr) {
        ESP_LOGE(TAG, "performUpdate() called before begin()");
        state_ = OtaState::Error;
        lastOtaError_ = "OTA manager not initialized";
        return;
    }

    // Guard: must be in Idle state
    if (state_ != OtaState::Idle) {
        ESP_LOGW(TAG, "performUpdate() called while state != Idle — ignoring");
        return;
    }

    state_ = OtaState::Downloading;
    publishProgress(0.0f, "Starting OTA update...", false, false);
    ESP_LOGI(TAG, "Launching OTA update pipeline");

#ifndef NATIVE_BUILD
    // Launch the download in a FreeRTOS task to avoid blocking tick()
    BaseType_t result = xTaskCreate(
        otaTaskWrapper,     // Task function
        "ota_update",       // Task name
        8192,               // Stack size (bytes)
        this,               // Parameter — pointer to this instance
        1,                  // Priority (low — background task)
        &taskHandle_        // Task handle output
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create OTA FreeRTOS task");
        state_ = OtaState::Error;
        lastOtaError_ = "Failed to create OTA task";
        publishProgress(0.0f, "Failed to create OTA task", true, false);
    }
#else
    // Native builds cannot run FreeRTOS tasks
    ESP_LOGW(TAG, "performUpdate() — no FreeRTOS on native build, setting Error");
    state_ = OtaState::Error;
    lastOtaError_ = "FreeRTOS not available on native build";
#endif
}

/**
 * @brief Static FreeRTOS task wrapper. Delegates to performUpdateTask().
 */
void otaUpdateManager::otaTaskWrapper(void* param) {
    auto* self = static_cast<otaUpdateManager*>(param);
    self->performUpdateTask();
#ifndef NATIVE_BUILD
    vTaskDelete(nullptr);
#endif
}

/**
 * @brief Execute the full OTA pipeline inside the FreeRTOS task.
 *
 * Steps:
 *   1. Check GitHub for releases
 *   2. Find .bin and .sig assets
 *   3. Initialize the verifier (if public key configured)
 *   4. Create WiFiClientSecure on the heap
 *   5. Create OtaDownloadEngine and configure
 *   6. Fetch signature file
 *   7. Download and flash firmware
 *   8. Finalize and reboot
 */
void otaUpdateManager::performUpdateTask() {
#ifndef NATIVE_BUILD
    ESP_LOGI(TAG, "OTA task started");
    publishProgress(0.0f, "Checking for update...", false, false);

    const FrameworkConfig& cfg = app_->getConfig();

    // --- Guard: firmware updates must be enabled ---
    if (!cfg.firmwareUpdatesEnabled) {
        ESP_LOGW(TAG, "Firmware updates are disabled in config");
        publishProgress(0.0f, "Firmware updates disabled", true, false);
        state_ = OtaState::Error;
        lastOtaError_ = "Firmware updates disabled";
        return;
    }

    // --- Guard: GitHub repo must be configured ---
    if (cfg.otaGithubRepo == nullptr) {
        ESP_LOGE(TAG, "No otaGithubRepo configured");
        publishProgress(0.0f, "No GitHub repo configured", true, false);
        state_ = OtaState::Error;
        lastOtaError_ = "No GitHub repo configured";
        return;
    }

    // --- Step 1: Check GitHub for releases ---
    GithubClient github(cfg.otaGithubRepo,
                        cfg.otaGithubToken ? cfg.otaGithubToken : "");
    ESP_LOGI(TAG, "Querying GitHub for latest release of %s", cfg.otaGithubRepo);

    if (!github.getLatestRelease()) {
        ESP_LOGE(TAG, "GitHub release query failed: %s", github.getLastError());
        publishProgress(0.0f, "GitHub query failed", true, false);
        state_ = OtaState::Error;
        lastOtaError_ = "GitHub release query failed";
        return;
    }

    ESP_LOGI(TAG, "Found release: %s with %d assets",
             github.releaseId.c_str(), github.releaseAssets);

    // --- Step 2: Find the .bin and .sig assets ---
    String binUrl = "";
    String sigUrl = "";
    String binName = "";

    for (int i = 0; i < github.releaseAssets; i++) {
        const String& name = github.releaseAssetName[i];
        int nameLen = name.length();

        // Check for .bin extension
        if (nameLen > 4 &&
            name[nameLen - 4] == '.' &&
            name[nameLen - 3] == 'b' &&
            name[nameLen - 2] == 'i' &&
            name[nameLen - 1] == 'n') {
            binUrl = github.releaseAssetURL[i];
            binName = name;
            ESP_LOGI(TAG, "Found firmware binary: %s", name.c_str());
        }

        // Check for .sig extension
        if (nameLen > 4 &&
            name[nameLen - 4] == '.' &&
            name[nameLen - 3] == 's' &&
            name[nameLen - 2] == 'i' &&
            name[nameLen - 1] == 'g') {
            sigUrl = github.releaseAssetURL[i];
            ESP_LOGI(TAG, "Found signature file: %s", name.c_str());
        }
    }

    if (binUrl.isEmpty()) {
        ESP_LOGE(TAG, "No .bin asset found in release %s", github.releaseId.c_str());
        publishProgress(0.0f, "No firmware binary in release", true, false);
        state_ = OtaState::Error;
        lastOtaError_ = "No firmware binary found in release";
        return;
    }

    if (sigUrl.isEmpty() && cfg.otaPublicKey != nullptr) {
        ESP_LOGW(TAG, "No .sig asset found but otaPublicKey is configured — security degraded");
    }

    // --- Step 3: Initialize the verifier (if public key configured) ---
    MbedTLSSignatureVerifier verifier;
    bool verifierReady = false;

    if (cfg.otaPublicKey != nullptr) {
        state_ = OtaState::Verifying;
        if (!verifier.begin(cfg.otaPublicKey)) {
            ESP_LOGE(TAG, "Failed to initialize MbedTLS verifier with public key");
            publishProgress(0.0f, "Verifier initialization failed", true, false);
            state_ = OtaState::Error;
            lastOtaError_ = "Verifier initialization failed";
            return;
        }
        verifierReady = true;
        ESP_LOGI(TAG, "Verifier initialized with public key");
        state_ = OtaState::Downloading;
    }

    // --- Step 4: Create WiFiClientSecure on the heap ---
    std::unique_ptr<WiFiClientSecure> client(new (std::nothrow) WiFiClientSecure());
    if (!client) {
        ESP_LOGE(TAG, "Failed to allocate WiFiClientSecure on heap");
        publishProgress(0.0f, "Memory allocation failed", true, false);
        state_ = OtaState::Error;
        lastOtaError_ = "WiFiClientSecure heap allocation failed";
        return;
    }
    // GitHub CDN redirects use different certs than the API — use insecure
    client->setInsecure();
    ESP_LOGI(TAG, "WiFiClientSecure allocated on heap");

    // --- Step 5: Create OtaDownloadEngine and configure ---
    OtaDownloadEngine engine;
    if (verifierReady) {
        engine.setVerifier(&verifier);
    }

    // Set progress callback to forward to EventBus
    engine.setProgressCallback([this](size_t bytesWritten, size_t totalSize) {
        float progress = (totalSize > 0)
            ? static_cast<float>(bytesWritten) / static_cast<float>(totalSize)
            : 0.0f;
        publishProgress(progress, "Downloading firmware...", false, false);
    });
    ESP_LOGI(TAG, "Download engine configured");

    // --- Step 6: Fetch signature file (if .sig URL available) ---
    uint8_t sigBuf[512];
    size_t sigLen = 0;
    String token = cfg.otaGithubToken ? cfg.otaGithubToken : "";

    if (!sigUrl.isEmpty()) {
        sigLen = engine.fetchSignature(*client, sigUrl, token, sigBuf, sizeof(sigBuf));
        if (sigLen == 0 && cfg.otaPublicKey != nullptr) {
            ESP_LOGW(TAG, "Signature fetch returned 0 bytes but public key is configured");
        } else if (sigLen > 0) {
            ESP_LOGI(TAG, "Fetched signature: %zu bytes", sigLen);
        }
    }

    publishProgress(0.1f, "Downloading firmware...", false, false);

    // --- Step 7: Download and flash firmware ---
    OtaError result = engine.downloadAndFlash(*client, binUrl, token, sigBuf, sigLen);

    if (result != OtaError::None) {
        lastOtaError_ = engine.getLastErrorString();
        ESP_LOGE(TAG, "OTA download failed: %s", lastOtaError_);
        publishProgress(0.0f, lastOtaError_, true, false);
        state_ = OtaState::Error;
        return;
    }

    ESP_LOGI(TAG, "Firmware downloaded and verified successfully");

    // --- Step 8: Finalize ---
    state_ = OtaState::Complete;
    publishProgress(1.0f, "Update complete — rebooting...", true, true);
    ESP_LOGI(TAG, "OTA update complete — rebooting in 2 seconds");

    delay(2000);  // Allow event propagation and display update
    ESP.restart();
#else
    // Native builds: no-op (should not be called)
    ESP_LOGW(TAG, "performUpdateTask() called on native build — no-op");
    state_ = OtaState::Error;
    lastOtaError_ = "Cannot perform OTA on native build";
#endif
}

/**
 * @brief Publish an OTA progress event to the EventBus.
 *
 * Creates an OtaProgressEvent on the stack and dispatches it via the
 * Application's EventBus on the "ota.progress" topic. The struct is
 * layout-compatible with OtaProgressPayload in u8g2DisplayModule.
 */
void otaUpdateManager::publishProgress(float progress, const char* status,
                                        bool complete, bool success) {
    OtaProgressEvent event;
    event.progress = progress;
    event.status   = status;
    event.complete = complete;
    event.success  = success;

#ifndef NATIVE_BUILD
    if (app_ != nullptr) {
        app_->getEventBus().publish("ota.progress", &event);
    }
#endif
    ESP_LOGD(TAG, "OTA progress: %.1f%% — %s (complete=%d, success=%d)",
             progress * 100.0f, status, complete, success);
}

/**
 * @brief Get the current OTA state machine state.
 */
OtaState otaUpdateManager::getState() const {
    return state_;
}

/**
 * @brief Get the last OTA error string.
 */
const char* otaUpdateManager::getLastOtaError() const {
    return lastOtaError_;
}

// ---------------------------------------------------------------------------
// Legacy Direct-Upload API (preserved for web upload route)
// ---------------------------------------------------------------------------

/**
 * @brief Initiates the OTA update process with size and MD5 validation.
 *
 * Used by the web server's direct firmware upload route. On native builds,
 * always returns false.
 */
bool otaUpdateManager::beginUpdate(size_t size, const char* expectedMD5) {
    ESP_LOGI(TAG, "Beginning OTA update: size=%zu, MD5=%s",
             size, expectedMD5 ? expectedMD5 : "(none)");
#ifndef NATIVE_BUILD
    // --- Step 1: Initialize the Update library ---
    if (!Update.begin(size)) {
        ESP_LOGE(TAG, "Update.begin() failed for size %zu", size);
        return false;
    }

    // --- Step 2: Set the expected MD5 hash ---
    // The Update library will automatically hash the written data
    // and compare it to this string when Update.end() is called.
    if (expectedMD5 != nullptr) {
        Update.setMD5(expectedMD5);
    }

    expectedSize_ = size;
    return true;
#else
    return false;
#endif
}

/**
 * @brief Writes data from the provided stream directly into the OTA partition.
 */
size_t otaUpdateManager::writeStream(Stream& dataStream) {
#ifndef NATIVE_BUILD
    size_t written = Update.writeStream(dataStream);
    ESP_LOGD(TAG, "writeStream: %zu bytes written", written);
    return written;
#else
    return 0;
#endif
}

/**
 * @brief Finalizes the OTA update. Checks that total bytes and MD5 match.
 */
bool otaUpdateManager::endUpdate() {
#ifndef NATIVE_BUILD
    if (Update.end()) {
        ESP_LOGI(TAG, "OTA update finalized successfully");
        return true;
    }
    ESP_LOGE(TAG, "OTA update finalization failed");
    return false;
#else
    return false;
#endif
}

/**
 * @brief Blesses the current running firmware by cancelling auto-rollback.
 */
void otaUpdateManager::markAppValid() {
    ESP_LOGI(TAG, "Marking firmware as valid (blessing app)");
#ifndef NATIVE_BUILD
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    esp_ota_mark_app_valid_cancel_rollback();
#endif
#endif
}

/**
 * @brief Triggers a rollback to the previous firmware partition and reboots.
 */
void otaUpdateManager::rollbackFirmware() {
    ESP_LOGW(TAG, "Firmware rollback triggered");
#ifndef NATIVE_BUILD
    const esp_partition_t* updatePartition = esp_ota_get_next_update_partition(NULL);

    if (updatePartition) {
        esp_ota_set_boot_partition(updatePartition);
        ESP.restart();
    }
#endif
}

// ---------------------------------------------------------------------------
// IWebContributor — REST API Routes
// ---------------------------------------------------------------------------

/**
 * @brief Register all REST API route handlers for OTA management.
 *
 * Routes:
 *   GET  /api/ota                          → status JSON
 *   POST /api/ota { "action": "rollback" } → trigger rollback
 *   POST /api/ota { "action": "markValid" }→ bless firmware
 *   POST /api/ota { "action": "checkGithub" } → check + perform update
 *   POST /api/ota { "action": "status" }   → current state + error
 */
void otaUpdateManager::registerRoutes(AsyncWebServer& server) {
#ifndef NATIVE_BUILD
    // --- GET: status endpoint ---
    server.on("/api/ota", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["status"] = "ready";
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // --- POST: action dispatcher ---
    server.on("/api/ota", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
               size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            if (!error) {
                const char* action = doc["action"] | "";

                if (strcmp(action, "rollback") == 0) {
                    request->send(200, "application/json", "{\"status\":\"ok\"}");
                    this->rollbackFirmware();
                    return;
                } else if (strcmp(action, "markValid") == 0) {
                    this->markAppValid();
                    request->send(200, "application/json", "{\"status\":\"ok\"}");
                    return;
                } else if (strcmp(action, "checkGithub") == 0) {
                    ESP_LOGI(TAG, "GitHub OTA check triggered via API");
                    bool updateAvailable = this->checkForUpdate();
                    if (updateAvailable) {
                        this->performUpdate();
                        request->send(200, "application/json",
                                      "{\"status\":\"ok\",\"msg\":\"Update started\"}");
                    } else {
                        request->send(200, "application/json",
                                      "{\"status\":\"ok\",\"msg\":\"No update available\"}");
                    }
                    return;
                } else if (strcmp(action, "status") == 0) {
                    JsonDocument resp;
                    const char* stateStr = "unknown";
                    switch (this->state_) {
                        case OtaState::Idle:        stateStr = "idle"; break;
                        case OtaState::Checking:    stateStr = "checking"; break;
                        case OtaState::Downloading: stateStr = "downloading"; break;
                        case OtaState::Verifying:   stateStr = "verifying"; break;
                        case OtaState::Complete:    stateStr = "complete"; break;
                        case OtaState::Error:       stateStr = "error"; break;
                    }
                    resp["state"] = stateStr;
                    if (this->lastOtaError_) {
                        resp["lastError"] = this->lastOtaError_;
                    }
                    String output;
                    serializeJson(resp, output);
                    request->send(200, "application/json", output);
                    return;
                }
            }
            request->send(400, "application/json",
                          "{\"status\":\"error\",\"msg\":\"Invalid action\"}");
        }
    );
#endif
}
