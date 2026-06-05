/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/OTAUpdateManager/OTAUpdateManager.cpp
 * Description: Implementation of the full OTA orchestrator. Preserves all legacy
 *              direct-upload functionality (beginUpdate/writeStream/endUpdate) and
 *              adds the GitHub-based checkForUpdate/performUpdate pipeline with
 *              FreeRTOS task offloading, IOTAVerifier-based cryptographic signature
 *              verification, and EventBus progress publishing.
 *
 * Exported Classes:
 * - OTAUpdateManager: [Class implementation]
 *   - begin():              Store Application reference for dependency access
 *   - checkForUpdate():     Query GitHub for new releases via GithubClient
 *   - performUpdate():      Launch the download→verify→flash FreeRTOS task
 *   - performUpdateTask():  Execute the full OTA pipeline (runs in task context)
 *   - otaTaskWrapper():     Static entry point for FreeRTOS xTaskCreate
 *   - publishProgress():    Publish OTAProgressEvent to the EventBus
 *   - getState():           Return the current OTA state machine state
 *   - getLastOtaError():    Return the last error string
 *   - beginUpdate():        Legacy: initiate direct OTA with size and MD5
 *   - writeStream():        Legacy: stream data to the flash partition
 *   - endUpdate():          Legacy: finalize the direct OTA update
 *   - markAppValid():       Bless the running firmware
 *   - rollbackFirmware():   Revert to the previous firmware partition
 *   - registerRoutes():     Register REST API route handlers
 *   - getConfigNamespace(): Return "ota" (IConfigurable)
 *   - getSchemaVersion():   Return 2 (IConfigurable)
 *   - applyConfig():        Apply OTA config from JSON section (IConfigurable)
 *   - serializeConfig():    Serialize OTA config to JSON section (IConfigurable)
 *   - migrateConfig():      Migrate from older schema version (IConfigurable)
 *   - resetConfig():        Restore OTA config factory defaults (IConfigurable)
 */

#include "OTAUpdateManager.hpp"
#include "platform/LogShim.h"

static const char* TAG = "OTAUpdateManager";

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
#include <HTTPClient.h>
#include <Preferences.h>
#include "esp_task_wdt.h"
#include "../Application/Application.hpp"
#include "../../lib/GithubClient/GithubClient.hpp"
#include "../../lib/OTADownloadEngine/OTADownloadEngine.hpp"
#include "../../lib/OTADownloadEngine/MbedTLSSignatureVerifier.hpp"
#include "../../lib/SemVer/SemVer.hpp"
#include "../../lib/OTAManifest/OTAManifest.hpp"
#else
#include <ArduinoJson.h>
#include "../Application/Application.hpp"
#endif

// ---------------------------------------------------------------------------
// Global instance — REMOVED (Application now owns _otaManager)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

OTAUpdateManager::OTAUpdateManager() {}

// ---------------------------------------------------------------------------
// IConfigurable Implementation
// ---------------------------------------------------------------------------

/**
 * @brief Return the unique config namespace for OTA settings.
 *
 * Maps to /mod/ota.json on the device filesystem, managed exclusively
 * by ConfigManager. Module code never reads or writes this file directly.
 */
const char* OTAUpdateManager::getConfigNamespace() const {
    return "ota";
}

/**
 * @brief Return the current schema version for the OTA config format.
 *
 * Version 2: adds "lastCheckedChannel" for implicit downgrade detection.
 * Increment this when a future firmware change requires a config migration.
 */
int OTAUpdateManager::getSchemaVersion() const {
    return 2;
}

/**
 * @brief Apply OTA configuration values from a parsed JSON section.
 *
 * Reads "channel", "autoCheck", and "lastCheckedChannel" from the section.
 * Missing keys fall back to safe defaults via ArduinoJson's | operator.
 *
 * @param section Read-only reference to the JSON object containing config keys.
 */
void OTAUpdateManager::applyConfig(const JsonObject& section) {
    const char* ch = section["channel"] | "stable";
    strlcpy(_channel, ch, sizeof(_channel));
    _autoCheck = section["autoCheck"] | true;
    const char* lcc = section["lastCheckedChannel"] | "";
    strlcpy(_lastCheckedChannel, lcc, sizeof(_lastCheckedChannel));
    ESP_LOGI(TAG, "Config applied — channel='%s' autoCheck=%d lastCheckedChannel='%s'",
             _channel, (int)_autoCheck, _lastCheckedChannel);
}

/**
 * @brief Serialize current OTA configuration into a JSON section.
 *
 * Writes "channel", "autoCheck", and "lastCheckedChannel" into the
 * provided section for persistence by ConfigManager (stored at /mod/ota.json).
 *
 * @param section Mutable reference to the target JSON object.
 */
void OTAUpdateManager::serializeConfig(JsonObject& section) const {
    section["channel"]            = _channel;
    section["autoCheck"]          = _autoCheck;
    section["lastCheckedChannel"] = _lastCheckedChannel;
}

/**
 * @brief Migrate OTA configuration from an older schema version.
 *
 * Handles v1→v2 migration by adding the lastCheckedChannel field.
 *
 * @param section   Mutable reference to the JSON config section.
 * @param fromVersion The stored schema version being migrated from.
 * @return true on success.
 */
bool OTAUpdateManager::migrateConfig(JsonObject& section, int fromVersion) {
    if (fromVersion < 2) {
        section["lastCheckedChannel"] = "";
        ESP_LOGI(TAG, "Migrated OTA config v%d → v2: added lastCheckedChannel", fromVersion);
    }
    return true;
}

/**
 * @brief Reset OTA configuration to factory defaults.
 *
 * Sets channel to "stable", autoCheck to true, and clears
 * lastCheckedChannel. ConfigManager calls saveModuleConfig() afterwards
 * to persist the reset state.
 */
void OTAUpdateManager::resetConfig() {
    strlcpy(_channel, "stable", sizeof(_channel));
    _autoCheck = true;
    _lastCheckedChannel[0] = '\0';
    ESP_LOGI(TAG, "Config reset to defaults — channel='stable' autoCheck=true lastCheckedChannel=''");
}



/**
 * @brief Initialize the OTA manager with dependencies from the Application.
 *
 * Stores the Application reference for later access to EventBus, config,
 * and other framework services.
 */
void OTAUpdateManager::begin(Application& app) {
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
bool OTAUpdateManager::checkForUpdate() {
    // Guard: must be initialized
    if (app_ == nullptr) {
        ESP_LOGE(TAG, "checkForUpdate() called before begin() — returning false");
        return false;
    }

    // Guard: must be in Idle state
    if (state_ != OTAState::Idle) {
        ESP_LOGW(TAG, "checkForUpdate() called while state != Idle — returning false");
        return false;
    }

    // Guard: must have a configured GitHub repo
    if (app_->getConfig().otaGithubRepo == nullptr) {
        ESP_LOGW(TAG, "checkForUpdate() — no otaGithubRepo configured — returning false");
        return false;
    }

    state_ = OTAState::Checking;
    ESP_LOGI(TAG, "Checking GitHub for firmware updates...");

#ifndef NATIVE_BUILD
    // Create GithubClient with repo and optional token from config
    const FrameworkConfig& cfg = app_->getConfig();
    GithubClient github(cfg.otaGithubRepo,
                        cfg.otaGithubToken ? cfg.otaGithubToken : "");

    bool success = github.getLatestReleaseForChannel(_channel);
    if (!success) {
        ESP_LOGE(TAG, "GitHub release check failed: %s", github.getLastError());
        state_ = OTAState::Idle;
        return false;
    }

    ESP_LOGI(TAG, "Latest release: %s (%d assets)", github.releaseId.c_str(), github.releaseAssets);

    // --- SemVer comparison: local build version vs. remote release tag ---
    FirmwareVersion local = { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, Channel::STABLE, 0 };
    FirmwareVersion remote = parseVersionTag(github.releaseId.c_str());

    // Normal case: remote > local → update available
    if (remote > local) {
        ESP_LOGI(TAG, "Update available: remote version is newer");
        strlcpy(_lastCheckedChannel, _channel, sizeof(_lastCheckedChannel));
        state_ = OTAState::Idle;
        return true;
    }

    // Implicit downgrade: channel changed since last check
    // e.g., switched from beta to stable — allow "downgrade" to the latest stable
    if (strcmp(_channel, _lastCheckedChannel) != 0 && _lastCheckedChannel[0] != '\0') {
        ESP_LOGI(TAG, "Channel changed ('%s' → '%s') — allowing implicit version change",
                 _lastCheckedChannel, _channel);
        strlcpy(_lastCheckedChannel, _channel, sizeof(_lastCheckedChannel));
        // Save the channel change to config
        if (app_ != nullptr) {
            app_->getConfigManager().saveModuleConfig(this);
        }
        state_ = OTAState::Idle;
        return true;  // Signal update available even if remote <= local
    }

    // Update lastCheckedChannel
    strlcpy(_lastCheckedChannel, _channel, sizeof(_lastCheckedChannel));

    ESP_LOGI(TAG, "No update available — local version is current or newer");
    state_ = OTAState::Idle;
    return false;
#else
    // Native builds cannot perform real GitHub checks
    state_ = OTAState::Idle;
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
void OTAUpdateManager::performUpdate() {
    // Guard: must be initialized
    if (app_ == nullptr) {
        ESP_LOGE(TAG, "performUpdate() called before begin()");
        state_ = OTAState::Error;
        lastOtaError_ = "OTA manager not initialized";
        return;
    }

    // Guard: must be in Idle state
    if (state_ != OTAState::Idle) {
        ESP_LOGW(TAG, "performUpdate() called while state != Idle — ignoring");
        return;
    }

    state_ = OTAState::Downloading;
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
        state_ = OTAState::Error;
        lastOtaError_ = "Failed to create OTA task";
        publishProgress(0.0f, "Failed to create OTA task", true, false);
    }
#else
    // Native builds cannot run FreeRTOS tasks
    ESP_LOGW(TAG, "performUpdate() — no FreeRTOS on native build, setting Error");
    state_ = OTAState::Error;
    lastOtaError_ = "FreeRTOS not available on native build";
#endif
}

/**
 * @brief Static FreeRTOS task wrapper. Delegates to performUpdateTask().
 */
void OTAUpdateManager::otaTaskWrapper(void* param) {
    auto* self = static_cast<OTAUpdateManager*>(param);
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
 *   5. Create OTADownloadEngine and configure
 *   6. Fetch signature file
 *   7. Download and flash firmware
 *   8. Finalize and reboot
 */
void OTAUpdateManager::performUpdateTask() {
#ifndef NATIVE_BUILD
    ESP_LOGI(TAG, "OTA task started");
    esp_task_wdt_add(NULL);  // Subscribe current task to TWDT
    publishProgress(0.0f, "Checking for update...", false, false);

    const FrameworkConfig& cfg = app_->getConfig();

    // --- Guard: firmware updates must be enabled ---
    if (!cfg.firmwareUpdatesEnabled) {
        ESP_LOGW(TAG, "Firmware updates are disabled in config");
        publishProgress(0.0f, "Firmware updates disabled", true, false);
        state_ = OTAState::Error;
        lastOtaError_ = "Firmware updates disabled";
        esp_task_wdt_delete(NULL);
        return;
    }

    // --- Guard: GitHub repo must be configured ---
    if (cfg.otaGithubRepo == nullptr) {
        ESP_LOGE(TAG, "No otaGithubRepo configured");
        publishProgress(0.0f, "No GitHub repo configured", true, false);
        state_ = OTAState::Error;
        lastOtaError_ = "No GitHub repo configured";
        esp_task_wdt_delete(NULL);
        return;
    }

    // --- Step 1: Check GitHub for releases ---
    GithubClient github(cfg.otaGithubRepo,
                        cfg.otaGithubToken ? cfg.otaGithubToken : "");
    ESP_LOGI(TAG, "Querying GitHub for latest release of %s", cfg.otaGithubRepo);

    if (!github.getLatestReleaseForChannel(_channel)) {
        ESP_LOGE(TAG, "GitHub release query failed: %s", github.getLastError());
        publishProgress(0.0f, "GitHub query failed", true, false);
        state_ = OTAState::Error;
        lastOtaError_ = "GitHub release query failed";
        esp_task_wdt_delete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Found release: %s with %d assets",
             github.releaseId.c_str(), github.releaseAssets);

    // --- Step 2: Find firmware and signature assets (manifest-aware) ---
    String binUrl = "";
    String sigUrl = "";
    String binName = "";

    // Check if this release contains a manifest.json
    String manifestUrl = "";
    for (int i = 0; i < github.releaseAssets; i++) {
        if (strcmp(github.releaseAssetName[i].c_str(), "manifest.json") == 0) {
            manifestUrl = github.releaseAssetURL[i];
            ESP_LOGI(TAG, "Found manifest.json asset");
            break;
        }
    }

    if (!manifestUrl.isEmpty()) {
        // --- Manifest path: download and parse manifest.json ---
        ESP_LOGI(TAG, "Downloading manifest.json for board '%s'", BOARD_ID);

        std::unique_ptr<WiFiClientSecure> manifestClient(new (std::nothrow) WiFiClientSecure());
        if (!manifestClient) {
            ESP_LOGE(TAG, "Failed to allocate WiFiClientSecure for manifest fetch");
            publishProgress(0.0f, "Memory allocation failed", true, false);
            state_ = OTAState::Error;
            lastOtaError_ = "Manifest client allocation failed";
            esp_task_wdt_delete(NULL);
            return;
        }
        manifestClient->setInsecure();

        HTTPClient http;
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        String manifestToken = cfg.otaGithubToken ? cfg.otaGithubToken : "";

        if (!http.begin(*manifestClient, manifestUrl)) {
            ESP_LOGE(TAG, "HTTPClient.begin() failed for manifest URL");
            publishProgress(0.0f, "Manifest download failed", true, false);
            state_ = OTAState::Error;
            lastOtaError_ = "Manifest HTTP begin failed";
            esp_task_wdt_delete(NULL);
            return;
        }

        http.addHeader("Accept", "application/octet-stream");
        if (!manifestToken.isEmpty()) {
            http.addHeader("Authorization", String("Bearer ") + manifestToken);
        }

        int httpCode = http.GET();
        if (httpCode != 200) {
            ESP_LOGE(TAG, "Manifest download HTTP %d", httpCode);
            http.end();
            publishProgress(0.0f, "Manifest download failed", true, false);
            state_ = OTAState::Error;
            lastOtaError_ = "Manifest download HTTP error";
            esp_task_wdt_delete(NULL);
            return;
        }

        char manifestBuf[2048];
        int payloadLen = http.getSize();
        if (payloadLen <= 0 || payloadLen >= (int)sizeof(manifestBuf)) {
            ESP_LOGE(TAG, "Manifest payload size invalid: %d", payloadLen);
            http.end();
            publishProgress(0.0f, "Manifest too large or empty", true, false);
            state_ = OTAState::Error;
            lastOtaError_ = "Manifest payload size invalid";
            esp_task_wdt_delete(NULL);
            return;
        }

        WiFiClient* stream = http.getStreamPtr();
        int bytesRead = stream->readBytes(manifestBuf, payloadLen);
        manifestBuf[bytesRead] = '\0';
        http.end();

        ESP_LOGI(TAG, "Manifest downloaded: %d bytes", bytesRead);

        // Parse the manifest
        OTAManifest manifest;
        if (!parseManifest(manifestBuf, bytesRead, manifest)) {
            ESP_LOGE(TAG, "Failed to parse manifest.json");
            publishProgress(0.0f, "Manifest parse failed", true, false);
            state_ = OTAState::Error;
            lastOtaError_ = "Manifest JSON parse failed";
            esp_task_wdt_delete(NULL);
            return;
        }

        // Find the asset matching this board
        const OTAAsset* boardAsset = findAssetForBoard(manifest, BOARD_ID);
        if (boardAsset == nullptr) {
            ESP_LOGE(TAG, "No firmware for board '%s' in manifest", BOARD_ID);
            publishProgress(0.0f, "No firmware for this board in manifest", true, false);
            state_ = OTAState::Error;
            lastOtaError_ = "No firmware for this board in manifest";
            esp_task_wdt_delete(NULL);
            return;
        }

        ESP_LOGI(TAG, "Manifest: found asset for board '%s' — firmware='%s' sig='%s'",
                 boardAsset->board, boardAsset->firmware, boardAsset->signature);

        // Match manifest filenames to release asset URLs
        for (int i = 0; i < github.releaseAssets; i++) {
            if (strcmp(github.releaseAssetName[i].c_str(), boardAsset->firmware) == 0) {
                binUrl = github.releaseAssetURL[i];
                binName = github.releaseAssetName[i];
                ESP_LOGI(TAG, "Matched firmware binary URL: %s", binName.c_str());
            }
            if (strcmp(github.releaseAssetName[i].c_str(), boardAsset->signature) == 0) {
                sigUrl = github.releaseAssetURL[i];
                ESP_LOGI(TAG, "Matched signature file URL: %s", github.releaseAssetName[i].c_str());
            }
        }
    } else {
        // --- Legacy fallback: no manifest.json found ---
        ESP_LOGW(TAG, "No manifest.json found — using legacy asset search");

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
    }

    if (binUrl.isEmpty()) {
        ESP_LOGE(TAG, "No .bin asset found in release %s", github.releaseId.c_str());
        publishProgress(0.0f, "No firmware binary in release", true, false);
        state_ = OTAState::Error;
        lastOtaError_ = "No firmware binary found in release";
        esp_task_wdt_delete(NULL);
        return;
    }

    if (sigUrl.isEmpty() && cfg.otaPublicKey != nullptr) {
        ESP_LOGE(TAG, "SECURITY ABORT: No .sig asset found but otaPublicKey is configured — refusing unsigned firmware");
        publishProgress(0.0f, "Missing signature — update rejected", true, false);
        state_ = OTAState::Error;
        lastOtaError_ = "Missing .sig file — update rejected (fail-closed)";
        esp_task_wdt_delete(NULL);
        return;
    }

    // --- Snapshot config manifest: Active → Previous before download ---
    app_->getConfigManager().getManifest().snapshotActiveToPrevious();
    app_->getConfigManager().getManifest().save();
    ESP_LOGI(TAG, "Config manifest: Active → Previous snapshot saved");

    // --- Step 3: Initialize the verifier (if public key configured) ---
    MbedTLSSignatureVerifier verifier;
    bool verifierReady = false;

    if (cfg.otaPublicKey != nullptr) {
        state_ = OTAState::Verifying;
        if (!verifier.begin(cfg.otaPublicKey)) {
            ESP_LOGE(TAG, "Failed to initialize MbedTLS verifier with public key");
            publishProgress(0.0f, "Verifier initialization failed", true, false);
            state_ = OTAState::Error;
            lastOtaError_ = "Verifier initialization failed";
            esp_task_wdt_delete(NULL);
            return;
        }
        verifierReady = true;
        ESP_LOGI(TAG, "Verifier initialized with public key");
        state_ = OTAState::Downloading;
    }

    // --- Step 4: Create WiFiClientSecure on the heap ---
    std::unique_ptr<WiFiClientSecure> client(new (std::nothrow) WiFiClientSecure());
    if (!client) {
        ESP_LOGE(TAG, "Failed to allocate WiFiClientSecure on heap");
        publishProgress(0.0f, "Memory allocation failed", true, false);
        state_ = OTAState::Error;
        lastOtaError_ = "WiFiClientSecure heap allocation failed";
        esp_task_wdt_delete(NULL);
        return;
    }
    // GitHub CDN redirects use different certs than the API — use insecure
    client->setInsecure();
    ESP_LOGI(TAG, "WiFiClientSecure allocated on heap");

    // --- Step 5: Create OTADownloadEngine and configure ---
    OTADownloadEngine engine;
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
            ESP_LOGE(TAG, "SECURITY ABORT: Signature fetch returned 0 bytes — refusing update");
            publishProgress(0.0f, "Signature fetch failed — update rejected", true, false);
            state_ = OTAState::Error;
            lastOtaError_ = "Signature empty — update rejected (fail-closed)";
            esp_task_wdt_delete(NULL);  // Unsubscribe before early return
            return;
        } else if (sigLen > 0) {
            ESP_LOGI(TAG, "Fetched signature: %zu bytes", sigLen);
        }
    }

    publishProgress(0.1f, "Downloading firmware...", false, false);

    // --- Step 7: Download and flash firmware ---
    OTAError result = engine.downloadAndFlash(*client, binUrl, token, sigBuf, sigLen);

    if (result != OTAError::None) {
        lastOtaError_ = engine.getLastErrorString();
        ESP_LOGE(TAG, "OTA download failed: %s", lastOtaError_);
        publishProgress(0.0f, lastOtaError_, true, false);
        state_ = OTAState::Error;
        esp_task_wdt_delete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Firmware downloaded and verified successfully");

    // --- Step 8: Finalize ---
    state_ = OTAState::Complete;
    publishProgress(1.0f, "Update complete — rebooting...", true, true);
    ESP_LOGI(TAG, "OTA update complete — rebooting in 2 seconds");

    esp_task_wdt_delete(NULL);  // Unsubscribe before reboot
    delay(2000);  // Allow event propagation and display update
    ESP.restart();
#else
    // Native builds: no-op (should not be called)
    ESP_LOGW(TAG, "performUpdateTask() called on native build — no-op");
    state_ = OTAState::Error;
    lastOtaError_ = "Cannot perform OTA on native build";
#endif
}

/**
 * @brief Publish an OTA progress event to the EventBus.
 *
 * Creates an OTAProgressEvent on the stack and dispatches it via the
 * Application's EventBus on the "ota.progress" topic. The struct is
 * layout-compatible with OTAProgressPayload in u8g2DisplayModule.
 */
void OTAUpdateManager::publishProgress(float progress, const char* status,
                                        bool complete, bool success) {
    OTAProgressEvent event;
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
OTAState OTAUpdateManager::getState() const {
    return state_;
}

/**
 * @brief Get the last OTA error string.
 */
const char* OTAUpdateManager::getLastOtaError() const {
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
bool OTAUpdateManager::beginUpdate(size_t size, const char* expectedMD5) {
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
size_t OTAUpdateManager::writeStream(Stream& dataStream) {
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
bool OTAUpdateManager::endUpdate() {
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
void OTAUpdateManager::markAppValid() {
    ESP_LOGI(TAG, "Marking firmware as valid (blessing app)");
#ifndef NATIVE_BUILD
    checkSecurityVersion();
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    esp_ota_mark_app_valid_cancel_rollback();
#endif
    // Snapshot config manifest: Active → Golden after firmware blessing
    if (app_ != nullptr) {
        app_->getConfigManager().getManifest().snapshotActiveToGolden();
        app_->getConfigManager().getManifest().save();
        ESP_LOGI(TAG, "Config manifest: Active → Golden snapshot saved (firmware blessed)");
    }
#endif
}

/**
 * @brief Check and update the NVS security version counter.
 *
 * Compares the compile-time SECURITY_VERSION macro against the value
 * stored in NVS under the "ota_security" namespace. If the compiled
 * version is higher, it updates the stored value. This provides anti-
 * rollback protection at the application layer.
 */
void OTAUpdateManager::checkSecurityVersion() {
#ifndef NATIVE_BUILD
    Preferences prefs;
    prefs.begin("ota_security", false);
    uint32_t stored = prefs.getUInt("sec_ver", 0);

    if (SECURITY_VERSION > stored) {
        prefs.putUInt("sec_ver", SECURITY_VERSION);
        ESP_LOGI(TAG, "Security version updated: %u → %u", stored, (uint32_t)SECURITY_VERSION);
    } else {
        ESP_LOGD(TAG, "Security version OK: stored=%u compiled=%u", stored, (uint32_t)SECURITY_VERSION);
    }
    prefs.end();
#endif
}

/**
 * @brief Triggers a rollback to the previous firmware partition and reboots.
 */
void OTAUpdateManager::rollbackFirmware() {
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
void OTAUpdateManager::registerRoutes(AsyncWebServer& server) {
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
                        case OTAState::Idle:        stateStr = "idle"; break;
                        case OTAState::Checking:    stateStr = "checking"; break;
                        case OTAState::Downloading: stateStr = "downloading"; break;
                        case OTAState::Verifying:   stateStr = "verifying"; break;
                        case OTAState::Complete:    stateStr = "complete"; break;
                        case OTAState::Error:       stateStr = "error"; break;
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
