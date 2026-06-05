/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/OTAUpdateManager/OTAUpdateManager.hpp
 * Description: Full OTA orchestrator for firmware updates via GitHub releases.
 *              Wires the OTADownloadEngine (redirect-aware download + flash),
 *              IOTAVerifier (cryptographic signature verification via MbedTLS),
 *              and GithubClient (release metadata queries) into a single,
 *              event-driven pipeline. Supports both direct web-upload OTA
 *              (legacy beginUpdate/writeStream/endUpdate) and the new
 *              GitHub-based checkForUpdate/performUpdate workflow.
 *
 *              Download runs in a FreeRTOS task to respect the tick() ≤ 10ms
 *              constraint. Progress events are published to the EventBus on
 *              the "ota.progress" topic using OTAProgressEvent, which is
 *              layout-compatible with OTAProgressPayload in u8g2DisplayModule.
 *
 * Exported Classes:
 * - OTAState: Lifecycle state enum for the OTA pipeline
 * - OTAProgressEvent: EventBus payload struct for OTA progress events
 * - OTAUpdateManager: Full OTA orchestrator service class
 *
 *   Orchestrator API:
 *   - begin():                Initialize with Application dependencies
 *   - checkForUpdate():       Query GitHub for a newer firmware release
 *   - performUpdate():        Launch the full download→verify→flash pipeline in a FreeRTOS task
 *   - getState():             Query the current OTA state machine state
 *   - getLastOtaError():      Retrieve the last OTA error string
 *
 *   Legacy Direct-Upload API:
 *   - beginUpdate():          Initiate direct OTA update with size and MD5
 *   - writeStream():          Stream firmware data to the flash partition
 *   - endUpdate():            Finalize the direct OTA update
 *   - markAppValid():         Bless the running firmware, cancelling auto-rollback
 *   - rollbackFirmware():     Revert to the previous firmware partition and reboot
 *
 *   IWebContributor (tab: "OTA", parent: "System", order: 50):
 *   - registerRoutes():       Register REST API routes
 *   - getTabName():           Return "OTA" for portal tab
 *   - getParentTab():         Return "System" for navigation hierarchy
 *   - getTabOrder():          Return 50 for pill ordering
 *
 *   IConfigurable (namespace: "ota", schema v1):
 *   - getConfigNamespace():   Return "ota"
 *   - getSchemaVersion():     Return 1
 *   - applyConfig():          Apply OTA config from JSON section
 *   - serializeConfig():      Serialize OTA config to JSON section
 *   - migrateConfig():        Migrate from older schema version
 *   - resetConfig():          Restore OTA config factory defaults
 */

#pragma once

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <Stream.h>
#else
#include <cstddef>  // for size_t
#include <stdint.h>
// Minimal Stream stub for NATIVE_BUILD
class Stream {};
#endif

#include "../../include/interfaces/IWebContributor.hpp"
#include "../../include/interfaces/IConfigurable.hpp"

// ---------------------------------------------------------------------------
// Forward declarations — Hollywood Principle: never include other module headers
// ---------------------------------------------------------------------------
class Application;
class EventBus;
class GithubClient;
class OTADownloadEngine;
class IOTAVerifier;
class MbedTLSSignatureVerifier;

// ---------------------------------------------------------------------------
// OTA State Machine
// ---------------------------------------------------------------------------

/**
 * @brief Lifecycle state for the OTA pipeline state machine.
 *
 * Transitions: Idle → Checking → Idle (no update)
 *              Idle → Downloading → Verifying → Complete → (reboot)
 *              Any → Error (on failure)
 */
enum class OTAState : uint8_t {
    Idle,         // No OTA activity — ready to accept a check or update
    Checking,     // Querying GitHub for latest release
    Downloading,  // FreeRTOS task running the download pipeline
    Verifying,    // Signature verification in progress
    Complete,     // Update finalized — waiting for reboot
    Error,        // An error occurred
};

// ---------------------------------------------------------------------------
// EventBus Payload
// ---------------------------------------------------------------------------

/**
 * @brief EventBus payload for "ota.progress" topic.
 *
 * Layout-compatible with OTAProgressPayload in U8G2DisplayModule.hpp.
 * The display module's subscriber can safely cast the EventBus payload
 * to either type for rendering OTA progress on-screen.
 */
struct OTAProgressEvent {
    float progress;       // 0.0 to 1.0 download fraction
    const char* status;   // Human-readable status message
    bool complete;        // True when OTA cycle is finished
    bool success;         // True if OTA succeeded (only valid when complete)
};

// ---------------------------------------------------------------------------
// OTA Update Manager
// ---------------------------------------------------------------------------

/**
 * @brief Full OTA orchestrator for GitHub-based firmware updates.
 *
 * Inherits IWebContributor for REST API route registration and IConfigurable
 * for persisted configuration via ConfigManager. Manages the full lifecycle
 * from release check through download, verification, flash, and reboot.
 * Also retains the legacy direct-upload path for web-based OTA.
 *
 * IConfigurable fields (namespace: "ota", schema version: 1):
 *   - channel   (char[16]): OTA release channel ("stable" or "beta")
 *   - autoCheck (bool):     Whether to auto-check for updates on boot
 */
class OTAUpdateManager : public IWebContributor, public IConfigurable {
public:
    /**
     * @brief Default constructor.
     */
    OTAUpdateManager();

    // -------------------------------------------------------------------------
    // New Orchestrator API
    // -------------------------------------------------------------------------

    /**
     * @brief Initialize the OTA manager with dependencies from the Application.
     *
     * Must be called before checkForUpdate() or performUpdate(). Stores the
     * Application reference for EventBus and config access.
     *
     * @param app Reference to the Application composition root.
     */
    void begin(Application& app);

    /**
     * @brief Check GitHub for a newer firmware version. Non-blocking.
     *
     * Creates a GithubClient with the repo and token from FrameworkConfig,
     * queries the latest release, and compares release IDs. On native builds,
     * always returns false.
     *
     * @return true if a new version is available, false otherwise.
     */
    bool checkForUpdate();

    /**
     * @brief Start the full OTA download + verify + flash pipeline.
     *
     * Launches a FreeRTOS task — returns immediately. Check getState()
     * for progress. On native builds, sets state to Error (no FreeRTOS).
     */
    void performUpdate();

    /**
     * @brief Get the current OTA state machine state.
     * @return Current OTAState value.
     */
    OTAState getState() const;

    /**
     * @brief Get the last OTA error string (from OTADownloadEngine).
     * @return Null-terminated C-string, or nullptr if no error.
     */
    const char* getLastOtaError() const;

    // -------------------------------------------------------------------------
    // Legacy Direct-Upload API (preserved for web upload route)
    // -------------------------------------------------------------------------

    /**
     * @brief Initiates the OTA update process (direct upload path).
     * @param size The total size of the firmware binary.
     * @param expectedMD5 The expected MD5 hash of the firmware for validation.
     * @return True if initialization succeeds.
     */
    bool beginUpdate(size_t size, const char* expectedMD5);

    /**
     * @brief Writes data from a stream to the OTA partition.
     * @param dataStream The stream containing the firmware binary.
     * @return The number of bytes written.
     */
    size_t writeStream(Stream& dataStream);

    /**
     * @brief Finalizes the OTA update and checks for MD5 validation success.
     * @return True if the update is complete and valid.
     */
    bool endUpdate();

    /**
     * @brief Blesses the current running firmware, cancelling any pending rollback.
     *        Should be called after critical subsystems (WiFi, FS) are operational.
     */
    void markAppValid();

    /**
     * @brief Triggers a rollback to the previous firmware partition and reboots.
     */
    void rollbackFirmware();

    // -------------------------------------------------------------------------
    // IWebContributor Interface
    // -------------------------------------------------------------------------

    /**
     * @brief Register all REST API route handlers with the web server.
     *
     * Registers the legacy OTA routes AND new GitHub-based update routes:
     *   POST /api/ota { "action": "checkGithub" }  → checkForUpdate() + performUpdate()
     *   POST /api/ota { "action": "status" }        → returns JSON with state, lastError
     *
     * @param server Reference to the AsyncWebServer instance.
     */
    void registerRoutes(AsyncWebServer& server) override;

    /**
     * @brief Get the UI tab name for this contributor.
     * @return String name of the tab.
     */
    const char* getTabName() const override { return "OTA"; }

    /**
     * @brief Get the parent tab name for hierarchical navigation.
     *
     * OTA is a sub-section of the "System" top-level tab, alongside
     * ConfigManager (order 20). This groups all system-level
     * contributors under a single bottom-nav item.
     *
     * @return "System"
     */
    const char* getParentTab() const override { return "System"; }

    /**
     * @brief Get the display order within the System sub-navigation.
     *
     * Positioned after Backup (40) in the System pill bar:
     * Status=10, Keys=20, MCP=30, Backup=40, OTA=50, Screenshot=60.
     *
     * @return 50
     */
    int getTabOrder() const override { return 50; }

    // -------------------------------------------------------------------------
    // IConfigurable Interface
    // -------------------------------------------------------------------------

    /**
     * @brief Return the unique config namespace for OTA settings.
     * @return "ota" — maps to /mod/ota.json on device filesystem.
     */
    const char* getConfigNamespace() const override;

    /**
     * @brief Return the current schema version for the OTA config format.
     * @return 1 — initial schema version.
     */
    int getSchemaVersion() const override;

    /**
     * @brief Apply OTA configuration values from a parsed JSON section.
     *
     * Reads "channel" and "autoCheck" from the provided section and applies
     * them to the internal state. Missing keys fall back to safe defaults.
     *
     * @param section Read-only reference to the JSON object containing config keys.
     */
    void applyConfig(const JsonObject& section) override;

    /**
     * @brief Serialize current OTA configuration into a JSON section.
     *
     * Writes "channel" and "autoCheck" into the provided section for
     * persistence by ConfigManager.
     *
     * @param section Mutable reference to the target JSON object.
     */
    void serializeConfig(JsonObject& section) const override;

    /**
     * @brief Migrate OTA configuration from an older schema version.
     *
     * Logs the migration attempt and returns true (no migration logic required
     * at schema version 1). Override in future versions when fields change.
     *
     * @param section   Mutable reference to the JSON config section.
     * @param fromVersion The stored schema version being migrated from.
     * @return true — migration always succeeds for v1.
     */
    bool migrateConfig(JsonObject& section, int fromVersion) override;

    /**
     * @brief Reset OTA configuration to factory defaults.
     *
     * Sets channel to "stable" and autoCheck to true.
     */
    void resetConfig() override;

private:
    // -------------------------------------------------------------------------
    // IConfigurable config state
    // -------------------------------------------------------------------------
    char _channel[16] = "stable";  ///< OTA release channel ("stable" or "beta")
    bool _autoCheck   = true;      ///< Whether to auto-check for updates on boot

    // -------------------------------------------------------------------------
    // Legacy state
    // -------------------------------------------------------------------------
    size_t expectedSize_ = 0;  // Tracks the total expected size for direct uploads

    // -------------------------------------------------------------------------
    // Orchestrator state
    // -------------------------------------------------------------------------
    Application*  app_          = nullptr;             // Composition root reference
    OTAState      state_        = OTAState::Idle;      // Current state machine state
    const char*   lastOtaError_ = nullptr;             // Last error string from download engine

#ifndef NATIVE_BUILD
    TaskHandle_t  taskHandle_   = nullptr;             // FreeRTOS task handle for download
#else
    void*         taskHandle_   = nullptr;             // Stub for native builds
#endif

    // -------------------------------------------------------------------------
    // FreeRTOS task plumbing
    // -------------------------------------------------------------------------

    /**
     * @brief Static FreeRTOS task function — delegates to performUpdateTask().
     * @param param Pointer to the OTAUpdateManager instance.
     */
    static void otaTaskWrapper(void* param);

    /**
     * @brief Instance method running the full OTA pipeline inside the FreeRTOS task.
     *
     * Steps: GitHub check → find .bin/.sig assets → init verifier → allocate
     * WiFiClientSecure → configure engine → fetch signature → download & flash
     * → finalize → reboot.
     */
    void performUpdateTask();

    /**
     * @brief Check and update the NVS security version counter.
     *
     * Compares the compile-time SECURITY_VERSION against the stored NVS
     * value. Updates NVS if the compiled version is higher. Called from
     * markAppValid() before blessing the partition.
     */
    void checkSecurityVersion();

    /**
     * @brief Publish an OTA progress event to the EventBus.
     *
     * Creates an OTAProgressEvent on the stack and publishes it on the
     * "ota.progress" topic. Also logs the progress at DEBUG level.
     *
     * @param progress Download fraction (0.0 to 1.0).
     * @param status   Human-readable status message.
     * @param complete True when OTA cycle is finished.
     * @param success  True if OTA succeeded.
     */
    void publishProgress(float progress, const char* status, bool complete, bool success);
};

