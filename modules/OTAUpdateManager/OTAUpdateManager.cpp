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
 *   - registerMCPTools():     Register ota_get_status, ota_check_now, ota_trigger_update MCP tools
 *   - registerRoutes():     Register REST API route handlers
 *   - getConfigNamespace(): Return "ota" (IConfigurable)
 *   - getSchemaVersion():   Return 4 (IConfigurable)
 *   - applyConfig():        Apply OTA config from JSON section (IConfigurable)
 *   - serializeConfig():    Serialize OTA config to JSON section (IConfigurable)
 *   - migrateConfig():      Migrate from older schema version (IConfigurable)
 *   - resetConfig():        Restore OTA config factory defaults (IConfigurable)
 */

#include "OTAUpdateManager.hpp"
#include "platform/LogShim.h"

static constexpr const char* TAG = "OTAUpdateManager";

// ---------------------------------------------------------------------------
// Platform-conditional includes
// Hardware headers are excluded from the native GoogleTest build.
// Module includes go in the .cpp ONLY (behind NATIVE_BUILD guard) to
// respect the Hollywood Principle — no cross-module includes in headers.
// ---------------------------------------------------------------------------
#ifndef NATIVE_BUILD
#include <PsychicHttp.h>
#include <PsychicUploadHandler.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include "esp_task_wdt.h"
#include <inttypes.h>
#include <freertos/idf_additions.h>
#include "../Application/Application.hpp"
#include "../../lib/GithubClient/GithubClient.hpp"
#include "../../lib/GithubClient/ota_root_cas.h"
#include "../../lib/OTADownloadEngine/OTADownloadEngine.hpp"
#include "../../lib/OTADownloadEngine/MbedTLSSignatureVerifier.hpp"
#include "../../lib/SemVer/SemVer.hpp"
#include "../../lib/OTAManifest/OTAManifest.hpp"
#include "../../modules/MCPServer/MCPToolRegistry.hpp"
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
 * Version 4: adds "updatesEnabled" (master kill-switch for applying updates).
 * Increment this when a future firmware change requires a config migration.
 */
int OTAUpdateManager::getSchemaVersion() const {
    return 4;
}

/**
 * @brief Apply OTA configuration values from a parsed JSON section.
 *
 * Reads "channel", "autoCheck", "lastCheckedChannel", and "updatesEnabled"
 * from the section. Missing keys fall back to safe defaults via ArduinoJson's
 * | operator.
 *
 * @param section Read-only reference to the JSON object containing config keys.
 */
void OTAUpdateManager::applyConfig(const JsonObject& section) {
    const char* ch = section["channel"] | "stable";
    strlcpy(_channel, ch, sizeof(_channel));
    _autoCheck      = section["autoCheck"]      | true;
    _updatesEnabled = section["updatesEnabled"] | true;
    const char* lcc = section["lastCheckedChannel"] | "";
    strlcpy(_lastCheckedChannel, lcc, sizeof(_lastCheckedChannel));
    _checkHour = section["checkHour"] | (uint8_t)3;
    if (_checkHour > 23) _checkHour = 3; // Clamp invalid values
    ESP_LOGI(TAG, "Config applied — channel='%s' autoCheck=%d updatesEnabled=%d checkHour=%u lastCheckedChannel='%s'",
             _channel, (int)_autoCheck, (int)_updatesEnabled, (unsigned)_checkHour, _lastCheckedChannel);
}

/**
 * @brief Serialize current OTA configuration into a JSON section.
 *
 * Writes "channel", "autoCheck", "lastCheckedChannel", and "updatesEnabled"
 * into the provided section for persistence by ConfigManager
 * (stored at /mod/ota.json).
 *
 * @param section Mutable reference to the target JSON object.
 */
void OTAUpdateManager::serializeConfig(JsonObject& section) const {
    section["channel"]            = _channel;
    section["autoCheck"]          = _autoCheck;
    section["updatesEnabled"]     = _updatesEnabled;
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
    if (fromVersion < 4) {
        section["updatesEnabled"] = true;
        ESP_LOGI(TAG, "Migrated OTA config v%d → v4: added updatesEnabled=true", fromVersion);
    }
    return true;
}

/**
 * @brief Reset OTA configuration to factory defaults.
 *
 * Sets channel to "stable", autoCheck to true, updatesEnabled to true, and
 * clears lastCheckedChannel. ConfigManager calls saveModuleConfig() afterwards
 * to persist the reset state.
 */
void OTAUpdateManager::resetConfig() {
    strlcpy(_channel, "stable", sizeof(_channel));
    _autoCheck      = true;
    _updatesEnabled = true;
    _lastCheckedChannel[0] = '\0';
    _checkHour = 3;
    ESP_LOGI(TAG, "Config reset to defaults — channel='stable' autoCheck=true updatesEnabled=true checkHour=3 lastCheckedChannel=''");
}



/**
 * @brief Initialize the OTA manager with dependencies from the Application.
 *
 * Extracts narrow references to the 3 services this module actually uses,
 * then discards the Application reference (Transient Service Locator, ADR-39).
 */
void OTAUpdateManager::begin(Application& app) {
    _config        = &app.getConfig();
    _configManager = &app.getConfigManager();
    _eventBus      = &app.getEventBus();
    ESP_LOGI(TAG, "OTA manager initialized (narrow refs extracted)");
}

/**
 * @brief Periodic tick — handles boot auto-check and daily scheduled check.
 *
 * Called by the Application main loop. Must complete in ≤10ms; all heavy
 * work is deferred to the FreeRTOS task launched by performUpdate().
 *
 * Boot check: fires once on the first tick after Wi-Fi is confirmed
 *   connected (WiFi.status() == WL_CONNECTED) if autoCheck is enabled.
 *
 * Daily check: fires once per calendar day when the wall-clock hour
 *   matches _checkHour and autoCheck is enabled. Requires NTP sync
 *   (time() > 1 000 000) and an active Wi-Fi connection.
 */
void OTAUpdateManager::tick(uint32_t currentMillis) {
    if (!_autoCheck || _state != OTAState::Idle) return;

#ifndef NATIVE_BUILD
    // Gate: do not attempt any network operation until Wi-Fi is confirmed up.
    // This prevents the boot check from racing the TLS/DNS stack during the
    // first few seconds after association, and prevents the daily check from
    // firing during a temporary network outage.
    if (WiFi.status() != WL_CONNECTED) return;
#endif

    // --- Boot check: fire once, after Wi-Fi is confirmed connected ---
    if (!_hasBootChecked) {
        _hasBootChecked = true;
        _silentCheck = true;  // Boot checks never show error modals
        ESP_LOGI(TAG, "Auto-check: boot trigger (Wi-Fi confirmed connected)");
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
    if (_config == nullptr) {
        ESP_LOGE(TAG, "checkForUpdate() called before begin() — returning false");
        return false;
    }

    // Guard: must be in Idle state
    if (_state != OTAState::Idle) {
        ESP_LOGW(TAG, "checkForUpdate() called while state != Idle — returning false");
        return false;
    }

    // Guard: must have a configured GitHub repo
    if (_config->otaGithubRepo == nullptr) {
        ESP_LOGW(TAG, "checkForUpdate() — no otaGithubRepo configured — returning false");
        return false;
    }

    _state = OTAState::Checking;
    _lastCheckMillis = millis();
    ESP_LOGI(TAG, "Checking for updates — local: %s, channel: '%s'", BUILD_VERSION, _channel);

#ifndef NATIVE_BUILD
    // Create GithubClient with repo and optional token from config
    const FrameworkConfig& cfg = *_config;
    GithubClient github(cfg.otaGithubRepo,
                        cfg.otaGithubToken ? cfg.otaGithubToken : "");

    bool success = github.getLatestReleaseForChannel(_channel);
    if (!success) {
        ESP_LOGE(TAG, "GitHub release check failed: %s", github.getLastError());
        _state = OTAState::Idle;
        return false;
    }

    ESP_LOGI(TAG, "Best release for '%s' channel: %s (%d assets)",
             _channel, github.releaseId.c_str(), github.releaseAssets);

    // --- SemVer comparison: local build version vs. remote release tag ---
    FirmwareVersion local = { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
                              static_cast<Channel>(VERSION_CHANNEL), VERSION_PRE_NUM };
    FirmwareVersion remote = parseVersionTag(github.releaseId.c_str());

    // Normal case: remote > local → update available
    if (remote > local) {
        ESP_LOGI(TAG, "Update available: remote version is newer");
        strlcpy(_lastCheckedChannel, _channel, sizeof(_lastCheckedChannel));
        _state = OTAState::Idle;
        return true;
    }

    // Implicit downgrade: channel changed since last check
    // e.g., switched from beta to stable — allow "downgrade" to the latest stable
    if (strcmp(_channel, _lastCheckedChannel) != 0 && _lastCheckedChannel[0] != '\0') {
        ESP_LOGI(TAG, "Channel changed ('%s' → '%s') — allowing implicit version change",
                 _lastCheckedChannel, _channel);
        strlcpy(_lastCheckedChannel, _channel, sizeof(_lastCheckedChannel));
        // Save the channel change to config
        if (_configManager != nullptr) {
            _configManager->saveModuleConfig(this);
        }
        _state = OTAState::Idle;
        return true;  // Signal update available even if remote <= local
    }

    // Update lastCheckedChannel
    strlcpy(_lastCheckedChannel, _channel, sizeof(_lastCheckedChannel));

    ESP_LOGI(TAG, "No update available — local %s is current or newer than remote %s (channel='%s')",
             BUILD_VERSION, github.releaseId.c_str(), _channel);
    _state = OTAState::Idle;
    return false;
#else
    // Native builds cannot perform real GitHub checks
    _state = OTAState::Idle;
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
    if (_config == nullptr) {
        ESP_LOGE(TAG, "performUpdate() called before begin()");
        _state = OTAState::Error;
        _lastOtaError = "OTA manager not initialized";
        return;
    }

    // Guard: must be in Idle state
    if (_state != OTAState::Idle) {
        ESP_LOGW(TAG, "performUpdate() called while state != Idle — ignoring");
        return;
    }

    _state = OTAState::Checking;
    ESP_LOGI(TAG, "Launching OTA update pipeline");

#ifndef NATIVE_BUILD
    // --- Pre-flight: DRAM headroom check ---
    uint32_t freeDram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    uint32_t required = OTA_TASK_STACK_BYTES + OTA_TASK_DRAM_HEADROOM_BYTES;
    if (freeDram < required) {
        ESP_LOGW(TAG,
            "Low DRAM before OTA task launch — free: %" PRIu32 " B, "
            "required: %" PRIu32 " B. Attempting PSRAM stack allocation.",
            freeDram, required);
    }

    // --- Launch OTA task — stack MUST be in DRAM (not PSRAM) ---
    // The OTA pipeline performs SPI flash writes (Update.begin/write/end,
    // ConfigManifest::save, NVS operations). ESP-IDF asserts when flash ops
    // run on a PSRAM-stack task because disabling the SPI cache makes the
    // PSRAM-resident stack inaccessible. 32 KiB DRAM is feasible given the
    // device's ~240 KiB free internal RAM at this point.
    BaseType_t result = xTaskCreateWithCaps(
        otaTaskWrapper,                            // Task function
        "ota_update",                              // Task name
        OTA_TASK_STACK_BYTES,                      // Stack size (bytes)
        this,                                      // Parameter
        5,                                         // Priority
        &_taskHandle,                              // Task handle output
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT      // DRAM stack (SPI flash safe)
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG,
            "Failed to create OTA FreeRTOS task — "
            "free DRAM: %" PRIu32 " B, stack required: %" PRIu32 " B. "
            "Device may need reboot to recover heap.",
            freeDram, OTA_TASK_STACK_BYTES);
        _state = OTAState::Error;
        _lastOtaError = "Failed to create OTA task (insufficient memory)";
        publishProgress(0.0f, "Failed to create OTA task (insufficient memory)", true, false);
    }
#else
    // Native builds cannot run FreeRTOS tasks
    ESP_LOGW(TAG, "performUpdate() — no FreeRTOS on native build, setting Error");
    _state = OTAState::Error;
    _lastOtaError = "FreeRTOS not available on native build";
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

    // NOTE: Do NOT register this task with TWDT here. The OTA pipeline makes
    // multiple blocking HTTPS calls (GitHub API, manifest, signature) involving
    // TLS handshakes and paginated responses that take 5-15+ seconds each.
    // TWDT is registered only during the firmware download phase (Step 7),
    // where the progress callback feeds it every ~4KB.

    const FrameworkConfig& cfg = *_config;

    // --- Guard: firmware updates must be enabled (persisted OTA config) ---
    if (!_updatesEnabled) {
        ESP_LOGW(TAG, "Firmware updates are disabled in config");
        publishProgress(0.0f, "Firmware updates disabled", true, false);
        _state = OTAState::Error;
        _lastOtaError = "Firmware updates disabled";
        return;
    }

    // --- Guard: GitHub repo must be configured ---
    if (cfg.otaGithubRepo == nullptr) {
        ESP_LOGE(TAG, "No otaGithubRepo configured");
        publishProgress(0.0f, "No GitHub repo configured", true, false);
        _state = OTAState::Error;
        _lastOtaError = "No GitHub repo configured";
        return;
    }

    // --- Step 1: Check GitHub for releases ---
    _lastCheckMillis = millis();
    GithubClient github(cfg.otaGithubRepo,
                        cfg.otaGithubToken ? cfg.otaGithubToken : "");
    ESP_LOGI(TAG, "Querying GitHub for latest release — repo: %s, channel: '%s', local: %s",
             cfg.otaGithubRepo, _channel, BUILD_VERSION);

    // Feed TWDT before blocking HTTPS call — TLS handshake to api.github.com
    // can take 3–5 seconds, which may exceed the default WDT timeout.
    if (!github.getLatestReleaseForChannel(_channel)) {
        ESP_LOGE(TAG, "GitHub release query failed: %s", github.getLastError());
        // Only show the error modal for manual checks — auto-checks (boot/daily)
        // run while network may be marginal; a transient failure must not pop
        // up a 'Firmware Update Failed' modal unprompted.
        if (!_silentCheck) {
            static char ghErrBuf[128];
            snprintf(ghErrBuf, sizeof(ghErrBuf), "GitHub query: %s", github.getLastError());
            _lastOtaError = ghErrBuf;
            publishProgress(0.0f, "GitHub query failed", true, false);
            _state = OTAState::Error;
        } else {
            _state = OTAState::Idle;
        }
        _silentCheck = false;
        return;
    }

    ESP_LOGI(TAG, "Best release for '%s' channel: %s (%d assets)",
             _channel, github.releaseId.c_str(), github.releaseAssets);

    // --- Step 1b: Find manifest.json in the release assets ---
    std::string manifestUrl;
    for (int i = 0; i < github.releaseAssets; i++) {
        if (strcmp(github.releaseAssetName[i].c_str(), "manifest.json") == 0) {
            manifestUrl = github.releaseAssetURL[i].c_str();
            ESP_LOGI(TAG, "Found manifest.json asset");
            break;
        }
    }

    // --- Step 1c: Resolve the authoritative version string ---
    // For floating tags (e.g. "latest-dev"), the tag_name is not a parseable
    // SemVer. Download the manifest.json to get the build-authoritative version.
    // For regular releases, the tag_name IS the version (e.g. "v0.0.11-dev.14").
    std::string resolvedVersion = github.releaseId;
    bool manifestParsed = false;
    OTAManifest manifest{};

    if (strncmp(github.releaseId.c_str(), "latest-", 7) == 0) {
        if (manifestUrl.empty()) {
            ESP_LOGE(TAG, "Floating tag '%s' has no manifest.json — cannot determine version",
                     github.releaseId.c_str());
            if (!_silentCheck) {
                publishProgress(0.0f, "No manifest in floating tag release", true, false);
                _state = OTAState::Error;
                _lastOtaError = "No manifest.json in floating tag release";
            } else {
                _state = OTAState::Idle;
            }
            _silentCheck = false;
            return;
        }

        // Download manifest.json to extract the authoritative version
        ESP_LOGI(TAG, "Floating tag detected — downloading manifest.json for version resolution");
        std::string manifestVersion = downloadAndParseManifestVersion_(
            manifestUrl, cfg, manifest);

        if (manifestVersion.empty()) {
            // downloadAndParseManifestVersion_ already set error state
            return;
        }

        resolvedVersion = manifestVersion;
        manifestParsed = true;
        ESP_LOGI(TAG, "Manifest version resolved: %s (from floating tag '%s')",
                 resolvedVersion.c_str(), github.releaseId.c_str());
    }

    // --- Version guard: only proceed if remote > local ---
    FirmwareVersion local = { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
                              static_cast<Channel>(VERSION_CHANNEL), VERSION_PRE_NUM };
    FirmwareVersion remote = parseVersionTag(resolvedVersion.c_str());

    if (!(remote > local)) {
        ESP_LOGI(TAG, "No update needed — local %s is current or newer than remote %s (channel='%s')",
                 BUILD_VERSION, resolvedVersion.c_str(), _channel);
        // No publishProgress — don't show the OTA screen for a no-op check
        _state = OTAState::Idle;
        return;
    }

    ESP_LOGI(TAG, "Update available: %s > %s (channel='%s') — proceeding with download",
             resolvedVersion.c_str(), BUILD_VERSION, _channel);

    // Update releaseId to the resolved version for downstream consumers
    // (e.g. setExpectedVersion, log messages, portal display)
    github.releaseId = resolvedVersion;

    // Only now transition to Downloading — a download will actually happen
    _state = OTAState::Downloading;
    publishProgress(0.0f, "Update found — preparing download...", false, false);

    // --- Step 2: Find firmware and signature assets (manifest-aware) ---
    std::string binUrl;
    std::string sigUrl;
    std::string binName;

    if (!manifestUrl.empty()) {
        // --- Manifest path: download and parse manifest.json ---

        if (!manifestParsed) {
            // Manifest was not already downloaded for version resolution (non-floating tag path)
            ESP_LOGI(TAG, "Downloading manifest.json for board '%s'", BOARD_ID);

            std::string manifestVersion = downloadAndParseManifestVersion_(
                manifestUrl, cfg, manifest);

            if (manifestVersion.empty()) {
                // downloadAndParseManifestVersion_ already set error state
                return;
            }
            manifestParsed = true;
        } else {
            ESP_LOGI(TAG, "Reusing manifest already parsed during version resolution");
        }

        // Find the asset matching this board
        const OTAAsset* boardAsset = findAssetForBoard(manifest, BOARD_ID);
        if (boardAsset == nullptr) {
            ESP_LOGE(TAG, "No firmware for board '%s' in manifest", BOARD_ID);
            publishProgress(0.0f, "No firmware for this board in manifest", true, false);
            _state = OTAState::Error;
            _lastOtaError = "No firmware for this board in manifest";
            return;
        }

        ESP_LOGI(TAG, "Manifest: found asset for board '%s' — firmware='%s' sig='%s'",
                 boardAsset->board, boardAsset->firmware, boardAsset->signature);

        // Match manifest filenames to release asset URLs
        for (int i = 0; i < github.releaseAssets; i++) {
            if (strcmp(github.releaseAssetName[i].c_str(), boardAsset->firmware) == 0) {
                binUrl = github.releaseAssetURL[i].c_str();
                binName = github.releaseAssetName[i].c_str();
                ESP_LOGI(TAG, "Matched firmware binary URL: %s", binName.c_str());
            }
            if (strcmp(github.releaseAssetName[i].c_str(), boardAsset->signature) == 0) {
                sigUrl = github.releaseAssetURL[i].c_str();
                ESP_LOGI(TAG, "Matched signature file URL: %s", github.releaseAssetName[i].c_str());
            }
        }
    } else {
        // --- Legacy fallback: no manifest.json found ---
        ESP_LOGW(TAG, "No manifest.json found — using legacy asset search");

        for (int i = 0; i < github.releaseAssets; i++) {
            const std::string name = github.releaseAssetName[i].c_str();
            int nameLen = (int)name.length();

            // Check for .bin extension
            if (nameLen > 4 &&
                name[nameLen - 4] == '.' &&
                name[nameLen - 3] == 'b' &&
                name[nameLen - 2] == 'i' &&
                name[nameLen - 1] == 'n') {
                binUrl = github.releaseAssetURL[i].c_str();
                binName = name;
                ESP_LOGI(TAG, "Found firmware binary: %s", name.c_str());
            }

            // Check for .sig extension
            if (nameLen > 4 &&
                name[nameLen - 4] == '.' &&
                name[nameLen - 3] == 's' &&
                name[nameLen - 2] == 'i' &&
                name[nameLen - 1] == 'g') {
                sigUrl = github.releaseAssetURL[i].c_str();
                ESP_LOGI(TAG, "Found signature file: %s", name.c_str());
            }
        }
    }

    if (binUrl.empty()) {
        ESP_LOGE(TAG, "No .bin asset found in release %s", github.releaseId.c_str());
        publishProgress(0.0f, "No firmware binary in release", true, false);
        _state = OTAState::Error;
        _lastOtaError = "No firmware binary found in release";
        return;
    }

    if (sigUrl.empty() && cfg.otaPublicKey != nullptr) {
        ESP_LOGE(TAG, "SECURITY ABORT: No .sig asset found but otaPublicKey is configured — refusing unsigned firmware");
        publishProgress(0.0f, "Missing signature — update rejected", true, false);
        _state = OTAState::Error;
        _lastOtaError = "Missing .sig file — update rejected (fail-closed)";
        return;
    }

    // --- Snapshot config manifest: Active → Previous before download ---
    _configManager->getManifest().snapshotActiveToPrevious();
    _configManager->getManifest().save();
    ESP_LOGI(TAG, "Config manifest: Active → Previous snapshot saved");

    // --- Step 3: Initialize the verifier (if public key configured) ---
    MbedTLSSignatureVerifier verifier;
    bool verifierReady = false;

    if (cfg.otaPublicKey != nullptr) {
        _state = OTAState::Verifying;
        if (!verifier.begin(cfg.otaPublicKey)) {
            ESP_LOGE(TAG, "Failed to initialize MbedTLS verifier with public key");
            publishProgress(0.0f, "Verifier initialization failed", true, false);
            _state = OTAState::Error;
            _lastOtaError = "Verifier initialization failed";
            return;
        }
        verifierReady = true;
        ESP_LOGI(TAG, "Verifier initialized with public key");
        _state = OTAState::Downloading;
    }

    // --- Step 4: Create WiFiClientSecure on the heap ---
    std::unique_ptr<WiFiClientSecure> client(new (std::nothrow) WiFiClientSecure());
    if (!client) {
        ESP_LOGE(TAG, "Failed to allocate WiFiClientSecure on heap");
        publishProgress(0.0f, "Memory allocation failed", true, false);
        _state = OTAState::Error;
        _lastOtaError = "WiFiClientSecure heap allocation failed";
        return;
    }
    // Binary download uses setInsecure() — the firmware payload is independently
    // verified via MbedTLSSignatureVerifier (RSA-2048 signature). Transport
    // security is redundant. GitHub CDN redirects use different certs than the API.
    client->setInsecure();
    ESP_LOGI(TAG, "WiFiClientSecure allocated on heap");

    // --- Step 5: Create OTADownloadEngine and configure ---
    OTADownloadEngine engine;
    if (verifierReady) {
        engine.setVerifier(&verifier);
    }

    // Set progress callback to forward to EventBus
    // Download progress maps to 2%-100% range (0%-2% covers API/manifest/sig phases)
    engine.setProgressCallback([](void* ctx, size_t bytesWritten, size_t totalSize) {
        auto* self = static_cast<OTAUpdateManager*>(ctx);
        float rawProgress = (totalSize > 0)
            ? static_cast<float>(bytesWritten) / static_cast<float>(totalSize)
            : 0.0f;
        float progress = 0.02f + (rawProgress * 0.98f); // 2% .. 100%
        self->publishProgress(progress, "Downloading firmware...", false, false);
        // Feed the Task Watchdog during firmware download — each progress
        // callback fires every ~4KB, keeping the WDT alive through multi-MB
        // transfers and TLS renegotiations.
        esp_task_wdt_reset();
    }, this);
    ESP_LOGI(TAG, "Download engine configured");

    // --- Step 6: Fetch signature file (if .sig URL available) ---
    uint8_t sigBuf[512];
    size_t sigLen = 0;
    std::string token = cfg.otaGithubToken ? cfg.otaGithubToken : "";

    if (!sigUrl.empty()) {
        sigLen = engine.fetchSignature(*client, sigUrl, token, sigBuf, sizeof(sigBuf));
        if (sigLen == 0 && cfg.otaPublicKey != nullptr) {
            ESP_LOGE(TAG, "SECURITY ABORT: Signature fetch returned 0 bytes — refusing update");
            publishProgress(0.0f, "Signature fetch failed — update rejected", true, false);
            _state = OTAState::Error;
            _lastOtaError = "Signature empty — update rejected (fail-closed)";
            return;
        } else if (sigLen > 0) {
            ESP_LOGI(TAG, "Fetched signature: %u bytes", (unsigned)sigLen);
        }
    }

    publishProgress(0.02f, "Downloading firmware...", false, false);

    // --- Step 7: Download and flash firmware ---
    // Set expected version for pre-flash binary validation safety gate.
    // github.releaseId contains the SemVer string (e.g. "v0.0.11-dev.14")
    // obtained from the release metadata. The engine strips the leading 'v'
    // and compares against the esp_app_desc_t.version field in the first
    // chunk of the downloaded binary before any flash writes occur.
    engine.setExpectedVersion(github.releaseId.c_str());

    // Register with TWDT only for the download phase — the progress callback
    // feeds the watchdog every ~4KB. All prior phases (GitHub API, manifest,
    // signature) involve unpredictable blocking I/O and must NOT be under TWDT.
    esp_task_wdt_add(NULL);
    OTAError result = engine.downloadAndFlash(*client, binUrl, token, sigBuf, sigLen);
    esp_task_wdt_delete(NULL);

    if (result != OTAError::None) {
        _lastOtaError = engine.getLastErrorString();
        ESP_LOGE(TAG, "OTA download failed: %s", _lastOtaError);
        publishProgress(0.0f, _lastOtaError, true, false);
        _state = OTAState::Error;
        return;
    }

    ESP_LOGI(TAG, "Firmware downloaded and verified successfully");

    // --- Step 8: Finalize ---
    _state = OTAState::Complete;
    publishProgress(1.0f, "Update complete — rebooting...", true, true);
    ESP_LOGI(TAG, "OTA update complete — rebooting in 2 seconds");

    // SAFETY: TWDT was deregistered at line ~761. This delay is safe only
    // because we are outside TWDT scope. Do not move TWDT deregistration
    // below this point.
    delay(2000);  // Allow event propagation and display update

    ESP_LOGI(TAG, "OTA task stack high-water mark: %u bytes free",
             static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr) * sizeof(StackType_t)));

    ESP.restart();
#else
    // Native builds: no-op (should not be called)
    ESP_LOGW(TAG, "performUpdateTask() called on native build — no-op");
    _state = OTAState::Error;
    _lastOtaError = "Cannot perform OTA on native build";
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
    _lastProgress = progress;
    OTAProgressEvent event;
    event.progress = progress;
    event.status   = status;
    event.complete = complete;
    event.success  = success;

#ifndef NATIVE_BUILD
    if (_eventBus != nullptr) {
        _eventBus->publish(OTAUpdateManagerEvents::OTA_PROGRESS, &event);
    }
#endif
    ESP_LOGD(TAG, "OTA progress: %.1f%% — %s (complete=%d, success=%d)",
             progress * 100.0f, status, complete, success);
}

/**
 * @brief Download manifest.json and parse it, returning the version string.
 *
 * Handles the full HTTPS download (CA-pinned → insecure fallback), buffered
 * read, and JSON deserialization. On success, populates the manifest struct
 * and returns the version string. On failure, sets OTA error state and
 * returns an empty string.
 */
std::string OTAUpdateManager::downloadAndParseManifestVersion_(
    const std::string& manifestUrl,
    const FrameworkConfig& cfg,
    OTAManifest& manifest) {

#ifndef NATIVE_BUILD
    std::unique_ptr<WiFiClientSecure> manifestClient(new (std::nothrow) WiFiClientSecure());
    if (!manifestClient) {
        ESP_LOGE(TAG, "Failed to allocate WiFiClientSecure for manifest fetch");
        publishProgress(0.0f, "Memory allocation failed", true, false);
        _state = OTAState::Error;
        _lastOtaError = "Manifest client allocation failed";
        return "";
    }
    // Try CA-pinned connection first (defense-in-depth)
    manifestClient->setCACert(OTA_ROOT_CA_BUNDLE);

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    std::string manifestToken = cfg.otaGithubToken ? cfg.otaGithubToken : "";

    if (!http.begin(*manifestClient, String(manifestUrl.c_str()))) {
        ESP_LOGE(TAG, "HTTPClient.begin() failed for manifest URL");
        publishProgress(0.0f, "Manifest download failed", true, false);
        _state = OTAState::Error;
        _lastOtaError = "Manifest HTTP begin failed";
        return "";
    }

    http.addHeader("Accept", "application/octet-stream");
    if (!manifestToken.empty()) {
        http.addHeader("Authorization", String((std::string("Bearer ") + manifestToken).c_str()));
    }

    int httpCode = http.GET();
    if (httpCode < 0) {
        ESP_LOGW(TAG, "Manifest fetch failed with pinned CA (err=%d) "
                      "— retrying without cert validation "
                      "(firmware signature is the trust anchor)", httpCode);
        http.end();

        // Feed TWDT before retry — TLS handshake can be slow
        esp_task_wdt_reset();

        // MUST create a fresh WiFiClientSecure — the failed handshake leaves
        // the mbedTLS context in a dirty state that cannot be reconfigured.
        manifestClient.reset(new (std::nothrow) WiFiClientSecure());
        if (!manifestClient) {
            ESP_LOGE(TAG, "Failed to allocate WiFiClientSecure for insecure retry");
            publishProgress(0.0f, "Memory allocation failed during retry", true, false);
            _state = OTAState::Error;
            _lastOtaError = "Manifest client retry allocation failed";
            return "";
        }
        manifestClient->setInsecure();

        if (!http.begin(*manifestClient, String(manifestUrl.c_str()))) {
            ESP_LOGE(TAG, "HTTPClient.begin() failed for manifest retry URL");
            publishProgress(0.0f, "Manifest download retry failed", true, false);
            _state = OTAState::Error;
            _lastOtaError = "Manifest HTTP retry begin failed";
            return "";
        }

        http.addHeader("Accept", "application/octet-stream");
        if (!manifestToken.empty()) {
            http.addHeader("Authorization", String((std::string("Bearer ") + manifestToken).c_str()));
        }

        httpCode = http.GET();
    }
    if (httpCode != 200) {
        ESP_LOGE(TAG, "Manifest download HTTP %d", httpCode);
        http.end();
        publishProgress(0.0f, "Manifest download failed", true, false);
        _state = OTAState::Error;
        _lastOtaError = "Manifest download HTTP error";
        return "";
    }

    // Static buffer — only one OTA pipeline runs at a time (enforced by _state guard).
    // Saves 2 KB of stack headroom on the 16 KB OTA task.
    static char manifestBuf[2048];
    int payloadLen = http.getSize();
    if (payloadLen <= 0 || payloadLen >= (int)sizeof(manifestBuf)) {
        ESP_LOGE(TAG, "Manifest payload size invalid: %d", payloadLen);
        http.end();
        publishProgress(0.0f, "Manifest too large or empty", true, false);
        _state = OTAState::Error;
        _lastOtaError = "Manifest payload size invalid";
        return "";
    }

    WiFiClient* stream = http.getStreamPtr();
    int bytesRead = stream->readBytes(manifestBuf, payloadLen);
    manifestBuf[bytesRead] = '\0';
    http.end();

    ESP_LOGI(TAG, "Manifest downloaded: %d bytes", bytesRead);

    // Parse the manifest
    if (!parseManifest(manifestBuf, bytesRead, manifest)) {
        ESP_LOGE(TAG, "Failed to parse manifest.json");
        publishProgress(0.0f, "Manifest parse failed", true, false);
        _state = OTAState::Error;
        _lastOtaError = "Manifest JSON parse failed";
        return "";
    }

    ESP_LOGI(TAG, "Manifest parsed: version='%s' channel='%s' assets=%d",
             manifest.version, manifest.channel, manifest.assetCount);

    return std::string(manifest.version);
#else
    // Native build stub — no WiFi, no HTTPS
    (void)manifestUrl;
    (void)cfg;
    (void)manifest;
    return "";
#endif
}

/**
 * @brief Get the current OTA state machine state.
 */
OTAState OTAUpdateManager::getState() const {
    return _state;
}

/**
 * @brief Get the last OTA error string.
 */
const char* OTAUpdateManager::getLastOtaError() const {
    return _lastOtaError;
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
             (unsigned)size, expectedMD5 ? expectedMD5 : "(none)");
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

    _expectedSize = size;
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
    if (_configManager != nullptr) {
        _configManager->getManifest().snapshotActiveToGolden();
        _configManager->getManifest().save();
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
esp_err_t OTAUpdateManager::handleSignedUpload(PsychicRequest* request,
                                           const String& filename,
                                           uint64_t index, uint8_t* data,
                                           size_t len, bool final) {
#ifndef NATIVE_BUILD
    if (_config == nullptr) {
        ESP_LOGE(TAG, "Upload: module not initialized (begin() not called)");
        return ESP_FAIL;
    }
    const FrameworkConfig& cfg = *_config;

    // --- Identify field by file extension ---
    // NOTE: PsychicHTTP's MultipartProcessor passes the actual filename from
    // Content-Disposition (e.g. "firmware-esp32dev.bin"), NOT the form field
    // name ("firmware" / "signature"). Match on extension.
    const bool isSigField = filename.endsWith(".sig");
    const bool isBinField = filename.endsWith(".bin");

    // ---- Signature field: collect bytes into _uploadSigBuf ----
    if (isSigField) {
        if (index == 0) {
            _uploadSigLen = 0;
            ESP_LOGI(TAG, "Upload: receiving signature file '%s'", filename.c_str());
        }
        size_t space = sizeof(_uploadSigBuf) - _uploadSigLen;
        size_t toCopy = (len < space) ? len : space;
        memcpy(_uploadSigBuf + _uploadSigLen, data, toCopy);
        _uploadSigLen += toCopy;
        if (final) {
            ESP_LOGI(TAG, "Upload: signature received — %u bytes", (unsigned)_uploadSigLen);
        }
        return ESP_OK;
    }

    // ---- Firmware field ----
    if (!isBinField) {
        ESP_LOGW(TAG, "Upload: ignoring unknown field '%s'", filename.c_str());
        return ESP_OK;
    }

    if (_uploadAborted) return ESP_FAIL; // Already failed — drain remaining chunks silently

    // --- First chunk: reset state and initialise verifier ---
    if (index == 0) {
        _uploadAborted  = false;
        _uploadBinSize  = 0;

        ESP_LOGI(TAG, "Upload: receiving firmware '%s'", filename.c_str());

        // Clean up any previous verifier
        delete _uploadVerifier;
        _uploadVerifier = nullptr;

        // Initialise MbedTLS verifier if a public key is configured
        if (cfg.otaPublicKey != nullptr) {
            _uploadVerifier = new (std::nothrow) MbedTLSSignatureVerifier();
            if (!_uploadVerifier || !_uploadVerifier->begin(cfg.otaPublicKey)) {
                ESP_LOGE(TAG, "Upload: verifier init failed");
                delete _uploadVerifier;
                _uploadVerifier = nullptr;
                _uploadAborted  = true;
                return ESP_FAIL;
            }
            ESP_LOGI(TAG, "Upload: signature verifier initialized");
        }

        // Validate ESP32 binary magic byte
        if (len > 0 && data[0] != 0xE9) {
            ESP_LOGE(TAG, "Upload: invalid binary header 0x%02X (expected 0xE9)", data[0]);
            _uploadAborted = true;
            return ESP_FAIL;
        }

        // Begin the flash update — size is unknown at this point so use OTA_SIZE_UNKNOWN
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            ESP_LOGE(TAG, "Upload: Update.begin() failed");
            _uploadAborted = true;
            return ESP_FAIL;
        }

        ESP_LOGI(TAG, "Upload: flash write started (OTA partition ready)");
    }

    // --- Accumulate firmware bytes into verifier and flash ---
    if (_uploadVerifier != nullptr) {
        _uploadVerifier->update(data, len);
    }

    if (Update.write(data, len) != len) {
        ESP_LOGE(TAG, "Upload: flash write failed at offset %u",
                 (unsigned)_uploadBinSize);
        Update.abort();
        _uploadAborted = true;
        delete _uploadVerifier;
        _uploadVerifier = nullptr;
        return ESP_FAIL;
    }

    _uploadBinSize += len;

    // Log progress every 128 KB
    if ((_uploadBinSize % (128 * 1024)) < len) {
        ESP_LOGI(TAG, "Upload: %u KB written to flash", (unsigned)(_uploadBinSize / 1024));
    }

    // --- Final firmware chunk: just log, verification deferred to onRequest ---
    if (final) {
        ESP_LOGI(TAG, "Upload: firmware transfer complete — %u bytes received", (unsigned)_uploadBinSize);
    }
    return ESP_OK;
#else
    (void)request; (void)filename; (void)index; (void)data; (void)len; (void)final;
    return ESP_OK;
#endif
}

// =============================================================================
// Signed Upload Finalisation — called by onRequest after ALL multipart fields
// =============================================================================
/**
 * @brief Verify signature and finalise the OTA update after all multipart
 *        fields have been received.
 *
 * PsychicHTTP's onRequest callback fires after the multipart parser has
 * processed every field. This guarantees that both the .bin and .sig data
 * are available before we attempt verification.
 *
 * @param response  The PsychicResponse to send the result on.
 * @return ESP_OK on success, ESP_FAIL on any error.
 */
esp_err_t OTAUpdateManager::finaliseSignedUpload(PsychicResponse* response) {
#ifndef NATIVE_BUILD
    // If the upload was already aborted during chunk processing, report it
    if (_uploadAborted) {
        ESP_LOGE(TAG, "Upload: finalisation skipped — upload was aborted during transfer");
        Update.abort();
        delete _uploadVerifier;
        _uploadVerifier = nullptr;
        response->send(500, "application/json",
                      "{\"status\":\"error\",\"msg\":\"Upload failed during transfer — check serial log\"}");
        return ESP_FAIL;
    }

    // No firmware received at all
    if (_uploadBinSize == 0) {
        ESP_LOGE(TAG, "Upload: no firmware data received");
        response->send(400, "application/json",
                      "{\"status\":\"error\",\"msg\":\"No firmware binary received\"}");
        return ESP_FAIL;
    }

    const FrameworkConfig& cfg = *_config;

    // Fail-closed: public key configured → signature is mandatory
    if (cfg.otaPublicKey != nullptr) {
        if (_uploadSigLen == 0) {
            ESP_LOGE(TAG, "Upload: SECURITY ABORT — no signature provided");
            Update.abort();
            delete _uploadVerifier;
            _uploadVerifier = nullptr;
            response->send(400, "application/json",
                          "{\"status\":\"error\",\"msg\":\"Signature file required — upload both .bin and .sig\"}");
            return ESP_FAIL;
        }

        ESP_LOGI(TAG, "Upload: verifying ECDSA signature (%u sig bytes, %u firmware bytes)...",
                 (unsigned)_uploadSigLen, (unsigned)_uploadBinSize);

        if (_uploadVerifier == nullptr ||
            !_uploadVerifier->verify(_uploadSigBuf, _uploadSigLen)) {
            ESP_LOGE(TAG, "Upload: ECDSA verification FAILED — firmware rejected");
            Update.abort();
            delete _uploadVerifier;
            _uploadVerifier = nullptr;
            response->send(403, "application/json",
                          "{\"status\":\"error\",\"msg\":\"Signature verification failed — firmware rejected\"}");
            return ESP_FAIL;
        }

        ESP_LOGI(TAG, "Upload: ECDSA verification PASSED ✓");
    } else {
        ESP_LOGW(TAG, "Upload: no public key configured — signature check SKIPPED");
    }

    delete _uploadVerifier;
    _uploadVerifier = nullptr;

    if (!Update.end(true /* set boot partition */)) {
        ESP_LOGE(TAG, "Upload: Update.end() failed — firmware not applied");
        response->send(500, "application/json",
                      "{\"status\":\"error\",\"msg\":\"Update finalisation failed\"}");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Upload: firmware flashed successfully — rebooting in 500ms");
    response->send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"Firmware verified and flashed — rebooting\"}");
    delay(500);
    ESP.restart();
    return ESP_OK; // unreachable but keeps compiler happy
#else
    (void)response;
    return ESP_OK;
#endif
}

void OTAUpdateManager::registerRoutes(PsychicHttpServer& server) {
#ifndef NATIVE_BUILD
    // --- GET: status endpoint ---
    server.on("/api/ota", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        doc["status"] = "ready";
        std::string output;
        serializeJson(doc, output);
        response->send(200, "application/json", output.c_str());
        return ESP_OK;
    });

    // --- POST: action dispatcher ---
    server.on("/api/ota", HTTP_POST,
        [this](PsychicRequest *request, PsychicResponse* response) {
            const char* body = request->bodyCStr();
            if (!body || strlen(body) == 0) {
                response->send(400, "application/json", "{\"error\":\"body_required\"}");
                return ESP_FAIL;
            }

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, body);
            if (!error) {
                const char* action = doc["action"] | "";

                if (strcmp(action, "rollback") == 0) {
                    const char* confirm = doc["confirm"] | "";
                    if (strcmp(confirm, "ROLLBACK") != 0) {
                        response->send(403, "application/json",
                                      "{\"status\":\"error\",\"msg\":\"Missing or invalid confirmation token\"}");
                        return ESP_FAIL;
                    }
                    response->send(200, "application/json", "{\"status\":\"ok\"}");
                    this->rollbackFirmware();
                    return ESP_OK;
                } else if (strcmp(action, "markValid") == 0) {
                    this->markAppValid();
                    response->send(200, "application/json", "{\"status\":\"ok\"}");
                    return ESP_OK;
                } else if (strcmp(action, "checkGithub") == 0) {
                    ESP_LOGI(TAG, "GitHub OTA check triggered via API");
                    if (this->_state == OTAState::Error) {
                        this->_state = OTAState::Idle;
                        this->_lastOtaError = nullptr;
                    }
                    this->_silentCheck = false;
                    this->performUpdate();
                    response->send(200, "application/json",
                                  "{\"status\":\"ok\",\"msg\":\"OTA check started\"}");
                    return ESP_OK;
                } else if (strcmp(action, "setChannel") == 0) {
                    const char* ch = doc["channel"] | "";
                    if (strlen(ch) > 0 && strlen(ch) < sizeof(this->_channel)) {
                        strlcpy(this->_channel, ch, sizeof(this->_channel));
                        if (_configManager != nullptr) {
                            _configManager->saveModuleConfig(this);
                        }
                        ESP_LOGI(TAG, "Channel changed to: %s", this->_channel);
                        response->send(200, "application/json", "{\"status\":\"ok\"}");
                    } else {
                        response->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid channel\"}");
                    }
                    return ESP_OK;
                } else if (strcmp(action, "setAutoCheck") == 0) {
                    if (doc["autoCheck"].is<bool>()) {
                        this->_autoCheck = doc["autoCheck"].as<bool>();
                        if (_configManager != nullptr) {
                            _configManager->saveModuleConfig(this);
                        }
                        ESP_LOGI(TAG, "Auto-check changed to: %d", (int)this->_autoCheck);
                        response->send(200, "application/json", "{\"status\":\"ok\"}");
                    } else {
                        response->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Missing autoCheck boolean\"}");
                    }
                    return ESP_OK;
                } else if (strcmp(action, "setUpdatesEnabled") == 0) {
                    if (doc["updatesEnabled"].is<bool>()) {
                        this->_updatesEnabled = doc["updatesEnabled"].as<bool>();
                        if (_configManager != nullptr) {
                            _configManager->saveModuleConfig(this);
                        }
                        ESP_LOGI(TAG, "Updates-enabled changed to: %d", (int)this->_updatesEnabled);
                        response->send(200, "application/json", "{\"status\":\"ok\"}");
                    } else {
                        response->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Missing updatesEnabled boolean\"}");
                    }
                    return ESP_OK;
                } else if (strcmp(action, "setCheckHour") == 0) {
                    if (doc["checkHour"].is<int>()) {
                        int h = doc["checkHour"].as<int>();
                        if (h >= 0 && h <= 23) {
                            this->_checkHour = static_cast<uint8_t>(h);
                            if (_configManager != nullptr) {
                                _configManager->saveModuleConfig(this);
                            }
                            ESP_LOGI(TAG, "Check hour changed to: %u", (unsigned)this->_checkHour);
                            response->send(200, "application/json", "{\"status\":\"ok\"}");
                        } else {
                            response->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"checkHour must be 0-23\"}");
                        }
                    } else {
                        response->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Missing checkHour integer\"}");
                    }
                    return ESP_OK;
                } else if (strcmp(action, "status") == 0) {
                    JsonDocument resp;
                    const char* stateStr = "unknown";
                    switch (this->_state) {
                        case OTAState::Idle:        stateStr = "idle"; break;
                        case OTAState::Checking:    stateStr = "checking"; break;
                        case OTAState::Downloading: stateStr = "downloading"; break;
                        case OTAState::Verifying:   stateStr = "verifying"; break;
                        case OTAState::Complete:    stateStr = "complete"; break;
                        case OTAState::Error:       stateStr = "error"; break;
                    }
                    resp["state"]          = stateStr;
                    resp["version"]        = BUILD_VERSION;
                    resp["channel"]        = this->_channel;
                    resp["boardId"]        = BOARD_ID;
                    resp["autoCheck"]      = this->_autoCheck;
                    resp["updatesEnabled"] = this->_updatesEnabled;
                    resp["checkHour"]      = this->_checkHour;
                    resp["dram_free_bytes"]  = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
                    resp["ota_stack_bytes"]  = OTA_TASK_STACK_BYTES;
                    resp["progress"]         = this->_lastProgress;
                    if (this->_lastOtaError) {
                        resp["lastError"] = this->_lastOtaError;
                    }
                    std::string output;
                    serializeJson(resp, output);
                    response->send(200, "application/json", output.c_str());
                    return ESP_OK;
                }
            }
            response->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid action\"}");
            return ESP_FAIL;
        }
    );

    // --- POST /api/ota/upload — signed multipart firmware upload ---
    PsychicUploadHandler* otaUploadHandler = new (std::nothrow) PsychicUploadHandler();
    if (otaUploadHandler == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate PsychicUploadHandler");
        return;
    }
    otaUploadHandler->onRequest([this](PsychicRequest* request, PsychicResponse* response) {
        // Called after ALL multipart fields have been processed.
        // Verify the signature and finalise the OTA update here.
        return this->finaliseSignedUpload(response);
    });
#ifdef ARDUINO
    // EXTERNAL CONSTRAINT: PsychicHttp API requires const String& in onUpload callback signature
    otaUploadHandler->onUpload([this](PsychicRequest* request, const String& filename,
                                      uint64_t index, uint8_t* data, size_t len, bool final) {
        return this->handleSignedUpload(request, filename, index, data, len, final);
    });
#else
    otaUploadHandler->onUpload([this](PsychicRequest* request, const char* filename,
                                      uint64_t index, uint8_t* data, size_t len, bool final) {
        return this->handleSignedUpload(request, String(filename), index, data, len, final);
    });
#endif
    server.on("/api/ota/upload", HTTP_POST, otaUploadHandler);
#endif
}

// =============================================================================
// IMCPContributor Implementation
// =============================================================================

/**
 * @brief Convert OTAState enum to a human-readable string.
 *
 * Used by ota_get_status MCP tool. Defined as a file-local helper to avoid
 * polluting the class interface.
 *
 * @param state The OTAState value to convert.
 * @return Null-terminated C-string label.
 */
static const char* otaStateToString(OTAState state) {
    switch (state) {
        case OTAState::Idle:        return "Idle";
        case OTAState::Checking:    return "Checking";
        case OTAState::Downloading: return "Downloading";
        case OTAState::Verifying:   return "Verifying";
        case OTAState::Complete:    return "Complete";
        case OTAState::Error:       return "Error";
        default:                    return "Unknown";
    }
}

void OTAUpdateManager::registerMCPTools(MCPToolRegistry& registry) {
#ifndef NATIVE_BUILD
    // --- ota_get_status: read-only status snapshot ---
    registry.addTool("ota_get_status", "Get OTA update manager status, firmware version, channel, and timing",
        [](void* context, JsonObject& args, JsonObject& result) {
            OTAUpdateManager* self = static_cast<OTAUpdateManager*>(context);
            result["state"]         = otaStateToString(self->_state);
            result["version"]       = BUILD_VERSION;
            result["channel"]       = self->_channel;
            result["last_check_ms"] = (self->_lastCheckMillis > 0)
                                     ? static_cast<long>(millis() - self->_lastCheckMillis)
                                     : -1;
            result["auto_check"]     = self->_autoCheck;
            result["updates_enabled"] = self->_updatesEnabled;
            if (self->_lastOtaError) {
                result["last_error"] = self->_lastOtaError;
            }
        }, this, false);

    // --- ota_check_now: force a GitHub release check ---
    registry.addTool("ota_check_now", "Force an immediate OTA update check against GitHub releases",
        [](void* context, JsonObject& args, JsonObject& result) {
            OTAUpdateManager* self = static_cast<OTAUpdateManager*>(context);

            // Reset error state if stuck — allows re-check after transient failure
            if (self->_state == OTAState::Error) {
                self->_state = OTAState::Idle;
                self->_lastOtaError = nullptr;
            }

            if (self->_state != OTAState::Idle) {
                result["update_available"] = false;
                result["latest_version"]   = "";
                result["error"]            = "OTA pipeline is busy";
                return;
            }

            bool available = self->checkForUpdate();
            result["update_available"] = available;
            result["latest_version"]   = BUILD_VERSION;  // Local version (remote not stored)
        }, this, true);

    // --- ota_trigger_update: begin the full OTA pipeline ---
    {
        JsonDocument schema;
        JsonObject props = schema["properties"].to<JsonObject>();
        props["version"]["type"] = "string";
        props["version"]["description"] = "Optional target version string (reserved for future use)";
        schema["type"] = "object";

        registry.addTool("ota_trigger_update",
            "Trigger the full OTA download, verify, and flash pipeline",
            schema,
            [](void* context, JsonObject& args, JsonObject& result) {
                OTAUpdateManager* self = static_cast<OTAUpdateManager*>(context);

                // Reset error state if stuck
                if (self->_state == OTAState::Error) {
                    self->_state = OTAState::Idle;
                    self->_lastOtaError = nullptr;
                }

                if (self->_state != OTAState::Idle) {
                    result["status"] = "OTA pipeline is busy";
                    return;
                }

                // Note: 'version' param accepted but not consumed yet.
                // The pipeline always targets the latest release for the
                // configured channel. Version-pinned OTA is a future feature.
                self->_silentCheck = false;
                self->performUpdate();
                result["status"] = "Update initiated";
            }, this, true);
    }
#endif
}
