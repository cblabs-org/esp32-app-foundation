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
 *   - handleSignedUpload():  Multipart upload handler with ECDSA verification
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
 * Version 3: adds "checkHour" for daily scheduled updates.
 * Increment this when a future firmware change requires a config migration.
 */
int OTAUpdateManager::getSchemaVersion() const {
    return 3;
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
    _checkHour = section["checkHour"] | (uint8_t)3;
    if (_checkHour > 23) _checkHour = 3; // Clamp invalid values
    ESP_LOGI(TAG, "Config applied — channel='%s' autoCheck=%d checkHour=%u lastCheckedChannel='%s'",
             _channel, (int)_autoCheck, (unsigned)_checkHour, _lastCheckedChannel);
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
    section["checkHour"]          = _checkHour;
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
    if (fromVersion < 3) {
        section["checkHour"] = 3;
        ESP_LOGI(TAG, "Migrated OTA config v%d → v3: added checkHour=3", fromVersion);
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
    _checkHour = 3;
    ESP_LOGI(TAG, "Config reset to defaults — channel='stable' autoCheck=true checkHour=3 lastCheckedChannel=''");
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
 * @brief Periodic tick — handles boot auto-check and daily scheduled check.
 *
 * Called by the Application main loop. Must complete in ≤10ms; all heavy
 * work is deferred to the FreeRTOS task launched by performUpdate().
 *
 * Boot check: fires once on the first tick after entering Running state
 *   (which guarantees Wi-Fi is connected) if autoCheck is enabled.
 *
 * Daily check: fires once per calendar day when the wall-clock hour
 *   matches _checkHour and autoCheck is enabled. Requires NTP sync
 *   (time() > 1 000 000).
 */
void OTAUpdateManager::tick() {
    if (!_autoCheck || state_ != OTAState::Idle) return;

    // --- Boot check: fire once on first tick (Wi-Fi confirmed connected) ---
    if (!_hasBootChecked) {
        _hasBootChecked = true;
        _silentCheck = true;  // Boot checks never show error modals
        ESP_LOGI(TAG, "Auto-check: boot trigger");
        performUpdate();
        return;
    }

#ifndef NATIVE_BUILD
    // --- Daily scheduled check at _checkHour ---
    time_t now = time(nullptr);
    if (now < 1000000UL) return; // NTP not yet synced

    struct tm t;
    localtime_r(&now, &t);
    uint32_t today = static_cast<uint32_t>(now / 86400);

    if (static_cast<uint8_t>(t.tm_hour) == _checkHour && today > _lastCheckDay) {
        _lastCheckDay = today;
        _silentCheck = true;  // Scheduled checks never show error modals
        ESP_LOGI(TAG, "Auto-check: daily trigger at hour %u", (unsigned)_checkHour);
        performUpdate();
    }
#endif
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
    ESP_LOGI(TAG, "Launching OTA update pipeline");

#ifndef NATIVE_BUILD
    // Launch the download in a FreeRTOS task to avoid blocking tick()
    BaseType_t result = xTaskCreate(
        otaTaskWrapper,     // Task function
        "ota_update",       // Task name
        16384 * 2,          // Stack size (bytes) — TLS needs 12-16KB minimum + headroom
        this,               // Parameter — pointer to this instance
        5,                  // Priority (mid — needs timely TLS handshake)
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
    // Note: Do NOT subscribe this task to TWDT — TLS handshakes to
    // api.github.com take 10+ seconds, exceeding the default 5-second
    // WDT timeout. The OTA pipeline is inherently long-running.

    const FrameworkConfig& cfg = app_->getConfig();

    // --- Guard: firmware updates must be enabled ---
    if (!cfg.firmwareUpdatesEnabled) {
        ESP_LOGW(TAG, "Firmware updates are disabled in config");
        publishProgress(0.0f, "Firmware updates disabled", true, false);
        state_ = OTAState::Error;
        lastOtaError_ = "Firmware updates disabled";
        return;
    }

    // --- Guard: GitHub repo must be configured ---
    if (cfg.otaGithubRepo == nullptr) {
        ESP_LOGE(TAG, "No otaGithubRepo configured");
        publishProgress(0.0f, "No GitHub repo configured", true, false);
        state_ = OTAState::Error;
        lastOtaError_ = "No GitHub repo configured";
        return;
    }

    // --- Step 1: Check GitHub for releases ---
    GithubClient github(cfg.otaGithubRepo,
                        cfg.otaGithubToken ? cfg.otaGithubToken : "");
    ESP_LOGI(TAG, "Querying GitHub for latest release of %s", cfg.otaGithubRepo);

    if (!github.getLatestReleaseForChannel(_channel)) {
        ESP_LOGE(TAG, "GitHub release query failed: %s", github.getLastError());
        // Only show the error modal for manual checks — auto-checks (boot/daily)
        // run while network may still be settling and a transient failure
        // should never pop up a "Firmware Update Failed" modal unprompted.
        if (!_silentCheck) {
            publishProgress(0.0f, "GitHub query failed", true, false);
        }
        state_ = OTAState::Idle;  // Return to Idle, not Error, on silent path
        lastOtaError_ = _silentCheck ? nullptr : github.getLastError();
        _silentCheck = false;
        return;
    }

    ESP_LOGI(TAG, "Found release: %s with %d assets",
             github.releaseId.c_str(), github.releaseAssets);

    // --- Version guard: only proceed if remote > local ---
    FirmwareVersion local = { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, Channel::STABLE, 0 };
    FirmwareVersion remote = parseVersionTag(github.releaseId.c_str());

    if (!(remote > local)) {
        ESP_LOGI(TAG, "No update needed — local %d.%d.%d is current or newer than remote %s",
                 VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, github.releaseId.c_str());
        // No publishProgress — don't show the OTA screen for a no-op check
        state_ = OTAState::Idle;
        return;
    }

    ESP_LOGI(TAG, "Update available: %s > %d.%d.%d — proceeding with download",
             github.releaseId.c_str(), VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

    // Only now show the OTA screen — a download will actually happen
    publishProgress(0.0f, "Update found — preparing download...", false, false);

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
            return;
        }

        // Find the asset matching this board
        const OTAAsset* boardAsset = findAssetForBoard(manifest, BOARD_ID);
        if (boardAsset == nullptr) {
            ESP_LOGE(TAG, "No firmware for board '%s' in manifest", BOARD_ID);
            publishProgress(0.0f, "No firmware for this board in manifest", true, false);
            state_ = OTAState::Error;
            lastOtaError_ = "No firmware for this board in manifest";
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
        return;
    }

    if (sigUrl.isEmpty() && cfg.otaPublicKey != nullptr) {
        ESP_LOGE(TAG, "SECURITY ABORT: No .sig asset found but otaPublicKey is configured — refusing unsigned firmware");
        publishProgress(0.0f, "Missing signature — update rejected", true, false);
        state_ = OTAState::Error;
        lastOtaError_ = "Missing .sig file — update rejected (fail-closed)";
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
    // Download progress maps to 2%-100% range (0%-2% covers API/manifest/sig phases)
    engine.setProgressCallback([this](size_t bytesWritten, size_t totalSize) {
        float rawProgress = (totalSize > 0)
            ? static_cast<float>(bytesWritten) / static_cast<float>(totalSize)
            : 0.0f;
        float progress = 0.02f + (rawProgress * 0.98f); // 2% .. 100%
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
            return;
        } else if (sigLen > 0) {
            ESP_LOGI(TAG, "Fetched signature: %u bytes", (unsigned)sigLen);
        }
    }

    publishProgress(0.02f, "Downloading firmware...", false, false);

    // --- Step 7: Download and flash firmware ---
    OTAError result = engine.downloadAndFlash(*client, binUrl, token, sigBuf, sigLen);

    if (result != OTAError::None) {
        lastOtaError_ = engine.getLastErrorString();
        ESP_LOGE(TAG, "OTA download failed: %s", lastOtaError_);
        publishProgress(0.0f, lastOtaError_, true, false);
        state_ = OTAState::Error;
        return;
    }

    ESP_LOGI(TAG, "Firmware downloaded and verified successfully");

    // --- Step 8: Finalize ---
    state_ = OTAState::Complete;
    publishProgress(1.0f, "Update complete — rebooting...", true, true);
    ESP_LOGI(TAG, "OTA update complete — rebooting in 2 seconds");

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
    ESP_LOGI(TAG, "Beginning OTA update: size=%u, MD5=%s",
             size, expectedMD5 ? expectedMD5 : "(none)");
#ifndef NATIVE_BUILD
    // --- Step 1: Initialize the Update library ---
    if (!Update.begin(size)) {
        ESP_LOGE(TAG, "Update.begin() failed for size %u", (unsigned)size);
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
    ESP_LOGD(TAG, "writeStream: %u bytes written", (unsigned)written);
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
// ---------------------------------------------------------------------------
// Signed Web Upload — POST /api/ota/upload (multipart: firmware + signature)
// ---------------------------------------------------------------------------

/**
 * @brief Per-chunk multipart handler for signed firmware uploads.
 *
 * ESPAsyncWebServer invokes this callback once per chunk for each multipart
 * field. The handler distinguishes between the "firmware" field (streamed
 * directly into the Update partition with verifier accumulation) and the
 * "signature" field (collected into uploadSigBuf_).
 *
 * Security contract (fail-closed):
 *   - If otaPublicKey is configured and no signature bytes arrive → 400.
 *   - If verify() returns false → 403, Update.abort() called.
 *   - If the binary header magic (0xE9) is wrong → 400, Update.abort().
 *
 * State is reset at index == 0 of the "firmware" field so that the handler
 * is safe across repeated upload attempts without a server restart.
 *
 * @param request  Incoming HTTP request context (used only for error responses).
 * @param filename Multipart field name ("firmware" or "signature").
 * @param index    Byte offset of this chunk within the field (0 = first chunk).
 * @param data     Pointer to this chunk's bytes.
 * @param len      Number of bytes in this chunk.
 * @param final    True when this is the last chunk for this field.
 */
void OTAUpdateManager::handleSignedUpload(AsyncWebServerRequest* request,
                                          const String& filename,
                                          size_t index, uint8_t* data,
                                          size_t len, bool final) {
#ifndef NATIVE_BUILD
    if (app_ == nullptr) {
        ESP_LOGE(TAG, "Signed upload: app_ is null — module not initialized");
        request->send(500, "application/json", "{\"status\":\"error\",\"msg\":\"OTA module not initialized\"}");
        return;
    }
    const FrameworkConfig& cfg = app_->getConfig();

    // ---- Signature field: collect bytes into uploadSigBuf_ ----
    if (filename == "signature") {
        if (index == 0) {
            uploadSigLen_ = 0; // Reset on first chunk
        }
        size_t space = sizeof(uploadSigBuf_) - uploadSigLen_;
        size_t toCopy = (len < space) ? len : space;
        memcpy(uploadSigBuf_ + uploadSigLen_, data, toCopy);
        uploadSigLen_ += toCopy;
        return;
    }

    // ---- Firmware field ----
    if (filename != "firmware") return; // Unknown field — ignore

    if (uploadAborted_) return; // Already failed — drain remaining chunks silently

    // --- First chunk: reset state and initialise verifier ---
    if (index == 0) {
        uploadAborted_  = false;
        uploadBinSize_  = 0;

        // Clean up any previous verifier
        delete uploadVerifier_;
        uploadVerifier_ = nullptr;

        // Initialise MbedTLS verifier if a public key is configured
        if (cfg.otaPublicKey != nullptr) {
            uploadVerifier_ = new (std::nothrow) MbedTLSSignatureVerifier();
            if (!uploadVerifier_ || !uploadVerifier_->begin(cfg.otaPublicKey)) {
                ESP_LOGE(TAG, "Signed upload: verifier init failed");
                delete uploadVerifier_;
                uploadVerifier_ = nullptr;
                uploadAborted_  = true;
                request->send(500, "application/json",
                              "{\"status\":\"error\",\"msg\":\"Verifier init failed\"}");
                return;
            }
        }

        // Validate ESP32 binary magic byte
        if (len > 0 && data[0] != 0xE9) {
            ESP_LOGE(TAG, "Signed upload: invalid binary header 0x%02X", data[0]);
            uploadAborted_ = true;
            request->send(400, "application/json",
                          "{\"status\":\"error\",\"msg\":\"Invalid firmware header — not an ESP32 binary\"}");
            return;
        }

        // Begin the flash update — size is unknown at this point so use OTA_SIZE_UNKNOWN
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            ESP_LOGE(TAG, "Signed upload: Update.begin() failed");
            uploadAborted_ = true;
            request->send(500, "application/json",
                          "{\"status\":\"error\",\"msg\":\"Update begin failed — insufficient space?\"}");
            return;
        }

        ESP_LOGI(TAG, "Signed upload: started");
    }

    // --- Accumulate firmware bytes into verifier and flash ---
    if (uploadVerifier_ != nullptr) {
        uploadVerifier_->update(data, len);
    }

    if (Update.write(data, len) != len) {
        ESP_LOGE(TAG, "Signed upload: Update.write() failed at offset %u",
                 (unsigned)uploadBinSize_);
        Update.abort();
        uploadAborted_ = true;
        delete uploadVerifier_;
        uploadVerifier_ = nullptr;
        request->send(500, "application/json",
                      "{\"status\":\"error\",\"msg\":\"Flash write failed\"}");
        return;
    }

    uploadBinSize_ += len;

    // --- Final chunk: verify signature then finalise ---
    if (final) {
        // Fail-closed: public key configured → signature is mandatory
        if (cfg.otaPublicKey != nullptr) {
            if (uploadSigLen_ == 0) {
                ESP_LOGE(TAG, "Signed upload: SECURITY ABORT — no signature provided");
                Update.abort();
                delete uploadVerifier_;
                uploadVerifier_ = nullptr;
                uploadAborted_  = true;
                request->send(400, "application/json",
                              "{\"status\":\"error\",\"msg\":\"Signature file required — upload both .bin and .sig\"}");
                return;
            }

            if (uploadVerifier_ == nullptr ||
                !uploadVerifier_->verify(uploadSigBuf_, uploadSigLen_)) {
                ESP_LOGE(TAG, "Signed upload: ECDSA verification FAILED");
                Update.abort();
                delete uploadVerifier_;
                uploadVerifier_ = nullptr;
                uploadAborted_  = true;
                request->send(403, "application/json",
                              "{\"status\":\"error\",\"msg\":\"Signature verification failed — firmware rejected\"}");
                return;
            }

            ESP_LOGI(TAG, "Signed upload: ECDSA verification PASSED (%u sig bytes, %u firmware bytes)",
                     (unsigned)uploadSigLen_, (unsigned)uploadBinSize_);
        } else {
            ESP_LOGW(TAG, "Signed upload: no public key configured — signature check SKIPPED");
        }

        delete uploadVerifier_;
        uploadVerifier_ = nullptr;

        if (!Update.end(true /* set boot partition */)) {
            ESP_LOGE(TAG, "Signed upload: Update.end() failed");
            uploadAborted_ = true;
            request->send(500, "application/json",
                          "{\"status\":\"error\",\"msg\":\"Update finalisation failed\"}");
            return;
        }

        ESP_LOGI(TAG, "Signed upload: complete — rebooting in 500ms");
        request->send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"Firmware verified and flashed — rebooting\"}");
        delay(500);
        ESP.restart();
    }
#else
    (void)request; (void)filename; (void)index; (void)data; (void)len; (void)final;
#endif
}

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
                    const char* confirm = doc["confirm"] | "";
                    if (strcmp(confirm, "ROLLBACK") != 0) {
                        request->send(403, "application/json",
                                      "{\"status\":\"error\",\"msg\":\"Missing or invalid confirmation token\"}");
                        return;
                    }
                    request->send(200, "application/json", "{\"status\":\"ok\"}");
                    this->rollbackFirmware();
                    return;
                } else if (strcmp(action, "markValid") == 0) {
                    this->markAppValid();
                    request->send(200, "application/json", "{\"status\":\"ok\"}");
                    return;
                } else if (strcmp(action, "checkGithub") == 0) {
                    ESP_LOGI(TAG, "GitHub OTA check triggered via API");
                    // Reset any previous error state — explicit user action
                    if (this->state_ == OTAState::Error) {
                        this->state_ = OTAState::Idle;
                        this->lastOtaError_ = nullptr;
                    }
                    // Manual check: always show errors (not a silent auto-check)
                    this->_silentCheck = false;
                    // Respond immediately — the full check+download runs
                    // in a background FreeRTOS task. checkForUpdate() blocks
                    // for 10+ seconds (TLS to api.github.com) and must NOT
                    // run on the AsyncWebServer request thread.
                    this->performUpdate();
                    request->send(200, "application/json",
                                  "{\"status\":\"ok\",\"msg\":\"OTA check started\"}");
                    return;
                } else if (strcmp(action, "setChannel") == 0) {
                    const char* ch = doc["channel"] | "";
                    if (strlen(ch) > 0 && strlen(ch) < sizeof(this->_channel)) {
                        strlcpy(this->_channel, ch, sizeof(this->_channel));
                        // Persist via ConfigManager
                        if (this->app_ != nullptr) {
                            this->app_->getConfigManager().saveModuleConfig(this);
                        }
                        ESP_LOGI(TAG, "Channel changed to: %s", this->_channel);
                        request->send(200, "application/json", "{\"status\":\"ok\"}");
                    } else {
                        request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid channel\"}");
                    }
                    return;
                } else if (strcmp(action, "setAutoCheck") == 0) {
                    if (doc["autoCheck"].is<bool>()) {
                        this->_autoCheck = doc["autoCheck"].as<bool>();
                        // Persist via ConfigManager
                        if (this->app_ != nullptr) {
                            this->app_->getConfigManager().saveModuleConfig(this);
                        }
                        ESP_LOGI(TAG, "Auto-check changed to: %d", (int)this->_autoCheck);
                        request->send(200, "application/json", "{\"status\":\"ok\"}");
                    } else {
                        request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Missing autoCheck boolean\"}");
                    }
                    return;
                } else if (strcmp(action, "setCheckHour") == 0) {
                    if (doc["checkHour"].is<int>()) {
                        int h = doc["checkHour"].as<int>();
                        if (h >= 0 && h <= 23) {
                            this->_checkHour = static_cast<uint8_t>(h);
                            if (this->app_ != nullptr) {
                                this->app_->getConfigManager().saveModuleConfig(this);
                            }
                            ESP_LOGI(TAG, "Check hour changed to: %u", (unsigned)this->_checkHour);
                            request->send(200, "application/json", "{\"status\":\"ok\"}");
                        } else {
                            request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"checkHour must be 0-23\"}");
                        }
                    } else {
                        request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Missing checkHour integer\"}");
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
                    resp["state"]     = stateStr;
                    resp["version"]   = String(VERSION_MAJOR) + "." + String(VERSION_MINOR) + "." + String(VERSION_PATCH);
                    resp["channel"]   = this->_channel;
                    resp["boardId"]   = BOARD_ID;
                    resp["autoCheck"] = this->_autoCheck;
                    resp["checkHour"] = this->_checkHour;
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

    // --- POST /api/ota/upload — signed multipart firmware upload ---
    // Accepts multipart/form-data with fields:
    //   "firmware"  — the compiled .bin binary
    //   "signature" — the detached ECDSA .sig file
    // Uses ArUploadHandlerFunction (4th arg) for multipart file data.
    // The handler accumulates firmware bytes through MbedTLSSignatureVerifier
    // and rejects the flash if the ECDSA signature does not verify.
    server.on("/api/ota/upload", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            // Completion handler — response already sent from handleSignedUpload().
        },
        [this](AsyncWebServerRequest* request, const String& filename,
               size_t index, uint8_t* data, size_t len, bool final) {
            this->handleSignedUpload(request, filename, index, data, len, final);
        }
    );
#endif
}
