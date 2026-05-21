/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/wifiManager/wifiManager.cpp
 * Description: Implementation of Wi-Fi credentials management and connection state machine.
 *              Handles AP captive portal and connection recovery.
 *
 * Exported Functions/Classes:
 * - WifiManager: [Class implementation]
 *   - begin: Initial non-blocking hardware initialization.
 *   - tick: Main non-blocking state machine for connection maintenance.
 *   - loadConfig: Load credentials.
 *   - saveConfig: Save credentials.
 *   - resetConfig: Erase credentials.
 *   - updateWiFi: Configuration persistence.
 *   - testConnection: Momentary connection probe for verification.
 */

#include "wifiManager.hpp"

#ifndef NATIVE_BUILD
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include "esp_wifi.h"
#include "esp_log.h"
static const char* TAG = "WIFI_MGR";
#define WIFI_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define WIFI_LOGW(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#define WIFI_LOGE(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#else
#include <cstdio>
#define WIFI_LOGI(fmt, ...) printf("[I] WIFI: " fmt "\n", ##__VA_ARGS__)
#define WIFI_LOGW(fmt, ...) printf("[W] WIFI: " fmt "\n", ##__VA_ARGS__)
#define WIFI_LOGE(fmt, ...) printf("[E] WIFI: " fmt "\n", ##__VA_ARGS__)
unsigned long millis() { return 0; } // Stub
void delay(unsigned long ms) {}      // Stub
#endif

WifiManager::WifiManager() : wifiDisconnectTimer(0) {
    memset(wifiSsid, 0, sizeof(wifiSsid));
    memset(wifiPass, 0, sizeof(wifiPass));
    memset(currentHostname, 0, sizeof(currentHostname));
    strlcpy(myUrl, "http://0.0.0.0", sizeof(myUrl));
}

WifiManager::~WifiManager() {
    // std::unique_ptr automatically handles deallocation of dnsServer
}

/**
 * @brief transitionTo helper to log and reset timers.
 */
void WifiManager::transitionTo(WiFiState newState) {
    currentState = newState;
#ifndef NATIVE_BUILD
    stateTimer = millis();
#endif
}

/**
 * @brief Load Wi-Fi credentials from /mod/wifiManager.json on LittleFS.
 */
bool WifiManager::loadConfig() {
#ifndef NATIVE_BUILD
    WIFI_LOGI("Loading WiFi configuration from LittleFS...");
    if (LittleFS.exists("/mod/wifiManager.json")) {
        File f = LittleFS.open("/mod/wifiManager.json", "r");
        if (!f) {
            WIFI_LOGE("Failed to open /mod/wifiManager.json for reading");
            return false;
        }
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, f);
        if (!error) {
            strlcpy(wifiSsid, doc["ssid"] | "", sizeof(wifiSsid));
            strlcpy(wifiPass, doc["pass"] | "", sizeof(wifiPass));
            if (strlen(wifiSsid) > 0) {
                WIFI_LOGI("Loaded WiFi credentials for: %s", wifiSsid);
            }
        } else {
            WIFI_LOGE("JSON deserialization failed");
        }
        f.close();
        return true;
    }
#endif
    return false;
}

/**
 * @brief Save Wi-Fi credentials to JSON.
 */
bool WifiManager::saveConfig() {
#ifndef NATIVE_BUILD
    WIFI_LOGI("Saving WiFi credentials...");
    JsonDocument doc;
    doc["ssid"] = wifiSsid;
    doc["pass"] = wifiPass;
    
    File f = LittleFS.open("/mod/wifiManager.json", "w");
    if (!f) {
        WIFI_LOGE("Failed to open /mod/wifiManager.json for writing");
        return false;
    }
    serializeJson(doc, f);
    f.close();
#endif
    return true;
}

/**
 * @brief Reset module configuration to factory defaults.
 */
void WifiManager::resetConfig() {
    memset(wifiSsid, 0, sizeof(wifiSsid));
    memset(wifiPass, 0, sizeof(wifiPass));
#ifndef NATIVE_BUILD
    LittleFS.remove("/mod/wifiManager.json");
    WiFi.disconnect(true, true);
    esp_wifi_restore();
#endif
    WIFI_LOGI("WiFi settings erased and restored to factory defaults.");
}

/**
 * @brief Core initialization routine (Non-Blocking).
 * @param hostname The hostname to set for station/AP modes.
 */
void WifiManager::begin(const char* hostname) {
    if (hostname != nullptr) strlcpy(currentHostname, hostname, sizeof(currentHostname));
    loadConfig();

#ifndef NATIVE_BUILD
    // Prepare for tick-based init
    WiFi.persistent(false);
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
#endif
    
    transitionTo(WiFiState::WIFI_INIT);
    WIFI_LOGI("WiFi Manager initialized in NON-BLOCKING mode. Hostname: %s", currentHostname);
}

/**
 * @brief Periodic maintenance tick for the WiFi state machine.
 */
void WifiManager::tick() {
#ifndef NATIVE_BUILD
    switch (currentState) {
        case WiFiState::WIFI_INIT:
            // Wait 2 seconds for system stability before touching the radio
            if (millis() - stateTimer > 2000) {
                if (strlen(wifiSsid) > 0) {
                    WIFI_LOGI("Connecting to %s...", wifiSsid);
                    WiFi.mode(WIFI_STA);
                    WiFi.setSleep(WIFI_PS_NONE);
                    if (strlen(currentHostname) > 0) {
                        WiFi.setHostname(currentHostname);
                    }
                    WiFi.begin(wifiSsid, wifiPass);
                    connectionRetries = 0;
                    transitionTo(WiFiState::WIFI_STA_CONNECTING);
                } else {
                    WIFI_LOGW("No credentials found. Falling back to AP mode.");
                    transitionTo(WiFiState::WIFI_AP_STARTING);
                }
            }
            break;

        case WiFiState::WIFI_STA_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                WIFI_LOGI("Connected successfully. IP: %s", WiFi.localIP().toString().c_str());
                isAPMode = false;
                wifiDisconnectTimer = 0; // Reset downtime tracker
                updateMyUrl();
                transitionTo(WiFiState::WIFI_READY);
            } else if (millis() - stateTimer > 15000) { // 15 second timeout for STA
                WIFI_LOGW("WiFi connection timed out. Falling back to AP.");
                WiFi.disconnect(true); // Ensure clean slate
                transitionTo(WiFiState::WIFI_AP_STARTING);
            }
            break;

        case WiFiState::WIFI_AP_STARTING:
            WIFI_LOGI("Starting Access Point setup (Captive Portal)...");
            isAPMode = true;
            WiFi.mode(WIFI_AP);
            
            {
                IPAddress apIP(192, 168, 4, 1);
                WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
                
                const char* portalName = (strlen(currentHostname) > 0) ? currentHostname : "ESP32-Setup";
                WiFi.softAP(portalName);
                
                // Start DNS Hijacker
                dnsServer = std::make_unique<DNSServer>();
                dnsServer->start(53, "*", apIP);
                WIFI_LOGI("Access Point '%s' is online.", portalName);
            }
            
            transitionTo(WiFiState::WIFI_READY);
            break;

        case WiFiState::WIFI_READY:
            // Standard operational maintenance
            processDNS();
            
            // Reconnection logic: If we're in STA mode and lost connection
            if (!isAPMode && WiFi.status() != WL_CONNECTED) {
                // If this is the first tick since disconnection, start the timer
                if (wifiDisconnectTimer == 0) {
                    wifiDisconnectTimer = millis();
                }

                // Wait 10 seconds before attempting to recycle the connection
                if (millis() - stateTimer > 10000) {
                    WIFI_LOGW("Primary connection lost. Retrying...");
                    transitionTo(WiFiState::WIFI_INIT);
                }
            } else {
                // Reset timer if we are connected or in AP mode
                stateTimer = millis();
                if (WiFi.status() == WL_CONNECTED) {
                    wifiDisconnectTimer = 0;
                }
            }
            break;

        case WiFiState::WIFI_SHUTDOWN:
            break;
    }
#endif
}

/**
 * @brief Handles DNS requests when in AP mode.
 */
void WifiManager::processDNS() {
#ifndef NATIVE_BUILD
    if (isAPMode && dnsServer != nullptr) {
        dnsServer->processNextRequest();
    }
#endif
}

/**
 * @brief Update and save WiFi credentials.
 */
void WifiManager::updateWiFi(const char* ssid, const char* pass) {
    if (ssid != nullptr) strlcpy(wifiSsid, ssid, sizeof(wifiSsid));
    if (pass != nullptr && strlen(pass) > 0) {
        strlcpy(wifiPass, pass, sizeof(wifiPass));
    }
    saveConfig();
}

/**
 * @brief Test a WiFi connection without permanently changing stored credentials.
 */
bool WifiManager::testConnection(const char* ssid, const char* pass, String& ipOut) {
    WIFI_LOGI("WIFI_TEST: Starting connection test...");
#ifndef NATIVE_BUILD
    const char* realPass = pass;
    if (pass != nullptr && strlen(pass) == 0) {
        WIFI_LOGI("WIFI_TEST: Using stored credentials for test.");
        realPass = wifiPass;
    }

    if (WiFi.status() == WL_CONNECTED && strcmp(WiFi.SSID().c_str(), ssid) == 0 && strcmp(realPass, wifiPass) == 0) {
        ipOut = WiFi.localIP().toString().c_str();
        WIFI_LOGI("WIFI_TEST: Already connected to this network. IP: %s", ipOut.c_str());
        return true;
    }

    WiFi.disconnect(true);
    delay(200); 

    WiFi.begin(ssid, realPass);
    
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500);
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        ipOut = WiFi.localIP().toString().c_str();
        WIFI_LOGI("WIFI_TEST: Success! IP: %s", ipOut.c_str());
        return true;
    } else {
        WIFI_LOGW("WIFI_TEST: Failed. Reconnecting to primary credentials.");
        if (strlen(wifiSsid) > 0) {
            WiFi.disconnect(true);
            delay(100);
            WiFi.begin(wifiSsid, wifiPass);
        }
        return false;
    }
#else
    ipOut = "192.168.1.100";
    return true;
#endif
}

/**
 * @brief Update the internal URL string for the Web GUI.
 */
void WifiManager::updateMyUrl() {
#ifndef NATIVE_BUILD
    IPAddress ip = WiFi.localIP();
    snprintf(myUrl, sizeof(myUrl), "http://%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
#endif
}

/**
 * @brief Returns true if WiFi has been disconnected for longer than the timeout period (3 minutes).
 */
bool WifiManager::isWifiPersistentError() const {
#ifndef NATIVE_BUILD
    if (isAPMode || (WiFi.status() == WL_CONNECTED)) return false;
    if (wifiDisconnectTimer == 0) return false;
    
    // Over 3 minutes of continuous disconnection
    return (millis() - wifiDisconnectTimer > 180000);
#else
    return false;
#endif
}
