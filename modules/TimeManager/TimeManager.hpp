/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/timeManager/TimeManager.hpp
 * Description: Manages NTP synchronization, POSIX timezone rules, and non-blocking time tick updates.
 *              Implements IConfigurable for persistent timezone/NTP config via ConfigManager
 *              and IWebContributor for the "Time" sub-pill in the System tab portal.
 *
 * Exported Functions/Classes:
 * - TimeSyncStatus: Enum representing the current NTP synchronization status.
 * - TimeManager: Class managing time sync and rules.
 *
 *   Lifecycle:
 *   - begin():              Initializes NTP config and timezone.
 *   - tick():               Non-blocking update routine to be called in main loop.  [ITickable]
 *
 *   Time Query API:
 *   - setTimezone():        Sets POSIX timezone string.
 *   - getLocalTime():       Gets current local time safely.
 *   - isTimeSet():          Returns true if time has been synchronized.
 *   - isConfigured():       Returns true if user has explicitly configured timezone.
 *
 *   IConfigurable (namespace: "time", schema v1):
 *   - getConfigNamespace(): Return "time"
 *   - getSchemaVersion():   Return 1
 *   - applyConfig():        Apply time config from JSON section
 *   - serializeConfig():    Serialize time config to JSON section
 *   - resetConfig():        Clear to defaults
 *
 *   IWebContributor (tab: "Time", parent: "System", order: 15):
 *   - registerRoutes():     Register REST API endpoints for time management
 *   - getTabName():         Return "Time" for portal tab
 *   - getParentTab():       Return "System" for navigation hierarchy
 *   - getTabOrder():        Return 15 for pill ordering
 */

#pragma once

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <time.h>
#include <esp_sntp.h>
#else
#include <string>
#include <stdint.h>
#include <time.h>
#endif

#include "../../include/interfaces/ITickable.hpp"
#include "../../include/interfaces/IConfigurable.hpp"
#include "../../include/interfaces/IWebContributor.hpp"

// Forward declaration — avoids circular include with configManager.hpp
class ConfigManager;

// Forward declaration — full class defined by PsychicHttp (hardware)
class PsychicRequest;
class PsychicResponse;
class PsychicHttpServer;

/**
 * @brief Enum representing the current NTP synchronization status
 */
enum class TimeSyncStatus {
    UNSYNCED,
    IN_PROGRESS,
    SYNCED
};

/**
 * @brief Kernel service that manages NTP synchronization and POSIX timezone rules.
 *
 * Implements ITickable so it can be registered in the Application tick dispatch
 * vector alongside other periodic services. Implements IConfigurable for
 * persistent timezone/NTP server configuration via the ConfigManager gateway.
 * Implements IWebContributor for the "Time" sub-pill in the portal.
 */
class TimeManager : public ITickable, public IConfigurable, public IWebContributor {
public:
    TimeManager() = default;
    ~TimeManager() override = default;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Initializes NTP config and sets the POSIX timezone.
     *
     * Called during the Application boot sequence after WiFi.begin().
     * Uses the effective POSIX timezone — either a persisted value already
     * loaded by applyConfig(), or the factory default seeded by resetConfig()
     * ("GMT0"). NTP servers are taken from member variables (populated by
     * applyConfig() or defaults).
     */
    void begin();

    /**
     * @brief Non-blocking update routine to be called in main loop.
     *
     * Handles the NTP state machine: polls SNTP status until synced,
     * then periodically re-syncs every RESYNC_INTERVAL_MS.
     */
    void tick() override;

    // -------------------------------------------------------------------------
    // IConfigurable Interface (ConfigManager Gateway)
    // -------------------------------------------------------------------------

    /**
     * @brief Return the config namespace for time settings.
     * @return "time"
     */
    const char* getConfigNamespace() const override { return "time"; }

    /**
     * @brief Return the current schema version for time config.
     * @return 1
     */
    int getSchemaVersion() const override { return 1; }

    /**
     * @brief Apply time configuration from a parsed JSON section.
     *
     * Reads timezone (IANA name + POSIX string) and NTP servers from the
     * persisted config. Calls setTimezone() to apply immediately.
     *
     * @param section Read-only JSON object containing config keys.
     */
    void applyConfig(const JsonObject& section) override;

    /**
     * @brief Serialize current time configuration into a JSON section.
     *
     * Writes timezone (IANA + POSIX), NTP servers, and configured flag.
     *
     * @param section Mutable JSON object to write config into.
     */
    void serializeConfig(JsonObject& section) const override;

    /**
     * @brief Reset time configuration to factory defaults.
     */
    void resetConfig() override;

    // -------------------------------------------------------------------------
    // IWebContributor Interface
    // -------------------------------------------------------------------------

    /**
     * @brief Register the TimeManager's REST API endpoints.
     * @param server Reference to the PsychicHttpServer instance.
     */
    void registerRoutes(PsychicHttpServer& server) override;

    /**
     * @brief Get the UI tab name for this contributor.
     * @return "Time"
     */
    const char* getTabName() const override { return "Time"; }

    /**
     * @brief Get the parent tab for navigation hierarchy.
     * @return "System" — Time appears as a sub-section of the System tab.
     */
    const char* getParentTab() const override { return "System"; }

    /**
     * @brief Get the tab display order within the System sub-navigation.
     * @return 15 — positioned between Status (10) and Keys (20).
     */
    int getTabOrder() const override { return 15; }

    // -------------------------------------------------------------------------
    // Time Query API
    // -------------------------------------------------------------------------

    /**
     * @brief Sets POSIX timezone string and applies it immediately.
     * @param timezone POSIX timezone string.
     */
    void setTimezone(const char* timezone);

    /**
     * @brief Gets current local time safely.
     * @param timeinfo Pointer to a tm struct to populate.
     * @return true if time is valid and populated, false otherwise.
     */
    bool getLocalTime(struct tm* timeinfo);

    /**
     * @brief Returns true if time has been successfully synchronized.
     * @return true if time is set.
     */
    bool isTimeSet() const;

    /**
     * @brief Returns true if the user has explicitly configured timezone.
     *
     * This is the primary check for TimeConfigGate. Returns true when
     * applyConfig() has been called with a persisted config section
     * (i.e., /mod/time.json exists and was loaded).
     *
     * @return true if timezone has been explicitly configured.
     */
    bool isConfigured() const { return _configured; }

    /**
     * @brief Get the current IANA timezone name.
     * @return IANA timezone string (e.g. "Europe/London"), or empty if not configured.
     */
    const char* getTimezone() const { return _timezone; }

    /**
     * @brief Get the current POSIX timezone rule string.
     * @return POSIX timezone string (e.g. "GMT0BST,M3.5.0/1,M10.5.0").
     */
    const char* getPosixTz() const { return _posixTz; }

    /**
     * @brief Get the current NTP sync status.
     * @return Current TimeSyncStatus value.
     */
    TimeSyncStatus getSyncStatus() const { return _syncStatus; }

    // -------------------------------------------------------------------------
    // Dependency Injection
    // -------------------------------------------------------------------------

    /**
     * @brief Wire the ConfigManager pointer for save operations.
     * @param cm Pointer to the ConfigManager instance.
     */
    void setConfigManager(ConfigManager* cm) { _configManager = cm; }

private:
    // -------------------------------------------------------------------------
    // Configuration State
    // -------------------------------------------------------------------------

    char _timezone[64] = "";                        ///< IANA timezone name (e.g. "Europe/London")
    char _posixTz[64]  = "";                        ///< POSIX timezone rule string
    char _ntpServer1[64] = "pool.ntp.org";          ///< Primary NTP server
    char _ntpServer2[64] = "time.nist.gov";         ///< Secondary NTP server
    char _ntpServer3[64] = "time.windows.com";      ///< Tertiary NTP server
    bool _configured = false;                       ///< True when user has explicitly set timezone

    // -------------------------------------------------------------------------
    // Sync State
    // -------------------------------------------------------------------------

    TimeSyncStatus _syncStatus = TimeSyncStatus::UNSYNCED; ///< Current NTP sync status
    unsigned long _lastSyncTime = 0;                       ///< millis() timestamp of last successful sync

    /// Re-sync interval: 6 hours (in milliseconds)
    static constexpr unsigned long RESYNC_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;

    // -------------------------------------------------------------------------
    // Dependencies
    // -------------------------------------------------------------------------

    ConfigManager* _configManager = nullptr; ///< Pointer to ConfigManager for save operations

    // -------------------------------------------------------------------------
    // Private Helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Reinitialize the SNTP client with current NTP server settings.
     *
     * Called after applyConfig() or when NTP servers are changed via the portal.
     * Resets _syncStatus to IN_PROGRESS so tick() begins polling again.
     */
    void _reinitNtp();

#ifndef NATIVE_BUILD
    /**
     * @brief Handler for GET /api/time/config
     */
    void _handleGetConfig(PsychicRequest* request, PsychicResponse* response);

    /**
     * @brief Handler for POST /api/time/config
     */
    void _handlePostConfig(PsychicRequest* request, PsychicResponse* response);

    /**
     * @brief Handler for GET /api/time/zones
     */
    void _handleGetZones(PsychicRequest* request, PsychicResponse* response);

    /**
     * @brief Handler for POST /api/time/zones/update
     */
    void _handlePostZonesUpdate(PsychicRequest* request, PsychicResponse* response);
#endif
};
