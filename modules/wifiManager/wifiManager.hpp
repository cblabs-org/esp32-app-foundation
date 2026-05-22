/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/wifiManager/wifiManager.hpp
 * Description: Wi-Fi credentials management and connection state machine.
 *              Handles AP captive portal and connection recovery.
 *
 * Exported Functions/Classes:
 * - WifiManager: Standard ESP32 WiFi lifecycle manager.
 *   - begin(): Connects to station or initializes AP recovery mode.
 *   - tick(): State machine for connection monitoring.
 *   - loadConfig(): Load credentials from LittleFS.
 *   - saveConfig(): Save credentials to LittleFS.
 *   - resetConfig(): Clear credentials.
 *   - updateWiFi(): Persists new credentials to non-volatile storage.
 *   - testConnection(): Validation probe for new credentials.
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
#include "../../include/interfaces/iConfigurable.hpp"
#include "../../include/interfaces/IWebContributor.hpp"

#ifndef NATIVE_BUILD
class DNSServer;
#else
class DNSServer {
public:
    DNSServer() {}
    ~DNSServer() {}
};
#endif


/**
 * @brief Class managing WiFi connectivity and configuration.
 *        Implements ITickable for state machine, iConfigurable for persistent config,
 *        and IWebContributor for REST API endpoints.
 */
class WifiManager : public ITickable, public iConfigurable, public IWebContributor {
private:
    /**
     * @brief High-level states for the WiFi connection state machine.
     */
    enum class WiFiState {
        WIFI_INIT,             // System just started, need to decide next step
        WIFI_STA_CONNECTING,   // Trying to connect as Station
        WIFI_AP_STARTING,      // Station failed, starting AP mode
        WIFI_READY,            // Connected or AP is active
        WIFI_SHUTDOWN          // Explicitly disabled
    };

    char wifiSsid[33];
    char wifiPass[65];
    char currentHostname[33];
    bool isAPMode = false;
    WiFiState currentState = WiFiState::WIFI_INIT;
    unsigned long stateTimer = 0;
    int connectionRetries = 0;
    unsigned long wifiDisconnectTimer = 0; ///< Continuous downtime counter (ms)
    char myUrl[24];                        ///< Formatted string URL for local IP
    unsigned long lastApiRequestTime = 0;  ///< Last time an API request was received

    std::unique_ptr<DNSServer> dnsServer;

    void transitionTo(WiFiState newState);
    void processDNS();

public:
    WifiManager();
    ~WifiManager() override;

    /**
     * @brief Initialize WiFi. Sets up initial state.
     * @param hostname The hostname to use for the AP and mDNS
     */
    void begin(const char* hostname);

    // --- iConfigurable Interface ---

    /**
     * @brief Load module configuration from LittleFS.
     * @return true if successful
     */
    bool loadConfig() override;

    /**
     * @brief Save module configuration to LittleFS.
     * @return true if successful
     */
    bool saveConfig() override;

    /**
     * @brief Reset module configuration to factory defaults.
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
     * @param server Reference to the AsyncWebServer instance.
     */
    void registerRoutes(AsyncWebServer& server) override;

    /**
     * @brief Get the UI tab name for this contributor.
     * @return String name of the tab, or nullptr if no UI tab.
     */
    const char* getTabName() const override { return "WiFi"; }

    // --- Custom Methods ---

    /**
     * @brief Record that an API request was received to keep AP mode alive.
     */
    void notifyApiActivity();

    /**
     * @brief Update and save WiFi credentials.
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
    bool isReady() const { return currentState == WiFiState::WIFI_READY; }

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
