/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/WifiManager/WifiManager.hpp
 * Description: Wi-Fi credentials management and connection state machine.
 *              Handles AP captive portal and connection recovery.
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

#ifndef NATIVE_BUILD
#include <Arduino.h>
#else
#include <string>
// Stub String class for NATIVE_BUILD tests
#ifndef STRING_STUB_DEFINED
#define STRING_STUB_DEFINED
class String {
public:
    String(const char* s) : str(s) {}
    String(const std::string& s) : str(s) {}
    String() {}
    const char* c_str() const { return str.c_str(); }
    String operator+(const char* s) const { return String(str + s); }
    String operator+(const String& s) const { return String(str + s.str); }
private:
    std::string str;
};
#endif
#endif

#include <memory>
#include <cstring>
#include "../../include/interfaces/ITickable.hpp"
#include "../../include/interfaces/IConfigurable.hpp"
#include "../../include/interfaces/IWebContributor.hpp"

#ifndef NATIVE_BUILD
class DNSServer;
class PsychicRequest;
class PsychicResponse;
class PsychicHttpServer;
#else
class DNSServer {
public:
    DNSServer() {}
    ~DNSServer() {}
};
// Forward declaration only — full stub is in WebServerManager.hpp
class PsychicRequest;
class PsychicResponse;
class PsychicHttpServer;
#endif

// Forward declarations — avoids circular include with ConfigManager.hpp / KeyManager.hpp
class ConfigManager;
class KeyManager;


/**
 * @brief Class managing WiFi connectivity and configuration.
 *        Implements ITickable for state machine, IConfigurable for persistent config
 *        (via ConfigManager gateway), and IWebContributor for REST API endpoints.
 */
class WifiManager : public ITickable, public IConfigurable, public IWebContributor {
private:
    /**
     * @brief High-level states for the WiFi connection state machine.
     */
    enum class WiFiState {
        Init,             // System just started, need to decide next step
        StaConnecting,   // Trying to connect as Station
        ApStarting,      // Station failed, starting AP mode
        Ready,            // Connected or AP is active
        Shutdown          // Explicitly disabled
    };

    char wifiSsid[33];
    char wifiPass[65];
    char currentHostname[33];
    char _defaultHostname[33];               ///< App-supplied factory default hostname (set via setDefaultHostname)
    bool isAPMode = false;
    WiFiState currentState = WiFiState::Init;
    unsigned long stateTimer = 0;
    int connectionRetries = 0;
    unsigned long wifiDisconnectTimer = 0; ///< Continuous downtime counter (ms)
    char myUrl[24];                        ///< Formatted string URL for local IP
    unsigned long lastApiRequestTime = 0;  ///< Last time an API request was received

    bool _reconnectRequested = false;        ///< Deferred reconnect flag (set by updateWiFi, acted on by tick)
    bool _restartRequested = false;          ///< Deferred restart flag (set by connect/forget, acted on by tick)

    ConfigManager* _configManager = nullptr; ///< Pointer to ConfigManager for save operations
    KeyManager* _keyManager = nullptr;       ///< Pointer to KeyManager for encrypted password storage

    std::unique_ptr<DNSServer> dnsServer;

    void transitionTo(WiFiState newState);
    void processDNS();

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
    void tick() override;

    // --- IWebContributor Interface ---

    /**
     * @brief Register all REST API route handlers with the web server.
     * @param server Reference to the PsychicHttpServer instance.
     */
    void registerRoutes(PsychicHttpServer& server) override;

    /**
     * @brief Get the UI tab name for this contributor.
     * @return "WiFi"
     */
    const char* getTabName() const override { return "WiFi"; }

    /**
     * @brief Get the tab display order. WiFi is the primary/first tab.
     * @return 10
     */
    int getTabOrder() const override { return 10; }

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
     * @brief Returns the current effective hostname (persisted value or factory default).
     * @return Current hostname string.
     */
    const char* getHostname() const { return currentHostname; }

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
     * @brief Returns true if currently operating in Setup/AP Mode.
     * @return true if AP mode is active
     */
    bool getAPMode() const { return isAPMode; }

    /**
     * @brief Returns true if WiFi is ready (either STA connected or AP active).
     * @return true if ready
     */
    bool isReady() const { return currentState == WiFiState::Ready; }

    /**
     * @brief Returns the current stored SSID.
     * @return current SSID
     */
    const char* getSSID() const { return wifiSsid; }

    /**
     * @brief Returns true if a password is set.
     * @return true if a password exists
     */
    bool hasPassword() const { return strlen(wifiPass) > 0; }

    /**
     * @brief Test a WiFi connection without permanently changing stored credentials.
     * @param ssid SSID to test
     * @param pass Password to test (can be empty to use current)
     * @param ipOut String to store IP on success
     * @return true on success
     */
    bool testConnection(const char* ssid, const char* pass, String& ipOut);

    /**
     * @brief Update the internal URL string for the Web GUI based on current IP.
     */
    void updateMyUrl();

    /**
     * @brief Returns the internal URL string for the Web GUI.
     * @return Formatted URL
     */
    const char* getMyUrl() const { return myUrl; }

    /**
     * @brief Returns true if WiFi has been disconnected for a prolonged period
     * (e.g. 3 mins).
     * @return true if in persistent error state
     */
    bool isWifiPersistentError() const;
};
