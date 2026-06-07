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
 *   IConfigurable (namespace: "ota", schema v2):
 *   - getConfigNamespace():   Return "ota"
 *   - getSchemaVersion():     Return 2
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
#include <string>
#include <cstddef>  // for size_t
#include <stdint.h>
// Stub String class for NATIVE_BUILD tests
#ifndef STRING_STUB_DEFINED
#define STRING_STUB_DEFINED
class String {
public:
    String(const char* s) : str(s ? s : "") {}
    String(const std::string& s) : str(s) {}
    String() {}
    const char* c_str() const { return str.c_str(); }
    bool isEmpty() const { return str.empty(); }
    int length() const { return (int)str.size(); }
    char operator[](int i) const { return str[i]; }
    bool operator==(const char* s) const { return str == s; }
    bool operator==(const String& s) const { return str == s.str; }
    String operator+(const char* s) const { return String(str + s); }
    String operator+(const String& s) const { return String(str + s.str); }
private:
    std::string str;
};
#endif
// Minimal Stream stub for NATIVE_BUILD
class Stream {};
#endif

#include "../../include/interfaces/IWebContributor.hpp"
#include "../../include/interfaces/IConfigurable.hpp"
#include "../../include/interfaces/ITickable.hpp"

// ---------------------------------------------------------------------------
// Forward declarations — Hollywood Principle: never include other module headers
// ---------------------------------------------------------------------------
class Application;
class EventBus;
class GithubClient;
class OTADownloadEngine;
class IOTAVerifier;
class MbedTLSSignatureVerifier;
class AsyncWebServerRequest;

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
 * IConfigurable fields (namespace: "ota", schema version: 2):
 *   - channel            (char[16]): OTA release channel ("stable", "beta", "dev")
 *   - autoCheck          (bool):     Whether to auto-check for updates on boot
 *   - lastCheckedChannel (char[16]): Last channel used for an update check
 */
class OTAUpdateManager : public IWebContributor, public IConfigurable, public ITickable {
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

    // --- ITickable ---
    void tick() override;

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
     * @return 2 — adds lastCheckedChannel for implicit downgrade detection.
     */
    int getSchemaVersion() const override;

    /**
     * @brief Apply OTA configuration values from a parsed JSON section.
     *
     * Reads "channel", "autoCheck", and "lastCheckedChannel" from the
     * provided section and applies them to internal state. Missing keys
     * fall back to safe defaults.
     *
     * @param section Read-only reference to the JSON object containing config keys.
     */
    void applyConfig(const JsonObject& section) override;

    /**
     * @brief Serialize current OTA configuration into a JSON section.
     *
     * Writes "channel", "autoCheck", and "lastCheckedChannel" into the
     * provided section for persistence by ConfigManager.
     *
     * @param section Mutable reference to the target JSON object.
     */
    void serializeConfig(JsonObject& section) const override;

    /**
     * @brief Migrate OTA configuration from an older schema version.
     *
     * Handles v1→v2 migration by adding the lastCheckedChannel field.
     *
     * @param section   Mutable reference to the JSON config section.
     * @param fromVersion The stored schema version being migrated from.
     * @return true on success.
     */
    bool migrateConfig(JsonObject& section, int fromVersion) override;

    /**
     * @brief Reset OTA configuration to factory defaults.
     *
     * Sets channel to "stable", autoCheck to true, and clears
     * lastCheckedChannel.
     */
    void resetConfig() override;

private:
    // -------------------------------------------------------------------------
    // IConfigurable config state
    // -------------------------------------------------------------------------
    char    _channel[16]            = "stable";
    bool    _autoCheck              = true;
    char    _lastCheckedChannel[16] = "";
    uint8_t _checkHour              = 3;   ///< Hour (0-23) at which daily auto-check fires

    // Scheduling state (not persisted — reset on each boot)
    uint32_t _lastCheckDay   = 0;     ///< Unix day of last scheduled check (seconds/86400)
    bool     _hasBootChecked = false; ///< Set true once the boot auto-check has fired
    bool     _silentCheck    = false; ///< True for auto-checks — suppresses transient error modals

    // -------------------------------------------------------------------------
    // Legacy state
    // -------------------------------------------------------------------------
    size_t expectedSize_ = 0;  // Tracks the total expected size for direct uploads

    // -------------------------------------------------------------------------
    // Signed web-upload state (per-request, reset at upload start)
    // -------------------------------------------------------------------------
    uint8_t  uploadSigBuf_[512] = {};   ///< Buffer for the .sig bytes from multipart upload
    size_t   uploadSigLen_   = 0;        ///< Number of valid bytes in uploadSigBuf_
    size_t   uploadBinSize_  = 0;        ///< Total firmware bytes received so far
    bool     uploadAborted_  = false;    ///< Set true when a security check fails mid-upload
    MbedTLSSignatureVerifier* uploadVerifier_ = nullptr; ///< Heap-allocated per upload, freed after use

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

    /**
     * @brief AsyncWebServer multipart upload handler for POST /api/ota/upload.
     *
     * Invoked per-chunk by ESPAsyncWebServer. Accumulates firmware bytes
     * through MbedTLSSignatureVerifier::update() and collects the signature
     * field bytes into uploadSigBuf_. On the final chunk, calls verify()
     * before writing to flash with the Update library.
     *
     * @param request  Incoming HTTP request context.
     * @param filename Multipart field name ("firmware" or "signature").
     * @param index    Byte offset of this chunk within the field.
     * @param data     Pointer to chunk data.
     * @param len      Number of bytes in this chunk.
     * @param final    True when this is the last chunk for this field.
     */
    void handleSignedUpload(AsyncWebServerRequest* request,
                            const String& filename,
                            size_t index, uint8_t* data, size_t len, bool final);
};

