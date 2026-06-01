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
 *              RESTful API endpoints (PR-04a/WO1):
 *              - GET  /api/wifi/status  — connection status + diagnostics
 *              - GET  /api/wifi/scan    — async WiFi scan with dedup + RSSI sort
 *              - POST /api/wifi/test    — test credentials without saving (1KB limit)
 *              - POST /api/wifi/connect — save credentials + deferred restart (1KB limit)
 *              - POST /api/wifi/forget  — erase credentials + restart to AP (256B limit)
 *
 * Exported Functions/Classes:
 * - WifiManager: [Class implementation]
 *   - begin: Initial non-blocking hardware initialization.
 *   - tick: Main non-blocking state machine for connection maintenance.
 *   - applyConfig: Apply config from JSON section.
 *   - serializeConfig: Serialize config to JSON section.
 *   - resetConfig: Erase credentials.
 *   - registerRoutes: Register 5 RESTful WiFi API endpoints.
 *   - updateWiFi: Configuration persistence via ConfigManager.
 *   - testConnection: Momentary connection probe for verification.
 *   - handleStatus: GET /api/wifi/status handler.
 *   - handleScan: GET /api/wifi/scan handler.
 *   - handleTest: POST /api/wifi/test handler.
 *   - handleConnect: POST /api/wifi/connect handler.
 *   - handleForget: POST /api/wifi/forget handler.
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

// --- Per-Route Body Size Limits (PR-04a/WO1) ---
static constexpr size_t BODY_LIMIT_WIFI_TEST    = 1024; // 1 KB for test/connect
static constexpr size_t BODY_LIMIT_WIFI_CONNECT = 1024; // 1 KB for test/connect
static constexpr size_t BODY_LIMIT_WIFI_FORGET  = 256;  // 256 B for forget

// --- Maximum networks returned by scan ---
static constexpr int MAX_SCAN_RESULTS = 15;

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
    // --- Handle deferred restart (set by connect/forget handlers) ---
    if (_restartRequested) {
        _restartRequested = false;
        ESP_LOGI(TAG, "Executing deferred restart...");
        delay(500); // Allow HTTP response to flush
        esp_restart();
        return; // Unreachable, but makes intent clear
    }

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
            // --- Deferred reconnect (triggered by updateWiFi via portal) ---
            if (_reconnectRequested) {
                _reconnectRequested = false;
                ESP_LOGI(TAG, "Executing deferred reconnect with new credentials...");
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
                transitionTo(WiFiState::WIFI_INIT);
                break;
            }

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
                // AP Mode 1-hour inactivity timeout (pragmatic interim; see WO10 notes)
                if (millis() - lastApiRequestTime > 3600000) {
                    ESP_LOGW(TAG, "AP Mode inactive for 1 hour. Shutting down AP to save power.");
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

// =============================================================================
// RESTful API Route Registration (PR-04a/WO1)
// =============================================================================

/**
 * @brief Register all REST API route handlers with the web server.
 *
 * Replaces the old GET/POST /api/wifi with 5 RESTful endpoints:
 *   GET  /api/wifi/status  — connection diagnostics
 *   GET  /api/wifi/scan    — async network scan
 *   POST /api/wifi/test    — test credentials (1KB body limit)
 *   POST /api/wifi/connect — save + restart (1KB body limit)
 *   POST /api/wifi/forget  — erase + restart to AP (256B body limit)
 */
void WifiManager::registerRoutes(AsyncWebServer& server) {
#ifndef NATIVE_BUILD
    // --- GET /api/wifi/status ---
    server.on("/api/wifi/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleStatus(request);
    });

    // --- GET /api/wifi/scan ---
    server.on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleScan(request);
    });

    // --- POST /api/wifi/test ---
    server.on("/api/wifi/test", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            // Final handler — response is sent in the body handler
        },
        NULL,
        [this](AsyncWebServerRequest* request, uint8_t* data,
               size_t len, size_t index, size_t total) {
            this->handleTest(request, data, len, index, total);
        }
    );

    // --- POST /api/wifi/connect ---
    server.on("/api/wifi/connect", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            // Final handler — response is sent in the body handler
        },
        NULL,
        [this](AsyncWebServerRequest* request, uint8_t* data,
               size_t len, size_t index, size_t total) {
            this->handleConnect(request, data, len, index, total);
        }
    );

    // --- POST /api/wifi/forget ---
    server.on("/api/wifi/forget", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            // Final handler — response is sent in the body handler
        },
        NULL,
        [this](AsyncWebServerRequest* request, uint8_t* data,
               size_t len, size_t index, size_t total) {
            this->handleForget(request, data, len, index, total);
        }
    );
#endif
}

// =============================================================================
// RESTful API Handler Implementations (PR-04a/WO1)
// =============================================================================

/**
 * @brief Handle GET /api/wifi/status.
 *
 * Returns comprehensive WiFi status including mode, SSID, IP, RSSI,
 * MAC address, channel, gateway, password existence flag, and AP mode state.
 */
void WifiManager::handleStatus(AsyncWebServerRequest* request) {
#ifndef NATIVE_BUILD
    this->notifyApiActivity();

    JsonDocument doc;
    doc["ssid"]       = this->getSSID();
    doc["passExists"] = this->hasPassword();
    doc["ap_mode"]    = this->getAPMode();
    doc["ip"]         = WiFi.localIP().toString();
    doc["mac"]        = WiFi.macAddress();

    // --- STA-mode specific fields ---
    if (!isAPMode && WiFi.status() == WL_CONNECTED) {
        doc["mode"]    = "STA";
        doc["rssi"]    = WiFi.RSSI();
        doc["channel"] = WiFi.channel();
        doc["gateway"] = WiFi.gatewayIP().toString();
    } else if (isAPMode) {
        doc["mode"]    = "AP";
        doc["ip"]      = WiFi.softAPIP().toString();
        doc["channel"] = 1; // Default AP channel
    } else {
        doc["mode"]    = "DISCONNECTED";
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
#endif
}

/**
 * @brief Handle GET /api/wifi/scan.
 *
 * Uses the async WiFi.scanNetworks() API:
 * - First call triggers an async scan and returns 202 {status:"scanning"}
 * - Subsequent calls while scan is running return 202 {status:"scanning"}
 * - When scan completes, returns 200 with networks array (RSSI-sorted,
 *   deduplicated by SSID, capped at MAX_SCAN_RESULTS)
 * - Calls WiFi.scanDelete() after reading results to free memory
 */
void WifiManager::handleScan(AsyncWebServerRequest* request) {
#ifndef NATIVE_BUILD
    this->notifyApiActivity();

    int16_t scanResult = WiFi.scanComplete();

    if (scanResult == WIFI_SCAN_FAILED) {
        // --- No scan in progress — start one ---
        ESP_LOGI(TAG, "Starting async WiFi scan...");
        WiFi.scanNetworks(true); // true = async
        request->send(202, "application/json", "{\"status\":\"scanning\"}");
        return;
    }

    if (scanResult == WIFI_SCAN_RUNNING) {
        // --- Scan is still in progress ---
        request->send(202, "application/json", "{\"status\":\"scanning\"}");
        return;
    }

    // --- Scan complete — process results ---
    ESP_LOGI(TAG, "Scan complete. Found %d networks.", scanResult);

    JsonDocument doc;
    JsonArray networks = doc["networks"].to<JsonArray>();

    // --- Build deduplicated, RSSI-sorted list ---
    // Use a simple O(n²) dedup since scan results are small (typically <30)
    struct ScanEntry {
        String ssid;
        int32_t rssi;
        bool secure;
        uint8_t channel;
    };

    // Temporary storage for dedup
    ScanEntry entries[MAX_SCAN_RESULTS];
    int entryCount = 0;

    for (int i = 0; i < scanResult && entryCount < MAX_SCAN_RESULTS; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) continue; // Skip hidden networks with no SSID

        // --- Dedup: keep strongest RSSI per SSID ---
        bool found = false;
        for (int j = 0; j < entryCount; j++) {
            if (entries[j].ssid == ssid) {
                found = true;
                if (WiFi.RSSI(i) > entries[j].rssi) {
                    entries[j].rssi    = WiFi.RSSI(i);
                    entries[j].secure  = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                    entries[j].channel = WiFi.channel(i);
                }
                break;
            }
        }

        if (!found) {
            entries[entryCount].ssid    = ssid;
            entries[entryCount].rssi    = WiFi.RSSI(i);
            entries[entryCount].secure  = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
            entries[entryCount].channel = WiFi.channel(i);
            entryCount++;
        }
    }

    // --- Sort by RSSI (strongest first) using simple insertion sort ---
    for (int i = 1; i < entryCount; i++) {
        ScanEntry key = entries[i];
        int j = i - 1;
        while (j >= 0 && entries[j].rssi < key.rssi) {
            entries[j + 1] = entries[j];
            j--;
        }
        entries[j + 1] = key;
    }

    // --- Serialize to JSON ---
    for (int i = 0; i < entryCount; i++) {
        JsonObject net = networks.add<JsonObject>();
        net["ssid"]    = entries[i].ssid;
        net["rssi"]    = entries[i].rssi;
        net["secure"]  = entries[i].secure;
        net["channel"] = entries[i].channel;
    }

    // --- Free scan memory ---
    WiFi.scanDelete();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
#endif
}

/**
 * @brief Handle POST /api/wifi/test.
 *
 * Accepts {ssid, pass} JSON body. Tests credentials without saving.
 * Returns {status:"ok", ip:"x.x.x.x"} on success or {status:"failed"} on failure.
 * Enforces 1KB body size limit — rejects with HTTP 413 if exceeded.
 */
void WifiManager::handleTest(AsyncWebServerRequest* request, uint8_t* data,
                              size_t len, size_t index, size_t total) {
#ifndef NATIVE_BUILD
    this->notifyApiActivity();

    // --- Per-route body size guard (PR-04a/WO1) ---
    if (total > BODY_LIMIT_WIFI_TEST) {
        request->send(413, "application/json", "{\"error\":\"Payload too large\"}");
        return;
    }

    // --- Only process on first/complete chunk ---
    if (index != 0) return;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);
    if (error) {
        request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid JSON\"}");
        return;
    }

    const char* ssid = doc["ssid"] | "";
    const char* pass = doc["pass"] | "";

    if (strlen(ssid) == 0) {
        request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Missing SSID\"}");
        return;
    }

    // --- Test connection ---
    String ipOut;
    bool success = this->testConnection(ssid, pass, ipOut);

    if (success) {
        JsonDocument resp;
        resp["status"] = "ok";
        resp["ip"]     = ipOut;
        String output;
        serializeJson(resp, output);
        request->send(200, "application/json", output);
    } else {
        request->send(200, "application/json", "{\"status\":\"failed\"}");
    }
#endif
}

/**
 * @brief Handle POST /api/wifi/connect.
 *
 * Accepts {ssid, pass} JSON body. Saves credentials via KeyManager (AES-256)
 * and ConfigManager, returns 200, and schedules a deferred device restart
 * so the WiFi stack reconnects with the new credentials.
 * Enforces 1KB body size limit — rejects with HTTP 413 if exceeded.
 */
void WifiManager::handleConnect(AsyncWebServerRequest* request, uint8_t* data,
                                 size_t len, size_t index, size_t total) {
#ifndef NATIVE_BUILD
    this->notifyApiActivity();

    // --- Per-route body size guard ---
    if (total > BODY_LIMIT_WIFI_CONNECT) {
        request->send(413, "application/json", "{\"error\":\"Payload too large\"}");
        return;
    }

    // --- Only process on first/complete chunk ---
    if (index != 0) return;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);
    if (error) {
        request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid JSON\"}");
        return;
    }

    const char* ssid = doc["ssid"] | "";
    const char* pass = doc["pass"] | "";

    if (strlen(ssid) == 0) {
        request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Missing SSID\"}");
        return;
    }

    // --- Save credentials and request deferred reconnect ---
    this->updateWiFi(ssid, pass);

    request->send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"Credentials saved. Restarting...\"}");

    // --- Schedule deferred restart to apply new credentials ---
    this->scheduleRestart();
#endif
}

/**
 * @brief Handle POST /api/wifi/forget.
 *
 * Clears stored SSID, password (KeyManager), and config (ConfigManager).
 * Returns 200 and schedules a deferred restart so the device boots into AP mode.
 * Enforces 256B body size limit — rejects with HTTP 413 if exceeded.
 */
void WifiManager::handleForget(AsyncWebServerRequest* request, uint8_t* data,
                                size_t len, size_t index, size_t total) {
#ifndef NATIVE_BUILD
    this->notifyApiActivity();

    // --- Per-route body size guard ---
    if (total > BODY_LIMIT_WIFI_FORGET) {
        request->send(413, "application/json", "{\"error\":\"Payload too large\"}");
        return;
    }

    // --- Only process on first/complete chunk ---
    if (index != 0) return;

    ESP_LOGW(TAG, "Forget WiFi requested — erasing credentials...");

    // --- Clear in-memory credentials ---
    memset(wifiSsid, 0, sizeof(wifiSsid));
    memset(wifiPass, 0, sizeof(wifiPass));

    // --- Delete encrypted password from KeyManager ---
    if (_keyManager != nullptr) {
        _keyManager->deleteKey("wifi_password");
        ESP_LOGI(TAG, "WiFi password removed from KeyManager");
    }

    // --- Persist empty config via ConfigManager ---
    if (_configManager != nullptr) {
        _configManager->saveModuleConfig(this);
        ESP_LOGI(TAG, "WiFi config cleared via ConfigManager");
    }

    request->send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"WiFi credentials erased. Restarting to AP mode...\"}");

    // --- Schedule deferred restart to AP mode ---
    this->scheduleRestart();
#endif
}

/**
 * @brief Schedule a deferred ESP restart.
 *
 * Sets the _restartRequested flag which is checked at the top of tick().
 * This is safe to call from HTTP handlers — the actual restart happens
 * on the main loop after the HTTP response has been flushed.
 */
void WifiManager::scheduleRestart() {
    _restartRequested = true;
    ESP_LOGI(TAG, "Deferred restart scheduled");
}

// =============================================================================
// Credential Management
// =============================================================================

/**
 * @brief Save new WiFi credentials and request a deferred reconnect.
 *
 * Persists SSID via ConfigManager and password via KeyManager (AES-256).
 * Sets _reconnectRequested so tick() will tear down and reconnect on the
 * main loop. This avoids destroying the WiFi stack from within an HTTP
 * handler (AsyncWebServer runs on the same connection being torn down).
 */
void WifiManager::updateWiFi(const char* ssid, const char* pass) {
    if (ssid != nullptr) strlcpy(wifiSsid, ssid, sizeof(wifiSsid));
    if (pass != nullptr && strlen(pass) > 0) {
        strlcpy(wifiPass, pass, sizeof(wifiPass));
        Logger::registerSecret(String(wifiPass));

        // Persist password via KeyManager (AES-256 encrypted)
        if (_keyManager != nullptr) {
            if (_keyManager->addKey("wifi_password", pass)) {
                ESP_LOGI(TAG, "WiFi password persisted via KeyManager");
            } else {
                ESP_LOGE(TAG, "CRITICAL: Failed to persist WiFi password!");
            }
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

    // --- Request deferred reconnect (handled safely in tick()) ---
    _reconnectRequested = true;
    ESP_LOGI(TAG, "Credentials saved — deferred reconnect requested");
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
