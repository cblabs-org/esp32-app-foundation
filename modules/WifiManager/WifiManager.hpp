/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/WifiManager/WifiManager.hpp
 * Description: Wi-Fi credentials management and connection state machine.
 *              Handles AP captive portal, connection recovery, and mesh roaming.
 *
 *              Connection policy (issue #236):
 *              - No credentials (first boot): setup AP in WIFI_AP mode with the
 *                portal auth bypass (isSetupMode()). After 1 h of portal
 *                inactivity Wi-Fi is switched off (unchanged behaviour).
 *              - Credentials stored: STA-first. A failed/timed-out attempt (15 s)
 *                never switches to AP-only mode; STA is retried indefinitely with
 *                backoff delays of 5 s, 10 s, 30 s, then every 60 s.
 *              - Credentials stored and STA down continuously for > 3 min: a
 *                fallback captive AP is started in WIFI_AP_STA mode so STA keeps
 *                retrying. Portal auth stays ENFORCED. The AP is torn down as soon
 *                as STA reconnects; after 1 h of AP inactivity only the fallback
 *                AP is dropped — Wi-Fi is never switched off.
 *
 *              Mesh Roaming and AP Selection:
 *              - Uses all-channel scan (WIFI_ALL_CHANNEL_SCAN) sorted by signal
 *                strength (WIFI_CONNECT_AP_BY_SIGNAL) during connection so the
 *                device always associates with the strongest AP at boot/reconnect.
 *              - 802.11k/v roaming (BTM, RRM) is enabled via CONFIG_ESP_WIFI_11KV_SUPPORT
 *                in sdkconfig.defaults — the IDF supplicant handles mesh controller
 *                steering requests natively, requiring no application-layer code.
 *
 *              Implements IConfigurable via ConfigManager gateway pattern:
 *              - Namespace: "wifi" (config stored at /mod/wifi.json)
 *              - Schema version: 1
 *              - Hostname is stored in the wifi config section
 *
 *              RESTful API endpoints (PR-04a/WO1):
 *              - GET  /api/wifi/status  — connection status + diagnostics
 *              - GET  /api/wifi/scan    — async WiFi scan with dedup
 *              - POST /api/wifi/test    — test credentials without saving
 *              - POST /api/wifi/connect — save credentials + deferred restart
 *              - POST /api/wifi/forget  — erase credentials + restart to AP
 *
 * Exported Functions/Classes:
 * - WifiManager: Standard ESP32 WiFi lifecycle manager.
 *
 *   Lifecycle:
 *   - begin():              Connects to station or initializes AP recovery mode.
 *   - tick():               State machine for connection monitoring.  [ITickable]
 *
 *   Custom Methods:
 *   - updateWiFi():         Persists new credentials and restarts the WiFi state machine.
 *   - testConnection():     Validation probe for new credentials.
 *   - setConfigManager():   Wire the ConfigManager pointer for save operations.
 *   - setKeyManager():      Wire the KeyManager pointer for encrypted password storage.
 *   - setDefaultHostname(): App-supplied factory default hostname (used by resetConfig()).
 *   - getHostname():        Current effective hostname (single source of truth for the framework).
 *
 *   IConfigurable (namespace: "wifi", schema v1):
 *   - getConfigNamespace(): Return "wifi"
 *   - getSchemaVersion():   Return 1
 *   - applyConfig():        Apply WiFi config from JSON section
 *   - serializeConfig():    Serialize WiFi config to JSON section
 *   - resetConfig():        Clear credentials.
 *
 *   IWebContributor (tab: "WiFi", order: 10):
 *   - registerRoutes():     Register 5 RESTful WiFi API endpoints
 */

#pragma once

#include <Arduino.h>

#include <memory>
#include <cstring>
#include "interfaces/ITickable.hpp"
#include "interfaces/IConfigurable.hpp"
#include "interfaces/IWebContributor.hpp"
#include "interfaces/IMCPContributor.hpp"
#include "utils/Periodic.hpp"

class DNSServer;
class PsychicRequest;
class PsychicResponse;
class PsychicHttpServer;

// Forward declarations — avoids circular include with ConfigManager.hpp / KeyManager.hpp
class ConfigManager;
class KeyManager;
class EventBus;

/**
 * @brief Well-known EventBus topic constants for WiFi state changes.
 *
 * Published by WifiManager when STA connectivity changes. Subscribers
 * (e.g. HomeAssistantModule) can react to real-time WiFi events.
 */
namespace WifiEvents {
    static constexpr const char* WIFI_CONNECTED    = "wifi.connected"; ///< Wifi Connected
    static constexpr const char* WIFI_DISCONNECTED = "wifi.disconnected"; ///< Wifi Disconnected
    static constexpr const char* WIFI_AP_STARTED   = "wifi.ap_started"; ///< Wifi Ap Started
}


/**
 * @brief Class managing WiFi connectivity and configuration.
 *        Implements ITickable for state machine, IConfigurable for persistent config
 *        (via ConfigManager gateway), IWebContributor for REST API endpoints,
 *        and IMCPContributor for MCP tools.
 */
class WifiManager : public ITickable, public IConfigurable, public IWebContributor, public IMCPContributor {
private:
    /**
     * @brief High-level states for the WiFi connection state machine.
     */
    enum class WiFiState {
        Init,             // System just started, need to decide next step
        StaConnecting,    // One STA connect attempt in flight (15 s timeout)
        StaBackoff,       // Credentials stored, STA attempt failed: waiting the
                          // backoff delay (5/10/30/60 s) before the next attempt.
                          // The fallback AP (AP+STA) is raised from here after
                          // > 3 min of continuous STA downtime (#236).
        ApStarting,       // No credentials: starting the first-boot setup AP
        Ready,            // STA connected, or setup AP active
        Shutdown          // Setup AP idle for 1 h: Wi-Fi off (no credentials only)
    };

    // --- Connection policy timings (#236) ---
    static constexpr uint32_t STA_ATTEMPT_TIMEOUT_MS   = 30000;    ///< Per-attempt STA connect timeout (30 s; 15 s was too short with BLE coex — dev.81)
    static constexpr uint32_t STA_LINK_LOSS_GRACE_MS   = 10000;    ///< Grace after link loss before the first reconnect attempt
    static constexpr uint32_t FALLBACK_AP_DELAY_MS     = 180000;   ///< Continuous STA downtime before the fallback AP starts (matches isWifiPersistentError())
    static constexpr uint32_t AP_INACTIVITY_TIMEOUT_MS = 3600000;  ///< Portal inactivity before the AP is dropped

    char _wifiSsid[33];
    char _wifiPass[65];
    char _currentHostname[33];
    char _defaultHostname[33];               ///< App-supplied factory default hostname (set via setDefaultHostname)
    bool _isAPMode = false;
    WiFiState _currentState = WiFiState::Init;
    Periodic _stateTimer;
    int _connectionRetries = 0;               ///< Consecutive failed STA attempts (drives the backoff schedule)
    bool _fallbackApSuppressed = false;       ///< Fallback AP dropped after 1 h idle; not re-raised until STA reconnects
    Periodic _wifiDisconnectTimer; ///< Continuous downtime tracker (ms)
    char _myUrl[24];                        ///< Formatted string URL for local IP
    volatile uint32_t _lastApiRequestTime = 0;  ///< Last time an API request was received (cross-thread)

    bool _reconnectRequested = false;        ///< Deferred reconnect flag (set by updateWiFi, acted on by tick)
    bool _restartRequested = false;          ///< Deferred restart flag (set by connect/forget, acted on by tick)

    ConfigManager* _configManager = nullptr; ///< Pointer to ConfigManager for save operations
    KeyManager* _keyManager = nullptr;       ///< Pointer to KeyManager for encrypted password storage
    EventBus* _eventBus = nullptr;           ///< Pointer to EventBus for publishing wifi state events

    std::unique_ptr<DNSServer> _dnsServer;

    void transitionTo(WiFiState newState, uint32_t now);
    void processDNS();
    void connectStation(const char* ssid, const char* pass);

    /** @brief Apply the Wi-Fi power-save mode (modem sleep when BT is enabled, #236). */
    void applyPowerSave();
    /** @brief Start one STA connect attempt (AP+STA if the fallback AP is up) and enter StaConnecting. */
    void startStaAttempt(uint32_t now);
    /** @brief STA association succeeded: tear down any fallback AP, reset retry state, publish WIFI_CONNECTED. */
    void onStaConnected(uint32_t now);
    /** @brief Start the captive AP (WIFI_AP in setup mode, WIFI_AP_STA when credentials exist) and publish WIFI_AP_STARTED. */
    void startAccessPoint(uint32_t now);
    /** @brief Stop the captive AP and DNS hijacker if running; clears _isAPMode. */
    void stopAccessPoint();
    /** @brief Start the continuous STA-downtime tracker if it is not already running. */
    void markStaDown(uint32_t now);
    /** @brief Milliseconds of continuous STA downtime (0 when not tracking). */
    uint32_t staDownMs(uint32_t now) const;

    // --- RESTful API Handlers (PR-04a/WO1) ---

    /**
     * @brief Handle GET /api/wifi/status — connection diagnostics
     * @param request The incoming HTTP request.
     */
    esp_err_t handleStatus(PsychicRequest* request, PsychicResponse* response);

    /**
     * @brief Handle GET /api/wifi/scan — triggers async WiFi scan and returns results.
     *        Returns 202 while scan is running; final result is RSSI-sorted, deduplicated,
     *        capped at 15 networks.
     * @param request The incoming HTTP request.
     */
    esp_err_t handleScan(PsychicRequest* request, PsychicResponse* response);

    /**
     * @brief Handle POST /api/wifi/test — test credentials without saving.
     *        Accepts {ssid, pass}. Returns {status:"ok", ip:"..."} or {status:"failed"}.
     *        Body limit: 1 KB.
     * @param request The incoming HTTP request.
     */
    esp_err_t handleTest(PsychicRequest* request, PsychicResponse* response);

    /**
     * @brief Handle POST /api/wifi/connect — save credentials and schedule restart.
     *        Accepts {ssid, pass}. Saves via KeyManager, returns 200, schedules restart.
     *        Body limit: 1 KB.
     * @param request The incoming HTTP request.
     */
    esp_err_t handleConnect(PsychicRequest* request, PsychicResponse* response);

    /**
     * @brief Handle POST /api/wifi/forget — erase credentials and restart to AP mode.
     *        Clears KeyManager + config, returns 200, schedules restart.
     *        Body limit: 256 bytes.
     * @param request The incoming HTTP request.
     */
    esp_err_t handleForget(PsychicRequest* request, PsychicResponse* response);

    /**
     * @brief Schedule a deferred ESP restart (500ms delay).
     *        Safe to call from HTTP handlers — actual restart happens in tick().
     */
    void scheduleRestart();

public:
    /** @brief Construct a WifiManager instance. */
    WifiManager();
    ~WifiManager() override;

    /**
     * @brief Initialize WiFi. Sets up initial state.
     *
     * Relies on the hostname already being populated — either by a persisted
     * value applied via applyConfig(), or by the factory default set in
     * resetConfig() (seeded from setDefaultHostname()).
     */
    void begin();

    // --- IConfigurable Interface (ConfigManager Gateway) ---

    /**
     * @brief Return the config namespace for WiFi settings.
     * @return "wifi"
     */
    const char* getConfigNamespace() const override { return "wifi"; }

    /**
     * @brief Return the current schema version for WiFi config.
     * @return 1
     */
    int getSchemaVersion() const override { return 1; }

    /**
     * @brief Apply WiFi configuration from a parsed JSON section.
     *
     * Reads "ssid" and "hostname" from the section. Password is retrieved
     * from KeyManager (AES-256 encrypted) rather than from config files.
     *
     * @param section Read-only JSON object containing config keys.
     */
    void applyConfig(const JsonObject& section) override;

    /**
     * @brief Serialize current WiFi configuration into a JSON section.
     *
     * Writes "ssid" and "hostname" to the section. Password is NOT serialised.
     *
     * @param section Mutable JSON object to write config into.
     */
    void serializeConfig(JsonObject& section) const override;

    /**
     * @brief Reset WiFi configuration to factory defaults.
     */
    void resetConfig() override;

    // --- ITickable Interface ---

    /**
     * @brief Periodic maintenance tick for the WiFi state machine.
     */
    void tick(uint32_t currentMillis) override;

    // --- IWebContributor Interface ---

    /**
     * @brief Register all REST API route handlers with the web server.
     * @param server Reference to the PsychicHttpServer instance.
     */
    void registerRoutes(PsychicHttpServer& server) override;

    /**
     * @brief Register the WiFi tab as a top-level navigation tab.
     * @param registry  The ITabRegistry to register tabs with.
     */
    void registerTabs(ITabRegistry& registry) override {
        registry.addTab("WiFi", nullptr, 10);
    }

    /**
     * @brief Return the API path prefix for body-size middleware matching.
     * @return "/api/wifi"
     */
    const char* getApiPrefix() const override { return "/api/wifi"; }

    /**
     * @brief Maximum POST body size for WiFi routes (1 KiB — credentials are small).
     * @return 1024
     */
    size_t getMaxPostBody() const override { return 1024; }

    // --- IMCPContributor Interface ---

    /**
     * @brief Register MCP tools for WiFi management
     * @param registry Reference to the tool registry
     */
    void registerMCPTools(MCPToolRegistry& registry) override;

    // --- Custom Methods ---

    /**
     * @brief Wire the ConfigManager pointer for save operations.
     * @param cm Pointer to the ConfigManager instance.
     */
    void setConfigManager(ConfigManager* cm) { _configManager = cm; }

    /**
     * @brief Wire the KeyManager pointer for encrypted password storage.
     * @param km Pointer to the KeyManager instance.
     */
    void setKeyManager(KeyManager* km) { _keyManager = km; }

    /**
     * @brief Wire the EventBus pointer for publishing wifi state events.
     * @param eb Pointer to the EventBus instance.
     */
    void setEventBus(EventBus* eb) { _eventBus = eb; }

    /**
     * @brief Framework fallback default hostname when no app-supplied default exists.
     */
    static constexpr const char* DEFAULT_HOSTNAME = "esp32-device";

    /**
     * @brief Framework fallback SoftAP SSID when no hostname is available.
     */
    static constexpr const char* DEFAULT_AP_SSID = "ESP32-Setup";

    /**
     * @brief Set the app-supplied factory default hostname.
     *
     * Called by the downstream app (via Application::getWifiManager()) before
     * Application::begin() runs, so resetConfig() has an app-specific default
     * to fall back to instead of the framework's generic "esp32-device".
     *
     * @param hostname Default hostname to seed resetConfig() with (truncated to 32 chars).
     */
    void setDefaultHostname(const char* hostname);

    /**
     * @brief Returns the app-supplied factory default hostname.
     * @return Default hostname string.
     */
    const char* getDefaultHostname() const { return _defaultHostname; }

    /**
     * @brief Returns the current effective hostname (persisted value or factory default).
     * @return Current hostname string.
     */
    const char* getHostname() const {
        if (strlen(_currentHostname) > 0 && (strcmp(_currentHostname, DEFAULT_HOSTNAME) != 0 || strcmp(_defaultHostname, DEFAULT_HOSTNAME) == 0)) {
            return _currentHostname;
        }
        return (strlen(_defaultHostname) > 0) ? _defaultHostname : DEFAULT_HOSTNAME;
    }

    /**
     * @brief Record that an API request was received to keep AP mode alive.
     */
    void notifyApiActivity();

    /**
     * @brief Save new WiFi credentials and request a deferred reconnect.
     *
     * Persists SSID via ConfigManager and password via KeyManager (AES-256).
     * Sets a deferred reconnect flag that is acted on by tick() on the main
     * loop. This avoids tearing down the WiFi stack from within an HTTP
     * handler (which would crash the AsyncWebServer running on that stack).
     *
     * @param ssid SSID to save.
     * @param pass Password to save (Masked as dots in UI).
     */
    void updateWiFi(const char* ssid, const char* pass);

    /**
     * @brief Returns true if the captive soft-AP is currently running — either
     *        the first-boot setup AP or the fallback AP raised during a long STA
     *        outage (see isSetupMode() to tell them apart).
     * @return true if AP mode is active
     */
    bool getAPMode() const { return _isAPMode; }

    /**
     * @brief Returns true only for first-boot setup: AP active AND no stored credentials.
     *
     * Security rationale (#236): the AuthManager portal-auth bypass must follow
     * this, NOT getAPMode(). A device that already has Wi-Fi credentials may raise
     * a fallback AP during a long STA outage; that outage can be forced by anyone
     * in radio range (e.g. spoofed deauthentication frames), so the portal behind
     * the fallback AP (config, credentials, OTA, MCP) must keep requiring login.
     * The bypass is only acceptable when there is nothing configured yet.
     *
     * @return true if in first-boot setup mode
     */
    bool isSetupMode() const { return _isAPMode && !hasCredentials(); }

    /**
     * @brief Returns true if WiFi is ready (either STA connected or AP active).
     *        The fallback AP counts as ready so the portal is served on it even
     *        while the state machine is cycling STA retries.
     * @return true if ready
     */
    bool isReady() const { return _currentState == WiFiState::Ready || _isAPMode; }

    // --- Reconnect policy (pure functions; unit-tested, #236) ---

    /**
     * @brief Backoff delay before the next STA attempt.
     * @param failedAttempts Consecutive failed attempts so far (>= 1).
     * @return 5 s, 10 s, 30 s, then 60 s for every later attempt.
     */
    static uint32_t staRetryDelayMs(int failedAttempts) {
        if (failedAttempts <= 1) return 5000;
        if (failedAttempts == 2) return 10000;
        if (failedAttempts == 3) return 30000;
        return 60000;
    }

    /**
     * @brief Decide whether the fallback AP (AP+STA, auth enforced) should start.
     * @param hasCreds       Stored credentials exist.
     * @param apActive       An AP is already running.
     * @param suppressed     Fallback AP was dropped after 1 h idle during this outage.
     * @param staDownForMs   Continuous STA downtime in ms.
     * @return true if the fallback AP should be started now.
     */
    static bool shouldStartFallbackAp(bool hasCreds, bool apActive, bool suppressed, uint32_t staDownForMs) {
        return hasCreds && !apActive && !suppressed && staDownForMs > FALLBACK_AP_DELAY_MS;
    }

    /**
     * @brief Returns the current stored SSID.
     * @return current SSID
     */
    const char* getSSID() const { return _wifiSsid; }

    /**
     * @brief Returns true if WiFi credentials (SSID) are configured.
     * @return true if SSID is non-empty
     */
    bool hasCredentials() const { return strlen(_wifiSsid) > 0; }

    /**
     * @brief Returns the SSID of the AP mode captive portal network.
     * @return AP SSID (effective hostname or "ESP32-Setup")
     */
    const char* getAPSSID() const {
        if (strlen(_currentHostname) > 0 && strcmp(_currentHostname, DEFAULT_HOSTNAME) != 0) {
            return _currentHostname;
        }
        if (strlen(_defaultHostname) > 0 && strcmp(_defaultHostname, DEFAULT_HOSTNAME) != 0) {
            return _defaultHostname;
        }
        if (strlen(_currentHostname) > 0) {
            return _currentHostname;
        }
        if (strlen(_defaultHostname) > 0) {
            return _defaultHostname;
        }
        return DEFAULT_AP_SSID;
    }

    /**
     * @brief Returns true if a password is set.
     * @return true if a password exists
     */
    bool hasPassword() const { return strlen(_wifiPass) > 0; }

    /**
     * @brief Test a WiFi connection without permanently changing stored credentials.
     * @param ssid SSID to test
     * @param pass Password to test (can be empty to use current)
     * @param ipOut std::string to store IP on success
     * @return true on success
     */
    bool testConnection(const char* ssid, const char* pass, std::string& ipOut);

    /**
     * @brief Update the internal URL string for the Web GUI based on current IP.
     */
    void updateMyUrl();

    /**
     * @brief Returns the internal URL string for the Web GUI.
     * @return Formatted URL
     */
    const char* getMyUrl() const { return _myUrl; }

    /**
     * @brief Returns true if WiFi has been disconnected for a prolonged period
     * (e.g. 3 mins).
     * @return true if in persistent error state
     */
    bool isWifiPersistentError() const;
};
