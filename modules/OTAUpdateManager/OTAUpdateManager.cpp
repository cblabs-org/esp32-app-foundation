/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/OTAUpdateManager/OTAUpdateManager.cpp
 * Description: Implementation of the full OTA orchestrator. Preserves all legacy
 *              direct-upload functionality (beginUpdate/writeStream/endUpdate) and
 *              adds the manifest-first checkForUpdate/performUpdate pipeline with
 *              a persistent FreeRTOS worker task (created once in begin(), reused
 *              via TaskNotify for every OTA check), IOTAVerifier-based cryptographic
 *              signature verification, and EventBus progress publishing.
 *              This eliminates the per-cycle 16 KB DRAM leak present in the prior
 *              one-shot task pattern, where xTaskCreateWithCaps + vTaskDelete
 *              (wrong API) leaked the DRAM-caps stack on every OTA check.
 *
 * Exported Classes:
 * - OTAUpdateManager: [Class implementation]
 *   - begin():              Store Application reference for dependency access
 *   - checkForUpdate():     Check for new firmware via manifest-first flow
 *   - performUpdate():      Notify the persistent OTA worker task via xTaskNotifyGive
 *   - performUpdateTask():  Execute the full OTA pipeline (runs in task context)
 *   - otaTaskLoop():        Persistent FreeRTOS task entry (blocks on ulTaskNotifyTake)
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

#ifdef WITH_OTA

#include "OTAUpdateManager.hpp"
#include "utils/Periodic.hpp"
#include "utils/serveGzipAsset.hpp"
#include "platform/LogShim.h"

static constexpr const char* TAG = "OTAUpdateManager";

// ---------------------------------------------------------------------------
// Platform-conditional includes
// Hardware headers are excluded from the native GoogleTest build.
// Module includes go in the .cpp ONLY (behind native test stub) to
// respect the Hollywood Principle — no cross-module includes in headers.
// ---------------------------------------------------------------------------
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
#include "Application.hpp"

#include "OTACerts.h"
#include "OTADownloadEngine.hpp"
#include "MbedTLSSignatureVerifier.hpp"
#include "interfaces/IWebContributor.hpp"
#include "OTAManifest.hpp"
#include "Audit.h"
#include "MCPToolRegistry.hpp"
#include "SemVer.hpp"
#include "portal_assets/ota_js.hpp"

// ---------------------------------------------------------------------------
// Global instance — REMOVED (Application now owns _otaManager)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

OTAUpdateManager::OTAUpdateManager() {}

/**
 * @brief Destructor — deletes the persistent OTA worker task.
 *
 * vTaskDeleteWithCaps() is the correct paired destructor for tasks created
 * with xTaskCreateWithCaps(). vTaskDelete() cannot free caps-allocated stacks
 * and would leak 16 KB of DRAM. Safe if begin() was never called (_taskHandle
 * will be nullptr and the guard skips the delete).
 */
OTAUpdateManager::~OTAUpdateManager() {
    if (_taskHandle != nullptr) {
        TaskHandle_t handle = _taskHandle;
        clearTaskHandle_();                 // null the member first (no dangling access)
        vTaskDeleteWithCaps(handle);        // free the caps-allocated DRAM stack
    }
}

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
 * Version 5: adds "learnedManifestUrl" for manifest URL resilience.
 * Increment this when a future firmware change requires a config migration.
 */
int OTAUpdateManager::getSchemaVersion() const {
    return 5;
}

/**
 * @brief Apply OTA configuration values from a parsed JSON section.
 *
 * Reads "channel", "autoCheck", "lastCheckedChannel", "updatesEnabled",
 * and "learnedManifestUrl" from the section. Missing keys fall back to
 * safe defaults via ArduinoJson's | operator.
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
    const char* lmu = section["learnedManifestUrl"] | "";
    strlcpy(_learnedManifestUrl, lmu, sizeof(_learnedManifestUrl));
    ESP_LOGI(TAG, "Config applied — channel='%s' autoCheck=%d updatesEnabled=%d checkHour=%u lastCheckedChannel='%s' learnedManifestUrl='%s'",
             _channel, (int)_autoCheck, (int)_updatesEnabled, (unsigned)_checkHour, _lastCheckedChannel, _learnedManifestUrl);
}

/**
 * @brief Serialize current OTA configuration into a JSON section.
 *
 * Writes "channel", "autoCheck", "lastCheckedChannel", "updatesEnabled",
 * and "learnedManifestUrl" into the provided section for persistence by
 * ConfigManager (stored at /mod/ota.json).
 *
 * @param section Mutable reference to the target JSON object.
 */
void OTAUpdateManager::serializeConfig(JsonObject& section) const {
    section["channel"]            = _channel;
    section["autoCheck"]          = _autoCheck;
    section["updatesEnabled"]     = _updatesEnabled;
    section["lastCheckedChannel"] = _lastCheckedChannel;
    section["checkHour"]          = _checkHour;
    section["learnedManifestUrl"] = _learnedManifestUrl;
}

/**
 * @brief Migrate OTA configuration from an older schema version.
 *
 * Handles v1→v2 migration by adding the lastCheckedChannel field.
 *
 * @param section   Mutable reference to the target JSON config section.
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
    if (fromVersion < 5) {
        section["learnedManifestUrl"] = "";
        ESP_LOGI(TAG, "Migrated OTA config v%d → v5: added learnedManifestUrl", fromVersion);
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
    _learnedManifestUrl[0] = '\0';
    ESP_LOGI(TAG, "Config reset to defaults — channel='stable' autoCheck=true updatesEnabled=true checkHour=3 learnedManifestUrl=''");
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

    // --- Create the persistent OTA worker task (once per device lifetime) ---
    // Stack MUST be in DRAM (MALLOC_CAP_INTERNAL) because the OTA pipeline
    // performs SPI flash writes. PSRAM-stack tasks are unsafe for flash I/O
    // (the SPI cache is disabled during flash ops, making PSRAM inaccessible).
    // Pre-flight DRAM check: task creation here — where memory is most abundant
    // — is far safer than dynamic allocation on every performUpdate() call.
    uint32_t freeDram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "Creating OTA worker task (stack=%" PRIu32 " B, free DRAM=%" PRIu32 " B)",
             OTA_TASK_STACK_BYTES, freeDram);

    BaseType_t result = xTaskCreateWithCaps(
        otaTaskLoop,
        "ota_worker",
        OTA_TASK_STACK_BYTES,
        this,
        5,
        &_taskHandle,
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG,
            "Failed to create OTA worker task — "
            "free DRAM: %" PRIu32 " B, stack required: %" PRIu32 " B. "
            "OTA checks will not be available until reboot.",
            freeDram, OTA_TASK_STACK_BYTES);
        _taskHandle = nullptr;
    } else {
        ESP_LOGI(TAG, "OTA worker task created (persistent, DRAM stack)");
    }
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

    // Gate: do not attempt any network operation until Wi-Fi is confirmed up.
    // This prevents the boot check from racing the TLS/DNS stack during the
    // first few seconds after association, and prevents the daily check from
    // firing during a temporary network outage.
    if (WiFi.status() != WL_CONNECTED) return;

    // --- Boot check: fire once, after Wi-Fi is confirmed connected ---
    if (!_hasBootChecked) {
        _hasBootChecked = true;
        _silentCheck = true;  // Boot checks never show error modals
        ESP_LOGI(TAG, "Auto-check: boot trigger (Wi-Fi confirmed connected)");
        performUpdate();
        return;
    }

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
}

/**
 * @brief Check for a newer firmware version.
 *
 * Guards: must be in Idle state and have a configured otaManifestUrl.
 * On native: always returns false.
 *
 * The full manifest-first check is performed by performUpdateTask().
 * This standalone method is retained for the MCP ota_check_now tool
 * and returns false until a lightweight manifest-only check is wired.
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

    // Guard: must have at least one manifest URL source (compiled-in or learned)
    if (_config->otaManifestUrl == nullptr && _learnedManifestUrl[0] == '\0') {
        ESP_LOGW(TAG, "checkForUpdate() — no manifest URL configured (compiled-in or learned) — returning false");
        return false;
    }

    _state = OTAState::Checking;
    _lastCheckTimer.reset(millis());
    ESP_LOGI(TAG, "Checking for updates — local: %s, channel: '%s'", BUILD_VERSION, _channel);

    // Manifest-first check is performed by performUpdateTask(). This method
    // returns false until a lightweight manifest-only version check is wired.
    ESP_LOGW(TAG, "checkForUpdate() — use performUpdate() for manifest-first check");
    _state = OTAState::Idle;
    return false;
}

/**
 * @brief Start the full OTA download + verify + flash pipeline.
 *
 * Launches a FreeRTOS task and returns immediately. The task runs
 * performUpdateTask() which handles the complete pipeline. On native
 * builds, sets state to Error (no FreeRTOS available).
 */
void OTAUpdateManager::performUpdate() {
    // Guard: must be initialized (begin() creates the worker task)
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

    // Guard: worker task must be running (begin() may have failed to create it)
    if (_taskHandle == nullptr) {
        ESP_LOGE(TAG, "OTA worker task unavailable — begin() not called or task creation failed");
        _state = OTAState::Error;
        _lastOtaError = "OTA worker task unavailable";
        publishProgress(0.0f, "OTA worker task unavailable", true, false);
        return;
    }

    // Transition state first (before notify, so the task sees Checking on wake)
    _state = OTAState::Checking;
    ESP_LOGI(TAG, "Notifying OTA worker task to begin pipeline");

    // Wake the persistent worker task — zero allocation, returns immediately.
    // The task was created with xTaskCreateWithCaps(MALLOC_CAP_INTERNAL) in
    // begin(). Its DRAM stack is reused for every OTA check; no per-cycle
    // heap allocation occurs here.
    xTaskNotifyGive(_taskHandle);
}

/**
 * @brief Persistent FreeRTOS task loop — blocks on TaskNotify between OTA checks.
 *
 * Runs for the lifetime of the device. Each performUpdate() call wakes this
 * task via xTaskNotifyGive(). After performUpdateTask() completes (or the
 * pipeline reaches reboot), the loop returns to blocking sleep, ready for
 * the next check. No memory is allocated or freed per cycle.
 *
 * Deletion: only from ~OTAUpdateManager() via vTaskDeleteWithCaps(_taskHandle).
 * vTaskDelete() must NOT be called here — it cannot free caps-allocated stacks.
 */
void OTAUpdateManager::otaTaskLoop(void* param) {
    auto* self = static_cast<OTAUpdateManager*>(param);
    for (;;) {
        // Block until performUpdate() sends a notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        self->performUpdateTask();
    }
}

/**
 * @brief Null out _taskHandle.
 *
 * Called from the destructor immediately before vTaskDeleteWithCaps() to
 * ensure the stale handle cannot be read after deletion.
 */
void OTAUpdateManager::clearTaskHandle_() {
    _taskHandle = nullptr;
}

/**
 * @brief Execute the full OTA pipeline inside the FreeRTOS task.
 *
 * Manifest-first flow (WO3 rewire):
 *   1. Fetch + verify manifest via fetchAndVerifyManifest_()
 *   2. Channel lookup via findChannel()
 *   3. SemVer version comparison
 *   4. Board asset lookup via findAssetForBoard()
 *   5. Snapshot config manifest (Active → Previous)
 *   6. Initialize the verifier (if public key configured)
 *   7. Create WiFiClientSecure on the heap
 *   8. Create OTADownloadEngine and configure
 *   9. Fetch signature file via asset URL
 *  10. Download and flash firmware via asset URL
 *  11. Finalize and reboot
 */
void OTAUpdateManager::performUpdateTask() {
    ESP_LOGI(TAG, "OTA task started (manifest-first flow)");
    AUDIT("ota", "pipeline_start");

    // NOTE: Do NOT register this task with TWDT here. The OTA pipeline makes
    // multiple blocking HTTPS calls (manifest, signature, firmware) involving
    // TLS handshakes that take 5-15+ seconds each. TWDT is registered only
    // during the firmware download phase (Step 10), where the progress
    // callback feeds it every ~4KB.

    const FrameworkConfig& cfg = *_config;

    // --- Guard: firmware updates must be enabled (persisted OTA config) ---
    if (!_updatesEnabled) {
        ESP_LOGW(TAG, "Firmware updates are disabled in config");
        publishProgress(0.0f, "Firmware updates disabled", true, false);
        _state = OTAState::Error;
        _lastOtaError = "Firmware updates disabled";
        return;
    }

    // --- Guard: at least one manifest URL must be available ---
    // Check both compiled-in URL and learned URL. fetchAndVerifyManifest_
    // handles the fallback logic; we only reject if neither source exists.
    if (cfg.otaManifestUrl == nullptr && _learnedManifestUrl[0] == '\0') {
        if (_silentCheck) {
            // Boot/scheduled checks: suppress error, return to idle
            _state = OTAState::Idle;
            _silentCheck = false;
            return;
        }
        ESP_LOGE(TAG, "No manifest URL configured (compiled-in or learned)");
        publishProgress(0.0f, "No manifest URL configured", true, false);
        _state = OTAState::Error;
        _lastOtaError = "No manifest URL configured";
        return;
    }

    // --- Step 1: Fetch, verify, and parse the manifest ---
    _lastCheckTimer.reset(millis());
    ESP_LOGI(TAG, "Fetching manifest — channel: '%s', local: %s", _channel, BUILD_VERSION);

    OTAManifest manifest{};
    if (!fetchAndVerifyManifest_(manifest)) {
        // fetchAndVerifyManifest_ already sets _state, _lastOtaError, and
        // publishes a progress event on failure.
        if (!_silentCheck) {
            // Error state already set by fetchAndVerifyManifest_
        } else {
            // Silent check: suppress error modal, return to idle
            _state = OTAState::Idle;
            _lastOtaError = nullptr;
        }
        _silentCheck = false;
        return;
    }

    // --- Step 2: Channel lookup — direct, no risk-floor scanning ---
    // The deploy script (WO4) ensures each channel entry in the manifest
    // already contains the best available version for that risk floor.
    const OTAChannel* channel = findChannel(manifest, _channel);
    if (channel == nullptr) {
        ESP_LOGW(TAG, "Channel '%s' not found in manifest (channels=%d) — no update for this channel",
                 _channel, manifest.channelCount);
        // Fail-closed: if the manifest doesn't contain our channel, do not
        // attempt any update. This is not an error — it just means no release
        // has been published for this channel yet.
        if (!_silentCheck) {
            // Not an error condition — just no update available
        }
        _state = OTAState::Idle;
        _silentCheck = false;
        return;
    }

    ESP_LOGI(TAG, "Channel '%s' found — manifest version: %s, securityVersion: %u",
             _channel, channel->version, (unsigned)channel->securityVersion);

    // --- Step 3: SemVer comparison — only proceed if remote > local ---
    FirmwareVersion local = { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
                              static_cast<Channel>(VERSION_CHANNEL), VERSION_PRE_NUM };
    FirmwareVersion remote = parseVersionTag(channel->version);

    if (!(remote > local)) {
        // Check for implicit downgrade: channel changed since last check
        // e.g., switched from beta to stable — allow "downgrade" to the latest stable
        if (strcmp(_channel, _lastCheckedChannel) != 0 && _lastCheckedChannel[0] != '\0') {
            ESP_LOGI(TAG, "Channel changed ('%s' → '%s') — allowing implicit version change",
                     _lastCheckedChannel, _channel);
            strlcpy(_lastCheckedChannel, _channel, sizeof(_lastCheckedChannel));
            if (_configManager != nullptr) {
                _configManager->saveModuleConfig(static_cast<IConfigurable*>(this));
            }
            // Fall through to download — channel switch is an implicit update
        } else {
            ESP_LOGI(TAG, "No update needed — local %s is current or newer than remote %s (channel='%s')",
                     BUILD_VERSION, channel->version, _channel);
            strlcpy(_lastCheckedChannel, _channel, sizeof(_lastCheckedChannel));
            _state = OTAState::Idle;
            _silentCheck = false;
            return;
        }
    } else {
        ESP_LOGI(TAG, "Update available: %s > %s (channel='%s') — proceeding",
                 channel->version, BUILD_VERSION, _channel);
    }

    // Update lastCheckedChannel
    strlcpy(_lastCheckedChannel, _channel, sizeof(_lastCheckedChannel));

    // --- Step 4: Board asset lookup ---
    const OTAAsset* asset = findAssetForBoard(*channel, BOARD_ID);
    if (asset == nullptr) {
        ESP_LOGE(TAG, "No asset for board '%s' in channel '%s' (assets=%d)",
                 BOARD_ID, _channel, channel->assetCount);
        publishProgress(0.0f, "No firmware for this board", true, false);
        _state = OTAState::Error;
        _lastOtaError = "No firmware asset for this board in manifest";
        _silentCheck = false;
        return;
    }

    ESP_LOGI(TAG, "Asset resolved — board: '%s', firmware: %s, signature: %s, size: %u",
             asset->board, asset->firmwareUrl, asset->signatureUrl, (unsigned)asset->size);

    // Only now transition to Downloading — a download will actually happen
    _state = OTAState::Downloading;
    publishProgress(0.0f, "Update found — preparing download...", false, false);
    _silentCheck = false;

    // --- Step 5: Snapshot config manifest: Active → Previous before download ---
    _configManager->getManifest().snapshotActiveToPrevious();
    _configManager->getManifest().save();
    ESP_LOGI(TAG, "Config manifest: Active → Previous snapshot saved");

    // --- Step 6: Initialize the verifier (if public key configured) ---
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

    // --- Step 7: Create WiFiClientSecure on the heap ---
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

    // --- Step 8: Create OTADownloadEngine and configure ---
    OTADownloadEngine engine;
    if (verifierReady) {
        engine.setVerifier(&verifier);
    }

    // Set progress callback to forward to EventBus
    // Download progress maps to 2%-100% range (0%-2% covers manifest/sig phases)
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

    // --- Step 9: Fetch signature file using manifest asset URL ---
    uint8_t sigBuf[512];
    size_t sigLen = 0;
    std::string token = cfg.otaToken ? cfg.otaToken : "";

    if (asset->signatureUrl[0] != '\0') {
        std::string sigUrlStr(asset->signatureUrl);
        sigLen = engine.fetchSignature(*client, sigUrlStr, token, sigBuf, sizeof(sigBuf));
        if (sigLen == 0 && cfg.otaPublicKey != nullptr) {
            ESP_LOGE(TAG, "SECURITY ABORT: Signature fetch returned 0 bytes — refusing update");
            publishProgress(0.0f, "Signature fetch failed — update rejected", true, false);
            _state = OTAState::Error;
            _lastOtaError = "Signature empty — update rejected (fail-closed)";
            return;
        } else if (sigLen > 0) {
            ESP_LOGI(TAG, "Fetched firmware signature: %u bytes", (unsigned)sigLen);
        }
    } else if (cfg.otaPublicKey != nullptr) {
        ESP_LOGE(TAG, "SECURITY ABORT: No signature URL in manifest but otaPublicKey is configured — refusing unsigned firmware");
        publishProgress(0.0f, "Missing signature URL — update rejected", true, false);
        _state = OTAState::Error;
        _lastOtaError = "No signature URL in manifest — update rejected (fail-closed)";
        return;
    }

    publishProgress(0.02f, "Downloading firmware...", false, false);

    // --- Step 10: Download and flash firmware using manifest asset URL ---
    // Set expected version for pre-flash binary validation safety gate.
    // channel->version contains the SemVer string (e.g. "v0.0.11-dev.14")
    // from the signed manifest. The engine strips the leading 'v' and
    // compares against the esp_app_desc_t.version field in the first chunk
    // of the downloaded binary before any flash writes occur.
    engine.setExpectedVersion(channel->version);

    std::string binUrlStr(asset->firmwareUrl);

    // Register with TWDT only for the download phase — the progress callback
    // feeds the watchdog every ~4KB. All prior phases (manifest, signature)
    // involve unpredictable blocking I/O and must NOT be under TWDT.
    esp_task_wdt_add(NULL);
    OTAError result = engine.downloadAndFlash(*client, binUrlStr, token, sigBuf, sigLen);
    esp_task_wdt_delete(NULL);

    if (result != OTAError::None) {
        _lastOtaError = engine.getLastErrorString();
        ESP_LOGE(TAG, "OTA download failed: %s", _lastOtaError);
        AUDIT("ota", "pipeline_failed", nullptr, _lastOtaError);
        publishProgress(0.0f, _lastOtaError, true, false);
        _state = OTAState::Error;
        return;
    }

    ESP_LOGI(TAG, "Firmware downloaded and verified successfully");

    // --- Step 11: Finalize ---
    _state = OTAState::Complete;
    publishProgress(1.0f, "Update complete — rebooting...", true, true);
    ESP_LOGI(TAG, "OTA update complete — rebooting in 2 seconds");
    AUDIT_SYNC("ota", "pipeline_complete", nullptr, "rebooting");

    // SAFETY: TWDT was deregistered above. This delay is safe only because
    // we are outside TWDT scope. Do not move TWDT deregistration below.
    delay(2000);  // Allow event propagation and display update

    if (_preRebootHook) {
        ESP_LOGI(TAG, "Executing pre-reboot hook before restart");
        _preRebootHook();
    }

    ESP_LOGI(TAG, "OTA task stack high-water mark: %u bytes free",
             static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr) * sizeof(StackType_t)));

    ESP.restart();
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
    _progressEvent.progress = progress;
    _progressEvent.status   = status;
    _progressEvent.complete = complete;
    _progressEvent.success  = success;

    if (_eventBus != nullptr) {
        _eventBus->publish(ApplicationEvents::OTA_PROGRESS, &_progressEvent);
    }
    ESP_LOGD(TAG, "OTA progress: %.1f%% — %s (complete=%d, success=%d)",
             progress * 100.0f, status, complete, success);
}

// ---------------------------------------------------------------------------
// Manifest Fetcher + Signature Verification (WO2)
// ---------------------------------------------------------------------------

/**
 * @brief Download a raw file via HTTPS into a caller-supplied buffer.
 *
 * Uses the CA-pinned → insecure fallback pattern used throughout the OTA
 * pipeline: try with the OTA root CA bundle first; if the TLS handshake
 * fails, create a fresh WiFiClientSecure in insecure mode and retry.
 * The firmware/manifest signature is the real trust anchor, not transport.
 *
 * @param url     Null-terminated HTTPS URL to fetch.
 * @param token   Optional Bearer token (empty string = no auth header).
 * @param buf     Destination buffer (must be large enough for the payload).
 * @param bufSize Size of the destination buffer.
 * @param outLen  [out] Number of bytes written into buf on success.
 * @return true on success, false on any network or sizing error.
 */
static bool downloadRawFile_(const char* url, const char* token,
                             char* buf, size_t bufSize, size_t& outLen) {
    // --- Allocate WiFiClientSecure with CA-pinned certs ---
    std::unique_ptr<WiFiClientSecure> client(new (std::nothrow) WiFiClientSecure());
    if (!client) {
        ESP_LOGE(TAG, "downloadRawFile_: WiFiClientSecure allocation failed");
        return false;
    }
    client->setCACert(OTA_CA_BUNDLE);

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (!http.begin(*client, String(url))) {
        ESP_LOGE(TAG, "downloadRawFile_: HTTPClient.begin() failed — url=%s", url);
        return false;
    }

    http.addHeader("Accept", "application/octet-stream");
    if (token[0] != '\0') {
        http.addHeader("Authorization", String((std::string("Bearer ") + token).c_str()));
    }

    int httpCode = http.GET();
    if (httpCode < 0) {
        ESP_LOGW(TAG, "downloadRawFile_: CA-pinned fetch failed (err=%d) — retrying insecure", httpCode);
        http.end();

        // Feed TWDT before retry — TLS handshake can be slow
        esp_task_wdt_reset();

        // MUST create a fresh WiFiClientSecure — the failed handshake leaves
        // the mbedTLS context in a dirty state that cannot be reconfigured.
        client.reset(new (std::nothrow) WiFiClientSecure());
        if (!client) {
            ESP_LOGE(TAG, "downloadRawFile_: WiFiClientSecure retry allocation failed");
            return false;
        }
        client->setInsecure();

        if (!http.begin(*client, String(url))) {
            ESP_LOGE(TAG, "downloadRawFile_: HTTPClient.begin() retry failed — url=%s", url);
            return false;
        }
        http.addHeader("Accept", "application/octet-stream");
        if (token[0] != '\0') {
            http.addHeader("Authorization", String((std::string("Bearer ") + token).c_str()));
        }
        httpCode = http.GET();
    }

    if (httpCode != 200) {
        ESP_LOGE(TAG, "downloadRawFile_: HTTP %d — url=%s", httpCode, url);
        http.end();
        return false;
    }

    int payloadLen = http.getSize();
    if (payloadLen <= 0 || static_cast<size_t>(payloadLen) >= bufSize) {
        ESP_LOGE(TAG, "downloadRawFile_: payload size invalid: %d (bufSize=%u)", payloadLen, (unsigned)bufSize);
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    int bytesRead = stream->readBytes(buf, payloadLen);
    buf[bytesRead] = '\0';
    http.end();

    outLen = static_cast<size_t>(bytesRead);
    ESP_LOGI(TAG, "downloadRawFile_: fetched %d bytes from %s", bytesRead, url);
    return true;
}

/**
 * @brief Fetch manifest.json + manifest.json.sig, verify signature, and parse.
 *
 * Implements the URL fallback mechanism:
 *   1. If _learnedManifestUrl is non-empty, try it first.
 *   2. Fall back to the compiled-in URL (_config->otaManifestUrl).
 *
 * For each URL attempt:
 *   a. Download manifest.json into a static 4096-byte buffer.
 *   b. Download manifest.json.sig into a 256-byte signature buffer
 *      (URL = manifest URL + ".sig").
 *   c. Verify the signature using MbedTLSSignatureVerifier:
 *      begin(publicKey), update(manifest bytes), verify(sig bytes).
 *   d. On pass → parseManifest() → return true.
 *   e. On fail → log warning, try next URL.
 *
 * After successful verification: if manifest.manifestUrl differs from
 * the current _learnedManifestUrl, update and persist via config save.
 */
bool OTAUpdateManager::fetchAndVerifyManifest_(OTAManifest& manifest) {
    const FrameworkConfig& cfg = *_config;
    const char* token = cfg.otaToken ? cfg.otaToken : "";

    // --- Build URL attempt list (learned first, compiled-in second) ---
    const char* urls[2] = { nullptr, nullptr };
    int urlCount = 0;

    if (_learnedManifestUrl[0] != '\0') {
        urls[urlCount++] = _learnedManifestUrl;
    }
    if (cfg.otaManifestUrl != nullptr && cfg.otaManifestUrl[0] != '\0') {
        // Only add compiled-in URL if it differs from the learned URL
        if (urlCount == 0 || strcmp(urls[0], cfg.otaManifestUrl) != 0) {
            urls[urlCount++] = cfg.otaManifestUrl;
        }
    }

    if (urlCount == 0) {
        ESP_LOGE(TAG, "fetchAndVerifyManifest_: no manifest URLs configured");
        _state = OTAState::Error;
        _lastOtaError = "No manifest URL configured";
        publishProgress(0.0f, "No manifest URL configured", true, false);
        return false;
    }

    // Static buffer — only one OTA pipeline runs at a time (enforced by _state guard).
    // Saves stack headroom on the 16 KB OTA task.
    static char manifestBuf[4096];
    uint8_t sigBuf[257];  // 256-byte ECDSA sig + 1 for null terminator appended by downloadRawFile_()

    for (int i = 0; i < urlCount; ++i) {
        const char* manifestUrl = urls[i];
        bool isLearnedUrl = (i == 0 && _learnedManifestUrl[0] != '\0');

        ESP_LOGI(TAG, "fetchAndVerifyManifest_: attempting URL %d/%d (%s) — %s",
                 i + 1, urlCount, isLearnedUrl ? "learned" : "compiled-in", manifestUrl);

        // --- (a) Download manifest.json ---
        size_t manifestLen = 0;
        if (!downloadRawFile_(manifestUrl, token, manifestBuf, sizeof(manifestBuf), manifestLen)) {
            ESP_LOGW(TAG, "fetchAndVerifyManifest_: manifest download failed for URL: %s", manifestUrl);
            continue;  // Try next URL
        }

        // --- (b) Download manifest.json.sig ---
        // Construct sig URL by appending ".sig" to the manifest URL
        char sigUrl[280];
        snprintf(sigUrl, sizeof(sigUrl), "%s.sig", manifestUrl);

        size_t sigLen = 0;
        if (!downloadRawFile_(sigUrl, token, reinterpret_cast<char*>(sigBuf), sizeof(sigBuf), sigLen)) {
            ESP_LOGW(TAG, "fetchAndVerifyManifest_: signature download failed for URL: %s", sigUrl);
            if (cfg.otaPublicKey != nullptr) {
                // Signature is mandatory when a public key is configured
                continue;  // Try next URL
            }
            // If no public key configured, proceed without signature (unsigned mode)
        }

        // --- (c) Verify signature ---
        if (cfg.otaPublicKey != nullptr) {
            MbedTLSSignatureVerifier verifier;
            if (!verifier.begin(cfg.otaPublicKey)) {
                ESP_LOGE(TAG, "fetchAndVerifyManifest_: verifier init failed with public key");
                continue;  // Try next URL
            }
            verifier.update(reinterpret_cast<const uint8_t*>(manifestBuf), manifestLen);
            if (!verifier.verify(sigBuf, sigLen)) {
                ESP_LOGW(TAG, "fetchAndVerifyManifest_: SIGNATURE VERIFICATION FAILED for URL: %s "
                              "— manifest is untrusted", manifestUrl);
                continue;  // Try next URL
            }
            ESP_LOGI(TAG, "fetchAndVerifyManifest_: signature verified successfully");
        } else {
            ESP_LOGW(TAG, "fetchAndVerifyManifest_: no otaPublicKey configured — skipping signature verification");
        }

        // --- (d) Parse manifest ---
        memset(&manifest, 0, sizeof(manifest));
        if (!parseManifest(manifestBuf, manifestLen, manifest)) {
            ESP_LOGE(TAG, "fetchAndVerifyManifest_: manifest parse failed for URL: %s", manifestUrl);
            continue;  // Try next URL
        }

        ESP_LOGI(TAG, "fetchAndVerifyManifest_: manifest parsed — version=%d, channels=%d",
                 manifest.manifestVersion, manifest.channelCount);

        // --- Log fallback if learned URL failed but compiled-in succeeded ---
        if (i > 0 && _learnedManifestUrl[0] != '\0') {
            ESP_LOGW(TAG, "fetchAndVerifyManifest_: learned URL failed, fell back to compiled-in URL");
        }

        // --- Update learned URL if manifest contains a new manifestUrl ---
        if (manifest.manifestUrl[0] != '\0' &&
            strcmp(manifest.manifestUrl, _learnedManifestUrl) != 0) {
            ESP_LOGI(TAG, "fetchAndVerifyManifest_: updating learned URL '%s' → '%s'",
                     _learnedManifestUrl, manifest.manifestUrl);
            strlcpy(_learnedManifestUrl, manifest.manifestUrl, sizeof(_learnedManifestUrl));

            // Persist the updated learned URL via config save
            if (_configManager != nullptr) {
                _configManager->saveModuleConfig(static_cast<IConfigurable*>(this));
                ESP_LOGI(TAG, "fetchAndVerifyManifest_: persisted updated learned manifest URL");
            }
        }

        return true;  // Success!
    }

    // --- All URLs exhausted — fail-closed ---
    ESP_LOGE(TAG, "fetchAndVerifyManifest_: all manifest URL attempts exhausted — aborting");
    _state = OTAState::Error;
    _lastOtaError = "Manifest fetch/verify failed for all URLs";
    publishProgress(0.0f, "Manifest fetch failed", true, false);
    return false;

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
}

/**
 * @brief Writes data from the provided stream directly into the OTA partition.
 */
size_t OTAUpdateManager::writeStream(Stream& dataStream) {
    size_t written = Update.writeStream(dataStream);
    ESP_LOGD(TAG, "writeStream: %u bytes written", (unsigned)written);
    return written;
}

/**
 * @brief Finalizes the OTA update. Checks that total bytes and MD5 match.
 */
bool OTAUpdateManager::endUpdate() {
    if (Update.end()) {
        ESP_LOGI(TAG, "OTA update finalized successfully");
        return true;
    }
    ESP_LOGE(TAG, "OTA update finalization failed");
    return false;
}

/**
 * @brief Blesses the current running firmware by cancelling auto-rollback.
 */
void OTAUpdateManager::markAppValid() {
    ESP_LOGI(TAG, "Marking firmware as valid (blessing app)");
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
}

void OTAUpdateManager::setPreRebootHook(std::function<void()> hook) {
    _preRebootHook = std::move(hook);
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
}

/**
 * @brief Triggers a rollback to the previous firmware partition and reboots.
 */
void OTAUpdateManager::rollbackFirmware() {
    ESP_LOGW(TAG, "Firmware rollback triggered");
    const esp_partition_t* updatePartition = esp_ota_get_next_update_partition(NULL);

    if (updatePartition) {
        esp_ota_set_boot_partition(updatePartition);
        ESP.restart();
    }
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
}

void OTAUpdateManager::registerRoutes(PsychicHttpServer& server) {
    server.on("/ota.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, ota_js_gz, ota_js_gz_len, ota_js_gz_etag, "application/javascript");
    });

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
                            _configManager->saveModuleConfig(static_cast<IConfigurable*>(this));
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
                            _configManager->saveModuleConfig(static_cast<IConfigurable*>(this));
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
                            _configManager->saveModuleConfig(static_cast<IConfigurable*>(this));
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
                                _configManager->saveModuleConfig(static_cast<IConfigurable*>(this));
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
    // --- ota_get_status: read-only status snapshot ---
    registry.addTool("ota_get_status", "Get OTA update manager status, firmware version, channel, and timing",
        [](void* context, JsonObject& args, JsonObject& result) {
            OTAUpdateManager* self = static_cast<OTAUpdateManager*>(context);
            result["state"]         = otaStateToString(self->_state);
            result["version"]       = BUILD_VERSION;
            result["channel"]       = self->_channel;
            result["last_check_ms"] = (self->_lastCheckTimer.getLastFire() > 0)
                                     ? static_cast<long>(millis() - self->_lastCheckTimer.getLastFire())
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
}

#endif // WITH_OTA
