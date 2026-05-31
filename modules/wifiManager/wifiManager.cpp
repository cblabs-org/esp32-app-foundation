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
 *              Config persistence is delegated to ConfigManager via the iConfigurable
 *              gateway interface. WifiManager never touches LittleFS directly for
 *              configuration — it provides applyConfig()/serializeConfig() callbacks.
 *
 * Exported Functions/Classes:
 * - WifiManager: [Class implementation]
 *   - begin: Initial non-blocking hardware initialization.
 *   - tick: Main non-blocking state machine for connection maintenance.
 *   - applyConfig: Apply config from JSON section.
 *   - serializeConfig: Serialize config to JSON section.
 *   - resetConfig: Erase credentials.
 *   - updateWiFi: Configuration persistence via ConfigManager.
 *   - testConnection: Momentary connection probe for verification.
 */

#include "wifiManager.hpp"
#include "../configManager/configManager.hpp"
#include "../keyManager/keyManager.hpp"

#ifndef NATIVE_BUILD
#include <FS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include "esp_wifi.h"
#else
unsigned long millis() { return 0; } // Stub
void delay(unsigned long ms) {}      // Stub
#endif

#include "platform/LogShim.h"
#include "../logger/logger.hpp"

static const char* TAG = "WIFI_MGR";

#ifdef NATIVE_BUILD
// A simple fallback for strlcpy in native testing environment if not available.
// macOS has strlcpy natively, but just to be safe if Linux is used:
#if !defined(__APPLE__) && !defined(__FreeBSD__)
static size_t strlcpy(char *dst, const char *src, size_t siz) {
    char *d = dst;
    const char *s = src;
    size_t n = siz;
    if (n != 0) {
        while (--n != 0) {
            if ((*d++ = *s++) == '\0') break;
        }
    }
    if (n == 0) {
        if (siz != 0) *d = '\0';
        while (*s++) ;
    }
    return (s - src - 1);
}
#endif
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

// =============================================================================
// iConfigurable — ConfigManager Gateway Callbacks
// =============================================================================

/**
 * @brief Apply WiFi configuration from a parsed JSON section.
 *
 * Reads "ssid" and "hostname" from the provided JSON object and copies
 * them into internal buffers. Password is retrieved from KeyManager
 * (AES-256 encrypted) rather than from config files.
 *
 * @param section Read-only JSON object containing config keys.
 */
void WifiManager::applyConfig(const JsonObject& section) {
    // --- Read SSID ---
    if (section["ssid"].is<const char*>()) {
        strlcpy(wifiSsid, section["ssid"], sizeof(wifiSsid));
        if (strlen(wifiSsid) > 0) {
            ESP_LOGI(TAG, "Config applied — SSID: %s", wifiSsid);
        }
    }

    // --- Read hostname ---
    if (section["hostname"].is<const char*>()) {
        strlcpy(currentHostname, section["hostname"], sizeof(currentHostname));
        if (strlen(currentHostname) > 0) {
            ESP_LOGI(TAG, "Config applied — Hostname: %s", currentHostname);
        }
    }

    // --- Retrieve encrypted password from KeyManager ---
    if (_keyManager != nullptr) {
        std::string storedPass = _keyManager->getKeyById("wifi_password");
        if (!storedPass.empty()) {
            strlcpy(wifiPass, storedPass.c_str(), sizeof(wifiPass));
            ESP_LOGI(TAG, "Config applied — password retrieved from KeyManager");
        }
    } else {
        ESP_LOGW(TAG, "applyConfig: KeyManager not wired — password not restored");
    }
}

/**
 * @brief Serialize current WiFi configuration into a JSON section.
 *
 * Writes "ssid" and "hostname" to the JSON object. Password is deliberately
 * excluded — secrets are managed by KeyManager.
 *
 * @param section Mutable JSON object to write config into.
 */
void WifiManager::serializeConfig(JsonObject& section) const {
    section["ssid"] = wifiSsid;
    section["hostname"] = currentHostname;
    // Password deliberately excluded — managed by KeyManager
}

/**
 * @brief Reset module configuration to factory defaults.
 */
void WifiManager::resetConfig() {
    memset(wifiSsid, 0, sizeof(wifiSsid));
    memset(wifiPass, 0, sizeof(wifiPass));
    memset(currentHostname, 0, sizeof(currentHostname));
#ifndef NATIVE_BUILD
    WiFi.disconnect(true, true);
    esp_wifi_restore();
#endif
    ESP_LOGI(TAG, "WiFi settings erased and restored to factory defaults.");
}

/**
 * @brief Core initialization routine (Non-Blocking).
 *
 * Note: loadConfig() is no longer called here — ConfigManager calls
 * applyConfig() during beginAll() before WifiManager::begin() runs.
 *
 * @param hostname The hostname to set for station/AP modes.
 */
void WifiManager::begin(const char* hostname) {
    if (hostname != nullptr) strlcpy(currentHostname, hostname, sizeof(currentHostname));
    // Config is already loaded by ConfigManager::beginAll() calling applyConfig()

#ifndef NATIVE_BUILD
    lastApiRequestTime = millis();
    // Prepare for tick-based init
    WiFi.persistent(false);
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
#endif
    
    transitionTo(WiFiState::WIFI_INIT);
    ESP_LOGI(TAG, "WiFi Manager initialized in NON-BLOCKING mode. Hostname: %s", currentHostname);
}

/**
 * @brief Periodic maintenance tick for the WiFi state machine.
 */
void WifiManager::tick() {
#ifndef NATIVE_BUILD
    switch (currentState) {
        case WiFiState::WIFI_INIT: {
            unsigned long elapsed = millis() - stateTimer;
            if (elapsed > 2000) {
                ESP_LOGI(TAG, "WIFI_INIT complete (waited %lums). SSID='%s' len=%d",
                         elapsed, wifiSsid, (int)strlen(wifiSsid));
                if (strlen(wifiSsid) > 0) {
                    ESP_LOGI(TAG, "Connecting to %s...", wifiSsid);
                    WiFi.mode(WIFI_STA);
                    WiFi.setSleep(WIFI_PS_NONE);
                    if (strlen(currentHostname) > 0) {
                        WiFi.setHostname(currentHostname);
                    }
                    WiFi.begin(wifiSsid, wifiPass);
                    connectionRetries = 0;
                    transitionTo(WiFiState::WIFI_STA_CONNECTING);
                } else {
                    ESP_LOGW(TAG, "No credentials found. Falling back to AP mode.");
                    transitionTo(WiFiState::WIFI_AP_STARTING);
                }
            }
            break;
        }

        case WiFiState::WIFI_STA_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                ESP_LOGI(TAG, "Connected successfully. IP: %s", WiFi.localIP().toString().c_str());
                isAPMode = false;
                wifiDisconnectTimer = 0; // Reset downtime tracker
                updateMyUrl();
                transitionTo(WiFiState::WIFI_READY);
            } else if (millis() - stateTimer > 15000) { // 15 second timeout for STA
                ESP_LOGW(TAG, "WiFi connection timed out. Falling back to AP.");
                WiFi.disconnect(true); // Ensure clean slate
                transitionTo(WiFiState::WIFI_AP_STARTING);
            }
            break;

        case WiFiState::WIFI_AP_STARTING:
            ESP_LOGI(TAG, "Starting Access Point setup (Captive Portal)...");
            isAPMode = true;
            WiFi.mode(WIFI_AP);
            
            {
                IPAddress apIP(192, 168, 4, 1);
                WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
                
                const char* portalName = (strlen(currentHostname) > 0) ? currentHostname : "ESP32-Setup";
                bool apResult = WiFi.softAP(portalName);
                ESP_LOGI(TAG, "softAP('%s') returned %s", portalName, apResult ? "true" : "FALSE");
                
                // Start DNS Hijacker
                dnsServer = std::make_unique<DNSServer>();
                dnsServer->start(53, "*", apIP);
                ESP_LOGI(TAG, "Access Point '%s' is online. IP: %s", portalName, apIP.toString().c_str());
                lastApiRequestTime = millis(); // Reset AP timeout timer
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
                    ESP_LOGW(TAG, "Primary connection lost. Retrying...");
                    transitionTo(WiFiState::WIFI_INIT);
                }
            } else if (isAPMode) {
                // AP Mode 10-minute inactivity timeout
                if (millis() - lastApiRequestTime > 600000) {
                    ESP_LOGW(TAG, "AP Mode inactive for 10 minutes. Shutting down AP to save power.");
                    transitionTo(WiFiState::WIFI_SHUTDOWN);
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
            if (isAPMode) {
                isAPMode = false;
                WiFi.softAPdisconnect(true);
                WiFi.mode(WIFI_OFF);
                if (dnsServer != nullptr) {
                    dnsServer->stop();
                    dnsServer.reset();
                }
                ESP_LOGI(TAG, "WiFi AP has been shut down.");
            }
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
 * @brief Record that an API request was received.
 */
void WifiManager::notifyApiActivity() {
#ifndef NATIVE_BUILD
    lastApiRequestTime = millis();
#endif
}

/**
 * @brief Register all REST API route handlers with the web server.
 */
void WifiManager::registerRoutes(AsyncWebServer& server) {
#ifndef NATIVE_BUILD
    server.on("/api/wifi", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->notifyApiActivity();
        JsonDocument doc;
        doc["ssid"] = this->getSSID();
        doc["passExists"] = this->hasPassword();
        doc["ap_mode"] = this->getAPMode();
        doc["ip"] = WiFi.localIP().toString();
        
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    server.on("/api/wifi", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            // Default handler. In ESPAsyncWebServer, if we only process body, we send response in the body handler.
            // If body was processed and response sent, this is ignored.
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            this->notifyApiActivity();
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            if (!error) {
                const char* ssid = doc["ssid"] | "";
                const char* pass = doc["pass"] | "";
                if (strlen(ssid) > 0) {
                    this->updateWiFi(ssid, pass);
                    request->send(200, "application/json", "{\"status\":\"ok\"}");
                    return;
                }
            }
            request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid JSON or missing SSID\"}");
        }
    );
#endif
}

/**
 * @brief Update, save, and reconnect with new WiFi credentials.
 *
 * Persists SSID via ConfigManager and password via KeyManager (AES-256).
 * Then restarts the WiFi state machine to connect with the new
 * credentials. If currently in AP mode, tears down the access point
 * and DNS server before transitioning to WIFI_INIT.
 */
void WifiManager::updateWiFi(const char* ssid, const char* pass) {
    if (ssid != nullptr) strlcpy(wifiSsid, ssid, sizeof(wifiSsid));
    if (pass != nullptr && strlen(pass) > 0) {
        strlcpy(wifiPass, pass, sizeof(wifiPass));
        Logger::registerSecret(String(wifiPass));

        // Persist password via KeyManager (AES-256 encrypted)
        if (_keyManager != nullptr) {
            _keyManager->addKey("wifi_password", pass);
            ESP_LOGI(TAG, "WiFi password persisted via KeyManager");
        } else {
            ESP_LOGW(TAG, "updateWiFi: KeyManager not wired — password not persisted!");
        }
    }

    // Persist via ConfigManager gateway
    if (_configManager != nullptr) {
        _configManager->saveModuleConfig(this);
    } else {
        ESP_LOGW(TAG, "updateWiFi: ConfigManager not wired — config not saved!");
    }

    // --- Restart WiFi state machine with new credentials ---
    ESP_LOGI(TAG, "New credentials saved — restarting WiFi connection...");
#ifndef NATIVE_BUILD
    if (isAPMode) {
        if (dnsServer) {
            dnsServer->stop();
            dnsServer.reset();
        }
        WiFi.softAPdisconnect(true);
        isAPMode = false;
    }
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
#endif
    transitionTo(WiFiState::WIFI_INIT);
}

/**
 * @brief Test a WiFi connection without permanently changing stored credentials.
 */
bool WifiManager::testConnection(const char* ssid, const char* pass, String& ipOut) {
    ESP_LOGI(TAG, "WIFI_TEST: Starting connection test...");
#ifndef NATIVE_BUILD
    const char* realPass = pass;
    if (pass != nullptr && strlen(pass) == 0) {
        ESP_LOGI(TAG, "WIFI_TEST: Using stored credentials for test.");
        realPass = wifiPass;
    }

    if (WiFi.status() == WL_CONNECTED && strcmp(WiFi.SSID().c_str(), ssid) == 0 && strcmp(realPass, wifiPass) == 0) {
        ipOut = WiFi.localIP().toString().c_str();
        ESP_LOGI(TAG, "WIFI_TEST: Already connected to this network. IP: %s", ipOut.c_str());
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
        ESP_LOGI(TAG, "WIFI_TEST: Success! IP: %s", ipOut.c_str());
        return true;
    } else {
        ESP_LOGW(TAG, "WIFI_TEST: Failed. Reconnecting to primary credentials.");
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
