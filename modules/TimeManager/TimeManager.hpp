/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/TimeManager/TimeManager.hpp
 * Description: Manages NTP synchronization (including RFC 2132 DHCP Option 42),
 *              POSIX/IANA timezone rules (including RFC 4833 DHCP Options 100/101),
 *              and non-blocking time tick updates.
 *              Implements IConfigurable for persistent timezone/NTP config (schema v2)
 *              via ConfigManager and IWebContributor for the "Time" sub-pill in the
 *              System tab portal.
 *
 * Exported Functions/Classes:
 * - TimeSyncStatus: Enum representing the current NTP synchronization status.
 * - DhcpTzSource: Enum representing the active DHCP timezone source (None, PCode, TCode).
 * - TimeManager: Class managing time sync, DHCP NTP/TZ discovery, and rules.
 *
 *   Lifecycle:
 *   - begin():              Initializes NTP config (and DHCP SNTP mode) and timezone.
 *   - tick():               Drains DHCP timezone staging, refreshes Option 42 slots,
 *                           and runs the non-blocking NTP sync state machine. [ITickable]
 *
 *   Time Query & Telemetry API:
 *   - setTimezone():        Sets POSIX timezone string.
 *   - getLocalTime():       Gets current local time safely.
 *   - isTimeSet():          Returns true if time has been synchronized.
 *   - isConfigured():       Returns true if user configured timezone or DHCP TZ is active.
 *   - getTimezone():        Returns active IANA timezone name (DHCP override or fallback).
 *   - getPosixTz():         Returns active POSIX timezone rule (DHCP override or fallback).
 *   - getConfiguredTimezone(): Returns persisted fallback IANA timezone name.
 *   - getConfiguredPosixTz():  Returns persisted fallback POSIX timezone string.
 *   - isDhcpNtpEnabled():   Returns true if DHCP Option 42 NTP acquisition is enabled.
 *   - isDhcpNtpActive():    Returns true if at least one DHCP Option 42 server is active.
 *   - getDhcpNtpServerCount(): Returns count of discovered DHCP Option 42 NTP servers.
 *   - getDhcpNtpServer():   Returns discovered DHCP Option 42 server IP string at index.
 *   - isDhcpTzEnabled():    Returns true if DHCP Option 100/101 timezone discovery is enabled.
 *   - isDhcpTzActive():     Returns true if a DHCP-supplied timezone override is active.
 *   - getDhcpTzSource():    Returns active DhcpTzSource enum value.
 *   - getDhcpTzSourceStr(): Returns "none", "pcode", or "tcode".
 *
 *   IConfigurable (namespace: "time", schema v2):
 *   - getConfigNamespace(): Return "time"
 *   - getSchemaVersion():   Return 2
 *   - migrateConfig():      Migrate v1 config to v2
 *   - applyConfig():        Apply time config from JSON section
 *   - serializeConfig():    Serialize persisted time config to JSON section
 *   - resetConfig():        Clear to defaults
 *
 *   IWebContributor (tab: "Time", parent: "System", order: 15):
 *   - registerRoutes():     Register REST API endpoints for time management
 *   - registerTabs():       Register "Time" tab (parent: "System", order: 15) via ITabRegistry.
 */

#pragma once

#include <Arduino.h>
#include <time.h>
#include <esp_sntp.h>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "interfaces/ITickable.hpp"
#include "interfaces/IConfigurable.hpp"
#include "interfaces/IWebContributor.hpp"

// Forward declarations — avoids circular includes
class ConfigManager;
class EventBus;

// Forward declarations — full classes defined by PsychicHttp
class PsychicRequest;
class PsychicResponse;
class PsychicHttpServer;

/**
 * @brief Enum representing the current NTP synchronization status.
 */
enum class TimeSyncStatus {
    UNSYNCED,
    IN_PROGRESS,
    SYNCED
};

/**
 * @brief Source of an active RFC 4833 DHCP timezone override.
 */
enum class DhcpTzSource {
    None,   ///< No DHCP timezone active (using persisted fallback)
    PCode,  ///< RFC 4833 Option 100 direct POSIX TZ string
    TCode   ///< RFC 4833 Option 101 IANA name resolved via /timezones.json
};

/**
 * @brief Kernel service that manages NTP synchronization and POSIX timezone rules.
 *
 * Implements ITickable so it can be registered in the Application tick dispatch
 * vector alongside other periodic services. Implements IConfigurable (schema v2)
 * for persistent timezone/NTP server and DHCP opt-in configuration via ConfigManager.
 * Implements IWebContributor for the "Time" sub-pill in the portal.
 */
class TimeManager : public ITickable, public IConfigurable, public IWebContributor {
public:
    /// Maximum SNTP server slots inspected for DHCP Option 42 and fallback preservation.
    static constexpr size_t MAX_SNTP_SLOTS = 3;

    TimeManager() = default;
    ~TimeManager() override = default;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Initializes NTP config, DHCP SNTP mode, and sets the active POSIX timezone.
     *
     * Called during the Application boot sequence after WiFi.begin().
     */
    void begin();

    /**
     * @brief Non-blocking update routine to be called in main loop.
     *
     * Drains pending RFC 4833 DHCP timezone payloads, refreshes Option 42 SNTP
     * fallback slots, and drives the NTP synchronization state machine.
     */
    void tick(uint32_t currentMillis) override;

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
     * @return 2
     */
    int getSchemaVersion() const override { return 2; }

    /**
     * @brief Migrate a persisted v1 time configuration section to schema v2.
     *
     * Seeds dhcpNtp=true when missing and sets dhcpTz=false when the v1 config
     * has an explicit user-configured timezone (preserving user choice across OTA),
     * or dhcpTz=true otherwise.
     *
     * @param section     Mutable JSON config section.
     * @param fromVersion Stored schema version being migrated from.
     * @return true on success.
     */
    bool migrateConfig(JsonObject& section, int fromVersion) override;

    /**
     * @brief Apply time configuration from a parsed JSON section.
     *
     * Reads persisted fallback timezone, NTP servers, and dhcpNtp/dhcpTz flags.
     *
     * @param section Read-only JSON object containing config keys.
     */
    void applyConfig(const JsonObject& section) override;

    /**
     * @brief Serialize persisted time configuration into a JSON section.
     *
     * Writes only persisted fallback fields and DHCP opt-in toggles (never
     * volatile DHCP-acquired runtime servers or timezone overrides).
     *
     * @param section Mutable JSON object to write config into.
     */
    void serializeConfig(JsonObject& section) const override;

    /**
     * @brief Reset time configuration and transient DHCP runtime state to factory defaults.
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
     * @brief Register the Time tab in the System sub-navigation.
     * @param registry The ITabRegistry to register tabs with.
     */
    void registerTabs(ITabRegistry& registry) override {
        registry.addTab("Time", "System", 15, nullptr, "/time.js");
    }

    /**
     * @brief Return the API path prefix for body-size middleware matching.
     * @return "/api/time"
     */
    const char* getApiPrefix() const override { return "/api/time"; }

    /**
     * @brief Maximum POST body size for time routes (32 KiB for timezone database upload).
     * @return 32768
     */
    size_t getMaxPostBody() const override { return 32768; }

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
     * @brief Returns true if timezone is configured (manually or via active DHCP TZ).
     * @return true if timezone is configured.
     */
    bool isConfigured() const { return _configured || (_dhcpTzActive && _dhcpTzEnabled); }

    /**
     * @brief Get the effective IANA timezone name (DHCP override if set, else configured fallback).
     * @return IANA timezone string (e.g. "Europe/London"), or empty if unavailable.
     */
    const char* getTimezone() const {
        return (_dhcpTzActive && _dhcpTzEnabled && _dhcpTimezone[0] != '\0')
            ? _dhcpTimezone : _timezone;
    }

    /**
     * @brief Get the effective POSIX timezone rule string (DHCP override if active, else configured fallback).
     * @return POSIX timezone string (e.g. "GMT0BST,M3.5.0/1,M10.5.0").
     */
    const char* getPosixTz() const {
        return (_dhcpTzActive && _dhcpTzEnabled) ? _dhcpPosixTz : _posixTz;
    }

    /**
     * @brief Get the persisted fallback IANA timezone name.
     * @return Configured fallback IANA timezone string.
     */
    const char* getConfiguredTimezone() const { return _timezone; }

    /**
     * @brief Get the persisted fallback POSIX timezone string.
     * @return Configured fallback POSIX timezone string.
     */
    const char* getConfiguredPosixTz() const { return _posixTz; }

    /**
     * @brief Get the current NTP sync status.
     * @return Current TimeSyncStatus value.
     */
    TimeSyncStatus getSyncStatus() const { return _syncStatus; }

    // -------------------------------------------------------------------------
    // DHCP NTP & Timezone Telemetry API
    // -------------------------------------------------------------------------

    /**
     * @brief Returns true if DHCP Option 42 NTP server discovery is enabled.
     */
    bool isDhcpNtpEnabled() const { return _dhcpNtpEnabled; }

    /**
     * @brief Returns true if one or more DHCP Option 42 NTP servers are currently active.
     */
    bool isDhcpNtpActive() const { return _dhcpNtpActive; }

    /**
     * @brief Returns the number of active DHCP Option 42 NTP servers (0..MAX_SNTP_SLOTS).
     */
    size_t getDhcpNtpServerCount() const { return _dhcpNtpCount; }

    /**
     * @brief Returns the active DHCP Option 42 NTP server IP string at @p idx, or "" if out of range.
     * @param idx Zero-based index (< getDhcpNtpServerCount()).
     */
    const char* getDhcpNtpServer(size_t idx) const {
        return (idx < _dhcpNtpCount) ? _dhcpNtpServers[idx] : "";
    }

    /**
     * @brief Returns true if RFC 4833 DHCP Option 100/101 timezone discovery is enabled.
     */
    bool isDhcpTzEnabled() const { return _dhcpTzEnabled; }

    /**
     * @brief Returns true if a DHCP-supplied timezone override is currently active.
     */
    bool isDhcpTzActive() const { return _dhcpTzActive; }

    /**
     * @brief Returns the active DHCP timezone source enum.
     */
    DhcpTzSource getDhcpTzSource() const { return _dhcpTzSource; }

    /**
     * @brief Returns the active DHCP timezone source string ("none", "pcode", or "tcode").
     */
    const char* getDhcpTzSourceStr() const;

    // -------------------------------------------------------------------------
    // Dependency Injection
    // -------------------------------------------------------------------------

    /**
     * @brief Wire the ConfigManager pointer for save operations.
     * @param cm Pointer to the ConfigManager instance.
     */
    void setConfigManager(ConfigManager* cm) { _configManager = cm; }

    /**
     * @brief Wire the EventBus pointer and subscribe once to WiFi connection events.
     * @param eb Pointer to the EventBus instance.
     */
    void setEventBus(EventBus* eb);

private:
    // -------------------------------------------------------------------------
    // Persisted Configuration State (NEVER mutated by runtime DHCP leases)
    // -------------------------------------------------------------------------

    char _timezone[64] = "";                        ///< Fallback IANA timezone name
    char _posixTz[64]  = "";                        ///< Fallback POSIX timezone rule string
    char _ntpServer1[64] = "pool.ntp.org";          ///< Primary static/fallback NTP server
    char _ntpServer2[64] = "time.nist.gov";         ///< Secondary static/fallback NTP server
    char _ntpServer3[64] = "time.windows.com";      ///< Tertiary static/fallback NTP server
    bool _configured = false;                       ///< True when persisted config was applied
    bool _dhcpNtpEnabled = true;                    ///< Enable RFC 2132 Option 42 SNTP discovery
    bool _dhcpTzEnabled  = true;                    ///< Enable RFC 4833 Option 100/101 TZ discovery

    // -------------------------------------------------------------------------
    // Volatile Runtime DHCP State (NEVER persisted to flash)
    // -------------------------------------------------------------------------

    bool _dhcpNtpActive = false;                    ///< True when Option 42 servers populate SNTP
    size_t _dhcpNtpCount = 0;                       ///< Count of active Option 42 IP strings
    char _dhcpNtpServers[MAX_SNTP_SLOTS][48] = {};  ///< Formatted Option 42 IP strings
    bool _dhcpTzActive = false;                     ///< True when Option 100/101 TZ override is active
    DhcpTzSource _dhcpTzSource = DhcpTzSource::None;///< Source of active DHCP TZ override
    char _dhcpTimezone[64] = "";                    ///< Active DHCP IANA label (from Option 101 TCode)
    char _dhcpPosixTz[64]  = "";                    ///< Active DHCP POSIX rule string

    // -------------------------------------------------------------------------
    // Sync State
    // -------------------------------------------------------------------------

    TimeSyncStatus _syncStatus = TimeSyncStatus::UNSYNCED; ///< Current NTP sync status
    unsigned long _lastSyncTime = 0;                       ///< millis() timestamp of last successful sync
    bool _begun = false;                                   ///< True after begin() has initialized SNTP

    /// Re-sync interval: 6 hours (in milliseconds)
    static constexpr unsigned long RESYNC_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;

    // -------------------------------------------------------------------------
    // Dependencies & EventBus Handoff
    // -------------------------------------------------------------------------

    ConfigManager* _configManager = nullptr;        ///< Pointer to ConfigManager for save operations
    EventBus* _eventBus = nullptr;                  ///< Pointer to EventBus for WiFi event subscription
    bool _eventBusSubscribed = false;               ///< Duplicate-subscribe guard
    std::atomic<bool> _wifiConnectedPending{false}; ///< Set in EventBus callback, drained in tick()

    // -------------------------------------------------------------------------
    // Private Helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Static EventBus callback for "wifi.connected" (sets atomic flag only).
     */
    static void _onWifiConnected(const char* topic, const void* payload, void* ctx);

    /**
     * @brief Reinitialize the SNTP client with current NTP server and DHCP settings.
     */
    void _reinitNtp();

    /**
     * @brief Clear all volatile DHCP NTP and timezone runtime state.
     */
    void _clearTransientDhcpState();

    /**
     * @brief Clear volatile DHCP timezone runtime state only.
     */
    void _clearDhcpTzRuntime();

    /**
     * @brief Clear volatile DHCP NTP server runtime state only.
     */
    void _clearDhcpNtpRuntime();

    /**
     * @brief Drain and apply any pending RFC 4833 Option 100/101 payload from the LwIP hook.
     */
    void _processPendingDhcpTimezone();

    /**
     * @brief Inspect active Option 42 SNTP slots and restore static fallbacks in empty slots.
     */
    void _refreshDhcpNtpStateAndFallback();

    /**
     * @brief Stream /timezones.json from LittleFS using fixed stack buffers to resolve IANA -> POSIX.
     *
     * @param iana     Target IANA timezone name (e.g. "America/New_York").
     * @param outPosix Destination buffer for POSIX TZ rule string.
     * @param outCap   Capacity of @p outPosix in bytes.
     * @return true if @p iana was found and fits in @p outPosix.
     */
    static bool _lookupIanaPosixTz(const char* iana, char* outPosix, size_t outCap);

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
};
