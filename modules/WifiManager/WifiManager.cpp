/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/WifiManager/wifiManager.cpp
 * Description: Implementation of Wi-Fi credentials management and connection state machine.
 *              Handles AP captive portal, connection recovery, and mesh roaming.
 *
 *              Config persistence is delegated to ConfigManager via the IConfigurable
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
 *   - tick: Main non-blocking state machine for connection maintenance
 *           (STA-first retry with backoff, fallback AP+STA, #236).
 *   - startStaAttempt / onStaConnected: STA attempt lifecycle helpers.
 *   - startAccessPoint / stopAccessPoint: Captive soft-AP lifecycle helpers.
 *   - applyPowerSave: Modem sleep on BT builds, WIFI_PS_NONE otherwise.
 *   - connectStation: Connects STA with 802.11k/v roaming and RSSI filtering.
 *   - applyConfig: Apply config from JSON section.
 *   - serializeConfig: Serialize config to JSON section.
 *   - resetConfig: Erase credentials and restore defaults.
 *   - registerRoutes: Register 5 RESTful WiFi API endpoints.
 *   - updateWiFi: Configuration persistence via ConfigManager.
 *   - testConnection: Momentary connection probe for verification.
 *   - handleStatus: GET /api/wifi/status handler.
 *   - handleScan: GET /api/wifi/scan handler.
 *   - handleTest: POST /api/wifi/test handler.
 *   - handleConnect: POST /api/wifi/connect handler.
 *   - handleForget: POST /api/wifi/forget handler.
 */

#include "WifiManager.hpp"
#include "ConfigManager.hpp"
#include "KeyManager.hpp"
#include "MCPToolRegistry.hpp"
#include "utils/Periodic.hpp"
#include "EventBus.hpp"
#include "interfaces/ITickable.hpp"

#include <PsychicHttp.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include "esp_wifi.h"

#include "platform/LogShim.h"
#include "Redactor.h"

static constexpr const char* TAG = "WifiManager";

// --- Per-Route Body Size Limits (PR-04a/WO1) ---
static constexpr size_t BODY_LIMIT_WIFI_TEST    = 1024; // 1 KB for test/connect
static constexpr size_t BODY_LIMIT_WIFI_CONNECT = 1024; // 1 KB for test/connect
static constexpr size_t BODY_LIMIT_WIFI_FORGET  = 256;  // 256 B for forget

// --- Maximum networks returned by scan ---
static constexpr int MAX_SCAN_RESULTS = 15;

struct WifiManager::ProvisioningGuard {
    WifiManager& self;
    bool blockedByRestart = false;
    explicit ProvisioningGuard(WifiManager& wm) : self(wm) {
        self._persistOpMutex.lock();
        blockedByRestart = self.isRestartPending();
        if (!blockedByRestart) {
            self._provisioningInProgress.store(true);
        }
    }
    ~ProvisioningGuard() {
        if (!blockedByRestart) {
            self.clearPendingSsid();
            self._provisioningInProgress.store(false);
        }
        self._persistOpMutex.unlock();
    }
    ProvisioningGuard(const ProvisioningGuard&) = delete;
    ProvisioningGuard& operator=(const ProvisioningGuard&) = delete;
};

WifiManager::WifiManager() : _wifiDisconnectTimer(0) {
    memset(_wifiSsid, 0, sizeof(_wifiSsid));
    memset(_wifiPass, 0, sizeof(_wifiPass));
    memset(_pendingSsid, 0, sizeof(_pendingSsid));
    memset(_currentHostname, 0, sizeof(_currentHostname));
    strlcpy(_defaultHostname, DEFAULT_HOSTNAME, sizeof(_defaultHostname));
    strlcpy(_myUrl, "http://0.0.0.0", sizeof(_myUrl));
}

WifiManager::~WifiManager() {
    // std::unique_ptr automatically handles deallocation of _dnsServer
}

/**
 * @brief transitionTo helper to log and reset timers.
 */
void WifiManager::transitionTo(WiFiState newState, uint32_t now) {
    _currentState.store(newState);
    _stateTimer.reset(now);
}

void WifiManager::clearPendingSsid() {
    std::lock_guard<FreeRTOSMutex> guard(_credMutex);
    _hasPendingSsid = false;
    memset(_pendingSsid, 0, sizeof(_pendingSsid));
}

// =============================================================================
// IConfigurable — ConfigManager Gateway Callbacks
// =============================================================================

/**
 * @brief Apply WiFi configuration from a parsed JSON section.
 *
 * Reads "ssid" and "hostname" from the provided JSON object
 * and copies them into internal members. Password is retrieved from KeyManager
 * (AES-256 encrypted) rather than from config files.
 *
 * @param section Read-only JSON object containing config keys.
 */
void WifiManager::applyConfig(const JsonObject& section) {
    // Retrieve encrypted password from KeyManager outside _credMutex (LittleFS I/O)
    std::string storedPass;
    const bool hasKeyManager = (_keyManager != nullptr);
    if (hasKeyManager) {
        storedPass = _keyManager->getKeyById("wifi_password");
    }

    bool hasSsid = false;
    bool hasHostname = false;
    char appliedSsid[sizeof(_wifiSsid)] = {};
    char appliedHost[sizeof(_currentHostname)] = {};

    {
        std::lock_guard<FreeRTOSMutex> guard(_credMutex);

        // --- Read SSID ---
        if (section["ssid"].is<const char*>()) {
            strlcpy(_wifiSsid, section["ssid"], sizeof(_wifiSsid));
            strlcpy(appliedSsid, _wifiSsid, sizeof(appliedSsid));
            hasSsid = (appliedSsid[0] != '\0');
        }

        // --- Read hostname ---
        if (section["hostname"].is<const char*>()) {
            const char* h = section["hostname"];
            // Reject a stale persisted "esp32-device" when the app has supplied
            // a custom default hostname (e.g. via setDefaultHostname()).
            const bool isStaleDefault = (strcmp(h, DEFAULT_HOSTNAME) == 0 &&
                                         strcmp(_defaultHostname, DEFAULT_HOSTNAME) != 0);
            if (strlen(h) > 0 && !isStaleDefault) {
                strlcpy(_currentHostname, h, sizeof(_currentHostname));
            } else if (strlen(_defaultHostname) > 0) {
                strlcpy(_currentHostname, _defaultHostname, sizeof(_currentHostname));
            }
            strlcpy(appliedHost, _currentHostname, sizeof(appliedHost));
            hasHostname = (appliedHost[0] != '\0');
        }

        // --- Apply retrieved encrypted password ---
        if (hasKeyManager && !storedPass.empty()) {
            strlcpy(_wifiPass, storedPass.c_str(), sizeof(_wifiPass));
        }
    }

    if (hasSsid) {
        ESP_LOGI(TAG, "Config applied — SSID: %s", appliedSsid);
    }
    if (hasHostname) {
        ESP_LOGI(TAG, "Config applied — Hostname: %s", appliedHost);
    }
    if (hasKeyManager) {
        if (!storedPass.empty()) {
            ESP_LOGI(TAG, "Config applied — password retrieved from KeyManager");
        }
    } else {
        ESP_LOGW(TAG, "applyConfig: KeyManager not wired — password not restored");
    }
}

/**
 * @brief Serialize current WiFi configuration into a JSON section.
 *
 * Writes "ssid" and "hostname" to the JSON object.
 * Password is deliberately excluded — secrets are managed by KeyManager.
 *
 * @param section Mutable JSON object to write config into.
 */
void WifiManager::serializeConfig(JsonObject& section) const {
    std::lock_guard<FreeRTOSMutex> guard(_credMutex);
    section["ssid"] = _hasPendingSsid ? _pendingSsid : _wifiSsid;
    section["hostname"] = _currentHostname;
    // Password deliberately excluded — managed by KeyManager
}

/**
 * @brief Reset module configuration to factory defaults.
 *
 * Hostname falls back to the app-supplied default (set via setDefaultHostname(),
 * or the framework fallback "esp32-device" if the app never called it).
 */
void WifiManager::resetConfig() {
    {
        std::lock_guard<FreeRTOSMutex> guard(_credMutex);
        memset(_wifiSsid, 0, sizeof(_wifiSsid));
        memset(_wifiPass, 0, sizeof(_wifiPass));
        memset(_pendingSsid, 0, sizeof(_pendingSsid));
        _hasPendingSsid = false;
        strlcpy(_currentHostname, _defaultHostname, sizeof(_currentHostname));
    }
    WiFi.disconnect(true, true);
    esp_wifi_restore();
    ESP_LOGI(TAG, "WiFi settings erased and restored to factory defaults.");
}

/**
 * @brief Connects to a WiFi station with all-channel scanning and signal-strength sorting.
 *
 * Configures all-channel scanning and signal-strength based AP selection so that
 * the device always associates with the strongest AP on the SSID at connection time.
 * Actual 802.11k/v roaming (BTM, RRM) is enabled via CONFIG_ESP_WIFI_11KV_SUPPORT
 * in sdkconfig.defaults — the IDF supplicant handles mesh steering natively.
 *
 * Uses the two-stage pattern (WiFi.begin with connect=false, then esp_wifi_set_config
 * + esp_wifi_connect) to prevent the Arduino core from overwriting the IDF config.
 *
 * @param ssid WiFi network SSID.
 * @param pass WiFi network password.
 */
void WifiManager::connectStation(const char* ssid, const char* pass) {
#ifndef UNIT_TEST
    WiFi.begin(ssid, pass, 0, nullptr, false);

    wifi_config_t conf;
    if (esp_wifi_get_config(WIFI_IF_STA, &conf) == ESP_OK) {
        conf.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
        conf.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
        esp_wifi_set_config(WIFI_IF_STA, &conf);
    }
    esp_wifi_connect();
#else
    WiFi.begin(ssid, pass);
#endif
}

/**
 * @brief Core initialization routine (Non-Blocking).
 *
 * Note: loadConfig() is no longer called here — ConfigManager calls
 * applyConfig() during beginAll() before WifiManager::begin() runs.
 *
 * The hostname is already populated by this point — either from a persisted
 * value (applyConfig) or the factory default (resetConfig) — so begin() simply
 * relies on currentHostname rather than accepting a parameter.
 */
void WifiManager::copyDefaultHostname(char* buf, size_t len) const {
    if (!buf || len == 0) return;
    std::lock_guard<FreeRTOSMutex> guard(_credMutex);
    strlcpy(buf, _defaultHostname, len);
}

void WifiManager::copyHostname(char* buf, size_t len) const {
    if (!buf || len == 0) return;
    std::lock_guard<FreeRTOSMutex> guard(_credMutex);
    const char* src = (_currentHostname[0] != '\0')
        ? _currentHostname
        : ((_defaultHostname[0] != '\0') ? _defaultHostname : DEFAULT_HOSTNAME);
    strlcpy(buf, src, len);
}

void WifiManager::copySSID(char* buf, size_t len) const {
    if (!buf || len == 0) return;
    std::lock_guard<FreeRTOSMutex> guard(_credMutex);
    strlcpy(buf, _wifiSsid, len);
}

void WifiManager::copyAPSSID(char* buf, size_t len) const {
    if (!buf || len == 0) return;
    std::lock_guard<FreeRTOSMutex> guard(_credMutex);
    const char* host = (_currentHostname[0] != '\0')
        ? _currentHostname
        : ((_defaultHostname[0] != '\0') ? _defaultHostname : DEFAULT_HOSTNAME);
    const char* src = (host[0] != '\0' && strcmp(host, DEFAULT_HOSTNAME) != 0)
        ? host
        : DEFAULT_AP_SSID;
    strlcpy(buf, src, len);
}

void WifiManager::begin() {
    // Config is already loaded by ConfigManager::beginAll() calling applyConfig()

    _lastApiRequestTime.store(millis());
    // Prepare for tick-based init
    WiFi.persistent(false);
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    
    _stateTimer.setInterval(2000);  // 2s debounce before Init fires (pre-MK: `elapsed > 2000`)
    transitionTo(WiFiState::Init, millis());
    char host[sizeof(_currentHostname)];
    copyHostname(host, sizeof(host));
    ESP_LOGI(TAG, "WiFi Manager initialized in NON-BLOCKING mode. Hostname: %s", host);
}

/**
 * @brief Apply the Wi-Fi power-save mode.
 *
 * Always WIFI_PS_NONE.  WO-10 originally set WIFI_PS_MIN_MODEM on BLE
 * builds (the ESP-IDF logs "Should enable WiFi modem sleep when both WiFi
 * and Bluetooth are enabled"), but on congested 2.4 GHz networks modem
 * sleep causes the radio to miss 802.11 auth response frames, leading to
 * persistent AUTH_EXPIRE failures (dev.81 regression).  The hardware
 * coexistence arbiter (CONFIG_ESP_COEX_SW_COEXIST_ENABLE) handles BLE/WiFi
 * time-sharing independently of the power-save mode.  For always-powered
 * devices there is no battery benefit to modem sleep.
 */
void WifiManager::applyPowerSave() {
    WiFi.setSleep(WIFI_PS_NONE);
}

/**
 * @brief Start the continuous STA-downtime tracker if it is not already running.
 */
void WifiManager::markStaDown(uint32_t now) {
    if (_wifiDisconnectTimer.getInterval() == 0) {
        _wifiDisconnectTimer.reset(now);
        _wifiDisconnectTimer.setInterval(1); // Enable tracking
    }
}

/**
 * @brief Milliseconds of continuous STA downtime (0 when not tracking).
 */
uint32_t WifiManager::staDownMs(uint32_t now) const {
    if (_wifiDisconnectTimer.getInterval() == 0) return 0;
    return now - _wifiDisconnectTimer.getLastFire();
}

/**
 * @brief Start one STA connect attempt and enter StaConnecting.
 *
 * If the fallback AP is up, the radio runs in WIFI_AP_STA so the portal
 * stays reachable while STA retries (#236 D-B).
 */
void WifiManager::startStaAttempt(uint32_t now) {
    char ssid[sizeof(_wifiSsid)];
    char pass[sizeof(_wifiPass)];
    {
        std::lock_guard<FreeRTOSMutex> guard(_credMutex);
        strlcpy(ssid, _wifiSsid, sizeof(ssid));
        strlcpy(pass, _wifiPass, sizeof(pass));
    }

    const bool apActive = _isAPMode.load();
    ESP_LOGI(TAG, "Connecting to %s (attempt %d%s)...", ssid, _connectionRetries + 1,
             apActive ? ", fallback AP active" : "");
    WiFi.mode(apActive ? WIFI_AP_STA : WIFI_STA);
    applyPowerSave();
    char host[sizeof(_currentHostname)];
    copyHostname(host, sizeof(host));
    if (host[0] != '\0') {
        WiFi.setHostname(host);
    }
    connectStation(ssid, pass);
    // Downtime counts from the first attempt, so a device that boots while its
    // router is off also gets the fallback AP after 3 min.
    markStaDown(now);
    transitionTo(WiFiState::StaConnecting, now);
}

/**
 * @brief STA association succeeded.
 *
 * Tears down the fallback AP (if any) BEFORE publishing WIFI_CONNECTED so
 * subscribers re-evaluating isSetupMode()/getAPMode() see the final state.
 */
void WifiManager::onStaConnected(uint32_t now) {
    ESP_LOGI(TAG, "Connected successfully. IP: %s", WiFi.localIP().toString().c_str());
    if (_isAPMode.load()) {
        ESP_LOGI(TAG, "STA restored — tearing down fallback AP.");
        stopAccessPoint();
    }
    _connectionRetries = 0;
    _fallbackApSuppressed = false;
    _wifiDisconnectTimer.setInterval(0); // Reset downtime tracker
    updateMyUrl();

    // Initialise mDNS with the device hostname (issue #232)
    // end() first to cleanly restart on WiFi reconnections — prevents
    // "Service already exists" errors from duplicate addService() calls.
    MDNS.end();
    char host[sizeof(_currentHostname)];
    copyHostname(host, sizeof(host));
    if (MDNS.begin(host)) {
        MDNS.addService("http", "tcp", 80);
        ESP_LOGI(TAG, "mDNS started: %s.local", host);
    } else {
        ESP_LOGW(TAG, "mDNS failed to start");
    }

    if (_eventBus) {
        int count = _eventBus->publish(WifiEvents::WIFI_CONNECTED);
        ESP_LOGI(TAG, "Published WIFI_CONNECTED to %d subscribers", count);
    }
    transitionTo(WiFiState::Ready, now);
}

/**
 * @brief Start the captive soft-AP and DNS hijacker.
 *
 * Setup mode (no credentials): WIFI_AP, as before. With credentials this is
 * the fallback AP: WIFI_AP_STA so STA keeps retrying the stored network, and
 * portal auth stays enforced (Application follows isSetupMode(), not
 * getAPMode()). The network itself stays open; the portal behind it requires
 * login.
 */
void WifiManager::startAccessPoint(uint32_t now) {
    const bool fallback = hasCredentials();
    ESP_LOGI(TAG, "Starting Access Point (Captive Portal, %s)...",
             fallback ? "fallback AP+STA, auth enforced" : "first-boot setup");
    _isAPMode.store(true);
    WiFi.mode(fallback ? WIFI_AP_STA : WIFI_AP);

    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    char portalName[sizeof(_currentHostname)];
    copyAPSSID(portalName, sizeof(portalName));
    bool apResult = WiFi.softAP(portalName);
    ESP_LOGI(TAG, "softAP('%s') returned %s", portalName, apResult ? "true" : "FALSE");

    // Start DNS Hijacker
    _dnsServer = std::make_unique<DNSServer>();
    _dnsServer->start(53, "*", apIP);
    ESP_LOGI(TAG, "Access Point '%s' is online. IP: %s", portalName, apIP.toString().c_str());
    _lastApiRequestTime.store(now); // Reset AP timeout timer

    // Notify subscribers that AP mode is active
    if (_eventBus) {
        _eventBus->publish(WifiEvents::WIFI_AP_STARTED);
    }
}

/**
 * @brief Stop the captive AP and DNS hijacker if running.
 *
 * softAPdisconnect(true) disables only the AP interface: from WIFI_AP_STA the
 * radio drops to WIFI_STA and an established STA link is kept.
 */
void WifiManager::stopAccessPoint() {
    if (!_isAPMode.load()) return;
    if (_dnsServer) {
        _dnsServer->stop();
        _dnsServer.reset();
    }
    WiFi.softAPdisconnect(true);
    _isAPMode.store(false);
}

/**
 * @brief Periodic maintenance tick for the WiFi state machine.
 *
 * All WiFi.* calls happen here, on the main loop.
 */
void WifiManager::tick(uint32_t currentMillis) {
    // --- Handle deferred restart (set by connect/forget handlers) ---
    if (_restartRequested.load()) {
        _restarting.store(true);
        ESP_LOGI(TAG, "Executing deferred restart...");
        // Allow HTTP response to flush to the client before rebooting.
        // Keep _restartRequested/_restarting latched across delay(500) so
        // concurrent Core 0 handlers reject new writes cleanly, then hold
        // _persistOpMutex across esp_restart() so any in-flight LittleFS write
        // finishes before reset.
        delay(500);
        std::lock_guard<FreeRTOSMutex> persistLock(_persistOpMutex);
        _restartRequested.store(false);
        _restarting.store(false);
        esp_restart();
        return; // Unreachable, but makes intent clear
    }

    // --- Deferred reconnect (triggered by updateWiFi / out-of-band config import) ---
    // Honoured in every state: with the fallback AP the portal is reachable
    // while the state machine cycles StaConnecting/StaBackoff (#236 D-F).
    if (!isProvisioningOrRestarting() && _reconnectRequested.load()) {
        _reconnectRequested.store(false);
        ESP_LOGI(TAG, "Executing deferred reconnect with new credentials...");
        stopAccessPoint();
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        _connectionRetries = 0;
        _fallbackApSuppressed = false;
        _wifiDisconnectTimer.setInterval(0);
        transitionTo(WiFiState::Init, currentMillis);
        return;
    }

    // The captive portal DNS must be served in every state the AP can be up in.
    processDNS();

    // --- Fallback AP inactivity (credentials stored) ---
    // Drop only the fallback AP; STA keeps retrying. Wi-Fi is never switched
    // off on a device with credentials (#236 D-D).
    // Guard: _lastApiRequestTime may be set by an HTTP handler (via
    // notifyApiActivity → millis()) AFTER currentMillis was captured
    // at tick-start, causing unsigned underflow to ~4.3 billion.
    const uint32_t lastReq = _lastApiRequestTime.load();
    if (_isAPMode.load() && !isProvisioningOrRestarting() && hasCredentials() &&
        currentMillis > lastReq &&
        (currentMillis - lastReq) > AP_INACTIVITY_TIMEOUT_MS) {
        ESP_LOGW(TAG, "Fallback AP inactive for 1 hour. Dropping AP; STA retries continue.");
        stopAccessPoint(); // AP_STA -> STA; the STA interface stays up
        _fallbackApSuppressed = true;
    }

    switch (_currentState.load()) {
        case WiFiState::Init: {
            if (_stateTimer.check(currentMillis)) {
                if (hasCredentials()) {
                    startStaAttempt(currentMillis);
                } else {
                    ESP_LOGW(TAG, "No credentials found. Falling back to AP mode.");
                    transitionTo(WiFiState::ApStarting, currentMillis);
                }
            }
            break;
        }

        case WiFiState::StaConnecting:
            if (isProvisioningOrRestarting()) {
                break;
            }
            if (WiFi.status() == WL_CONNECTED) {
                onStaConnected(currentMillis);
            } else if (currentMillis - _stateTimer.getLastFire() > STA_ATTEMPT_TIMEOUT_MS) {
                // Abort the pending association without disabling the STA
                // interface (keeps a running fallback AP untouched).
                WiFi.disconnect(false);
                if (!hasCredentials()) {
                    // Credentials were erased mid-attempt: first-boot setup path.
                    ESP_LOGW(TAG, "WiFi connection timed out and no credentials remain. Starting setup AP.");
                    transitionTo(WiFiState::ApStarting, currentMillis);
                    break;
                }
                // STA-first (#236 D-A): never fall back to AP-only mode.
                _connectionRetries++;
                ESP_LOGW(TAG, "WiFi connection attempt %d timed out. Retrying in %u s.",
                         _connectionRetries, (unsigned)(staRetryDelayMs(_connectionRetries) / 1000));
                transitionTo(WiFiState::StaBackoff, currentMillis);
            }
            break;

        case WiFiState::StaBackoff:
            if (isProvisioningOrRestarting()) {
                break;
            }
            // The driver may still re-associate on its own during the wait.
            if (WiFi.status() == WL_CONNECTED) {
                onStaConnected(currentMillis);
                break;
            }
            // Long outage: raise the fallback AP alongside STA (#236 D-B).
            if (shouldStartFallbackAp(hasCredentials(), _isAPMode.load(), _fallbackApSuppressed,
                                      staDownMs(currentMillis))) {
                ESP_LOGW(TAG, "STA down for > %u s. Starting fallback AP alongside STA.",
                         (unsigned)(FALLBACK_AP_DELAY_MS / 1000));
                startAccessPoint(currentMillis);
            }
            if (currentMillis - _stateTimer.getLastFire() >= staRetryDelayMs(_connectionRetries)) {
                startStaAttempt(currentMillis);
            }
            break;

        case WiFiState::ApStarting:
            // Reached only without credentials: first-boot setup AP (unchanged).
            startAccessPoint(currentMillis);
            transitionTo(WiFiState::Ready, currentMillis);
            break;

        case WiFiState::Ready:
            if (_isAPMode.load()) {
                if (!isProvisioningOrRestarting() && hasCredentials()) {
                    // Credentials were applied while the setup AP was up
                    // (e.g. config import): leave setup mode via the normal
                    // reconnect path so STA-first applies from here on.
                    _reconnectRequested.store(true);
                    break;
                }
                // Setup AP 1-hour inactivity timeout (no credentials only).
                // Guard: see the underflow note above.
                if (!isProvisioningOrRestarting() &&
                    currentMillis > lastReq &&
                    (currentMillis - lastReq) > AP_INACTIVITY_TIMEOUT_MS) {
                    ESP_LOGW(TAG, "AP Mode inactive for 1 hour. Shutting down AP to save power.");
                    transitionTo(WiFiState::Shutdown, currentMillis);
                }
            } else if (WiFi.status() != WL_CONNECTED) {
                // STA link lost. If this is the first tick since disconnection,
                // start the downtime tracker and notify subscribers.
                if (_wifiDisconnectTimer.getInterval() == 0) {
                    markStaDown(currentMillis);
                    if (_eventBus) {
                        _eventBus->publish(WifiEvents::WIFI_DISCONNECTED);
                    }
                }

                // Give the driver a grace period to re-associate on its own,
                // then start the STA retry schedule.
                if (staDownMs(currentMillis) > STA_LINK_LOSS_GRACE_MS) {
                    ESP_LOGW(TAG, "Primary connection lost. Retrying...");
                    _connectionRetries = 0;
                    startStaAttempt(currentMillis);
                }
            } else if (_wifiDisconnectTimer.getInterval() != 0) {
                // Re-associated within the grace period: pair the earlier
                // WIFI_DISCONNECTED with WIFI_CONNECTED.
                onStaConnected(currentMillis);
            }
            break;

        case WiFiState::Shutdown:
            // Reached only from setup mode (no credentials).
            if (_isAPMode.load() && !isProvisioningOrRestarting()) {
                stopAccessPoint();
                WiFi.mode(WIFI_OFF);
                ESP_LOGI(TAG, "WiFi AP has been shut down.");
            }
            break;
    }
}

/**
 * @brief Handles DNS requests when in AP mode.
 */
void WifiManager::processDNS() {
    if (_isAPMode.load() && _dnsServer != nullptr) {
        _dnsServer->processNextRequest();
    }
}

/**
 * @brief Record that an API request was received.
 */
void WifiManager::notifyApiActivity() {
    _lastApiRequestTime.store(millis());
}

/**
 * @brief Set the app-supplied factory default hostname.
 *
 * Stored separately from currentHostname so resetConfig() can restore it
 * without depending on the app calling begin() with a parameter. Must be
 * called before Application::begin() (and therefore before resetConfig()
 * could run) to take effect on a factory-reset boot.
 *
 * @param hostname Default hostname to seed resetConfig() with (truncated to 32 chars).
 */
void WifiManager::setDefaultHostname(const char* hostname) {
    if (hostname != nullptr) {
        std::lock_guard<FreeRTOSMutex> guard(_credMutex);
        bool shouldUpdateCurrent = (strlen(_currentHostname) == 0 ||
                                    strcmp(_currentHostname, DEFAULT_HOSTNAME) == 0 ||
                                    strcmp(_currentHostname, _defaultHostname) == 0);
        strlcpy(_defaultHostname, hostname, sizeof(_defaultHostname));
        if (shouldUpdateCurrent) {
            strlcpy(_currentHostname, hostname, sizeof(_currentHostname));
        }
    }
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
void WifiManager::registerRoutes(PsychicHttpServer& server) {
    // --- GET /api/wifi/status ---
    server.on("/api/wifi/status", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        return this->handleStatus(request, response);
    });

    // --- GET /api/wifi/scan ---
    server.on("/api/wifi/scan", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        return this->handleScan(request, response);
    });

    // --- POST /api/wifi/test ---
    server.on("/api/wifi/test", HTTP_POST,
        [this](PsychicRequest* request, PsychicResponse* response) {
            return this->handleTest(request, response);
        }
    );

    // --- POST /api/wifi/connect ---
    server.on("/api/wifi/connect", HTTP_POST,
        [this](PsychicRequest* request, PsychicResponse* response) {
            return this->handleConnect(request, response);
        }
    );

    // --- POST /api/wifi/forget ---
    server.on("/api/wifi/forget", HTTP_POST,
        [this](PsychicRequest* request, PsychicResponse* response) {
            return this->handleForget(request, response);
        }
    );
}

// =============================================================================
// IMCPContributor Implementation
// =============================================================================

void WifiManager::registerMCPTools(MCPToolRegistry& registry) {
    registry.addTool("wifi_get_status", "Get current WiFi connection status and diagnostics",
        [](void* context, JsonObject& args, JsonObject& result) {
            WifiManager* self = static_cast<WifiManager*>(context);
            if (self->_isAPMode.load()) {
                result["connected"] = false;
                result["ip"] = WiFi.softAPIP().toString();
                result["gateway"] = "";
                result["subnet"] = "";
                result["rssi"] = 0;
                result["bssid"] = "";
                result["ap_mode"] = true;
            } else {
                result["connected"] = (WiFi.status() == WL_CONNECTED);
                result["ip"] = WiFi.localIP().toString();
                result["gateway"] = WiFi.gatewayIP().toString();
                result["subnet"] = WiFi.subnetMask().toString();
                result["rssi"] = WiFi.RSSI();
                result["bssid"] = WiFi.BSSIDstr();
                result["ap_mode"] = false;
            }
        }, this, false);

    registry.addTool("wifi_scan_networks", "Force a WiFi scan and return discovered networks",
        [](void* context, JsonObject& args, JsonObject& result) {
            WifiManager* self = static_cast<WifiManager*>(context);
            if (self->_isAPMode.load()) {
                WiFi.mode(WIFI_AP_STA);
            }
#if defined(WITH_BLE)
            int16_t count = WiFi.scanNetworks(false, false, false, 120);
#else
            int16_t count = WiFi.scanNetworks(false);
#endif
            if (count < 0) {
                result["error"] = "Scan failed";
            } else {
                JsonArray networks = result["networks"].to<JsonArray>();
                for (int16_t i = 0; i < count && i < MAX_SCAN_RESULTS; ++i) {
                    JsonObject net = networks.add<JsonObject>();
                    net["ssid"] = WiFi.SSID(i);
                    net["rssi"] = WiFi.RSSI(i);
                    net["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                }
                WiFi.scanDelete();
            }
            // Only the setup AP returns to AP-only; the fallback AP must keep
            // STA running so it can keep retrying the stored network (#236).
            if (self->isSetupMode()) {
                WiFi.mode(WIFI_AP);
            }
        }, this, true);

    registry.addTool("wifi_forget", "Erase WiFi credentials and restart to AP mode",
        [](void* context, JsonObject& args, JsonObject& result) {
            WifiManager* self = static_cast<WifiManager*>(context);
            ProvisioningGuard guard(*self);
            if (guard.blockedByRestart) {
                result["success"] = false;
                result["isError"] = true;
                result["error"] = "Restart already in progress";
                return;
            }
            if (!self->stageAndPersistErasure()) {
                result["success"] = false;
                result["isError"] = true;
                result["error"] = "Failed to erase WiFi credentials";
                return;
            }
            self->commitStagedErasure();
            // Do NOT call WiFi.disconnect() here: wifi_forget runs on Core 0
            // HTTP MCP handler and must keep STA up until the response flushes;
            // scheduleRestart() + begin() on reboot handles WiFi reset.
            self->scheduleRestart();
            result["success"] = true;
        }, this, true);
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
esp_err_t WifiManager::handleStatus(PsychicRequest* request, PsychicResponse* response) {
    this->notifyApiActivity();

    char ssidBuf[sizeof(_wifiSsid)];
    bool passExists = false;
    {
        std::lock_guard<FreeRTOSMutex> guard(_credMutex);
        strlcpy(ssidBuf, _wifiSsid, sizeof(ssidBuf));
        passExists = (_wifiPass[0] != '\0');
    }
    const bool apMode = _isAPMode.load();

    JsonDocument doc;
    doc["ssid"]           = ssidBuf;
    doc["passExists"]     = passExists;
    doc["ap_mode"]        = apMode;
    doc["ip"]             = WiFi.localIP().toString();
    doc["mac"]            = WiFi.macAddress();

    // --- STA-mode specific fields ---
    if (!apMode && WiFi.status() == WL_CONNECTED) {
        doc["mode"]    = "STA";
        doc["rssi"]    = WiFi.RSSI();
        doc["channel"] = WiFi.channel();
        doc["gateway"] = WiFi.gatewayIP().toString();
    } else if (apMode) {
        doc["mode"]    = "AP";
        doc["ip"]      = WiFi.softAPIP().toString();
        doc["channel"] = 1; // Default AP channel
    } else {
        doc["mode"]    = "DISCONNECTED";
    }

    std::string output;
    serializeJson(doc, output);
    response->send(200, "application/json", output.c_str());
    return ESP_OK;
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
esp_err_t WifiManager::handleScan(PsychicRequest* request, PsychicResponse* response) {
    this->notifyApiActivity();

    int16_t scanResult = WiFi.scanComplete();

    if (scanResult == WIFI_SCAN_FAILED) {
        // --- No scan in progress — start one ---
        if (_isAPMode.load()) {
            // Must switch to AP+STA so the STA radio can scan without
            // tearing down the AP.  Plain WiFi.scanNetworks(true) in
            // AP-only mode causes the driver to drop to STA on completion.
            WiFi.mode(WIFI_AP_STA);
        }
        ESP_LOGI(TAG, "Starting async WiFi scan...");
#if defined(WITH_BLE)
        // Use 120ms max channel time when coexistence with BLE is active to prevent radio starvation
        WiFi.scanNetworks(true, false, false, 120);
#else
        WiFi.scanNetworks(true); // true = async
#endif
        response->send(202, "application/json", "{\"status\":\"scanning\"}");
        return ESP_OK;
    }

    if (scanResult == WIFI_SCAN_RUNNING) {
        // --- Scan is still in progress ---
        response->send(202, "application/json", "{\"status\":\"scanning\"}");
        return ESP_OK;
    }

    // --- Scan complete — process results ---
    ESP_LOGI(TAG, "Scan complete. Found %d networks.", scanResult);

    JsonDocument doc;
    JsonArray networks = doc["networks"].to<JsonArray>();

    // --- Build deduplicated, RSSI-sorted list ---
    // Use a simple O(n²) dedup since scan results are small (typically <30)
    struct ScanEntry {
        std::string ssid;
        int32_t rssi;
        bool secure;
        uint8_t channel;
    };

    // Temporary storage for dedup
    ScanEntry entries[MAX_SCAN_RESULTS];
    int entryCount = 0;

    for (int i = 0; i < scanResult && entryCount < MAX_SCAN_RESULTS; i++) {
        std::string ssid = WiFi.SSID(i).c_str();
        if (ssid.empty()) continue; // Skip hidden networks with no SSID

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

    // NOTE: We intentionally stay in AP_STA mode after scanning.
    // Switching back to WIFI_AP reinitializes the AP interface and
    // disconnects all connected clients. The idle STA radio in AP_STA
    // mode uses negligible extra power.

    std::string output;
    serializeJson(doc, output);
    response->send(200, "application/json", output.c_str());
    return ESP_OK;
}

/**
 * @brief Handle POST /api/wifi/test.
 *
 * Accepts {ssid, pass} JSON body. Tests credentials without saving.
 * Returns {status:"ok", ip:"x.x.x.x"} on success or {status:"failed"} on failure.
 * Enforces 1KB body size limit — rejects with HTTP 413 if exceeded.
 */
esp_err_t WifiManager::handleTest(PsychicRequest* request, PsychicResponse* response) {
    this->notifyApiActivity();

    // --- Per-route body size guard (PR-04a/WO1) ---
    if (request->contentLength() > BODY_LIMIT_WIFI_TEST) {
        response->send(413, "application/json", "{\"error\":\"Payload too large\"}");
        return ESP_FAIL;
    }

    const char* body = request->bodyCStr();
    if (!body || strlen(body) == 0) {
        response->send(400, "application/json", "{\"error\":\"body_required\"}");
        return ESP_FAIL;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        response->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid JSON\"}");
        return ESP_FAIL;
    }

    const char* ssid = doc["ssid"] | "";
    const char* pass = doc["pass"] | "";

    if (strlen(ssid) == 0) {
        response->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Missing SSID\"}");
        return ESP_FAIL;
    }

    // --- Test connection ---
    std::string ipOut;
    bool success = this->testConnection(ssid, pass, ipOut);

    if (success) {
        JsonDocument resp;
        resp["status"] = "ok";
        resp["ip"]     = ipOut;
        std::string output;
        serializeJson(resp, output);
        response->send(200, "application/json", output.c_str());
    } else {
        response->send(200, "application/json", "{\"status\":\"failed\"}");
    }
    return ESP_OK;
}

/**
 * @brief Handle POST /api/wifi/connect.
 *
 * Accepts {ssid, pass} JSON body. Saves credentials via KeyManager (AES-256)
 * and ConfigManager, returns 200, and schedules a deferred device restart
 * so the WiFi stack reconnects with the new credentials.
 * Enforces 1KB body size limit — rejects with HTTP 413 if exceeded.
 */
esp_err_t WifiManager::handleConnect(PsychicRequest* request, PsychicResponse* response) {
    this->notifyApiActivity();

    // --- Per-route body size guard ---
    if (request->contentLength() > BODY_LIMIT_WIFI_CONNECT) {
        response->send(413, "application/json", "{\"error\":\"Payload too large\"}");
        return ESP_FAIL;
    }

    const char* body = request->bodyCStr();
    if (!body || strlen(body) == 0) {
        response->send(400, "application/json", "{\"error\":\"body_required\"}");
        return ESP_FAIL;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        response->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid JSON\"}");
        return ESP_FAIL;
    }

    const char* ssid = doc["ssid"] | "";
    const char* pass = doc["pass"] | "";

    if (strlen(ssid) == 0) {
        response->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Missing SSID\"}");
        return ESP_FAIL;
    }

    {
        ProvisioningGuard guard(*this);
        if (guard.blockedByRestart) {
            response->send(503, "application/json", "{\"status\":\"error\",\"msg\":\"Restart already in progress\"}");
            return ESP_FAIL;
        }
        // Stage and persist credentials without mutating _wifiSsid or setting
        // _reconnectRequested while Core 0 is still writing flash / sending 200.
        if (!stageAndPersistCredentials(ssid, pass)) {
            response->send(500, "application/json", "{\"status\":\"error\",\"msg\":\"Failed to save credentials\"}");
            return ESP_FAIL;
        }

        response->send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"Credentials saved. Restarting...\"}");

        commitStagedCredentials(pass);

        // Set _restartRequested BEFORE ProvisioningGuard clears
        // _provisioningInProgress so Core 1 tick() never triggers a live
        // reconnect between credential commit and deferred restart.
        this->scheduleRestart();
    }
    return ESP_OK;
}

/**
 * @brief Handle POST /api/wifi/forget.
 *
 * Clears stored SSID, password (KeyManager), and config (ConfigManager).
 * Returns 200 and schedules a deferred restart so the device boots into AP mode.
 * Enforces 256B body size limit — rejects with HTTP 413 if exceeded.
 */
esp_err_t WifiManager::handleForget(PsychicRequest* request, PsychicResponse* response) {
    this->notifyApiActivity();

    // --- Per-route body size guard ---
    if (request->contentLength() > BODY_LIMIT_WIFI_FORGET) {
        response->send(413, "application/json", "{\"error\":\"Payload too large\"}");
        return ESP_FAIL;
    }

    ESP_LOGW(TAG, "Forget WiFi requested — erasing credentials...");

    {
        ProvisioningGuard guard(*this);
        if (guard.blockedByRestart) {
            response->send(503, "application/json", "{\"status\":\"error\",\"msg\":\"Restart already in progress\"}");
            return ESP_FAIL;
        }
        if (!stageAndPersistErasure()) {
            response->send(500, "application/json", "{\"status\":\"error\",\"msg\":\"Failed to erase credentials\"}");
            return ESP_FAIL;
        }

        response->send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"WiFi credentials erased. Restarting to AP mode...\"}");

        commitStagedErasure();

        // --- Schedule deferred restart to AP mode ---
        this->scheduleRestart();
    }
    return ESP_OK;
}

/**
 * @brief Schedule a deferred ESP restart.
 *
 * Sets the _restartRequested flag which is checked at the top of tick().
 * This is safe to call from HTTP handlers — the actual restart happens
 * on the main loop after the HTTP response has been flushed.
 */
void WifiManager::scheduleRestart() {
    _restartRequested.store(true);
    ESP_LOGI(TAG, "Deferred restart scheduled");
}

// =============================================================================
// Credential Management
// =============================================================================

bool WifiManager::stageAndPersistCredentials(const char* ssid, const char* pass) {
    {
        std::lock_guard<FreeRTOSMutex> guard(_credMutex);
        if (ssid != nullptr) {
            strlcpy(_pendingSsid, ssid, sizeof(_pendingSsid));
            _hasPendingSsid = true;
        } else {
            _hasPendingSsid = false;
            memset(_pendingSsid, 0, sizeof(_pendingSsid));
        }
    }

    bool ok = true;
    if (pass != nullptr && strlen(pass) > 0) {
        char boundedPass[sizeof(_wifiPass)];
        strlcpy(boundedPass, pass, sizeof(boundedPass));
        const auto redaction = Redactor::instance().registerSecret(boundedPass);
        if (redaction == Redactor::RegisterResult::Full ||
            redaction == Redactor::RegisterResult::TooLong) {
            ESP_LOGE(TAG, "WiFi password will NOT be redacted from logs: %s",
                     Redactor::describe(redaction));
        }

        // Persist password via KeyManager (AES-256 encrypted) using the same
        // bounded buffer committed to _wifiPass and registered with Redactor.
        if (_keyManager != nullptr) {
            if (_keyManager->addKey("wifi_password", boundedPass, "WiFi", true, true)) {
                ESP_LOGI(TAG, "WiFi password persisted via KeyManager");
            } else {
                ESP_LOGE(TAG, "CRITICAL: Failed to persist WiFi password!");
                ok = false;
            }
        } else {
            ESP_LOGW(TAG, "stageAndPersistCredentials: KeyManager not wired — password not persisted!");
        }
    }

    // Persist via ConfigManager gateway (serializeConfig emits _pendingSsid)
    if (ok) {
        if (_configManager != nullptr) {
            if (!_configManager->saveModuleConfig(this)) {
                ESP_LOGE(TAG, "CRITICAL: Failed to persist WiFi config via ConfigManager!");
                ok = false;
            }
        } else {
            ESP_LOGW(TAG, "stageAndPersistCredentials: ConfigManager not wired — config not saved!");
        }
    }

    return ok;
}

void WifiManager::commitStagedCredentials(const char* pass) {
    std::lock_guard<FreeRTOSMutex> guard(_credMutex);
    if (_hasPendingSsid) {
        strlcpy(_wifiSsid, _pendingSsid, sizeof(_wifiSsid));
        _hasPendingSsid = false;
        memset(_pendingSsid, 0, sizeof(_pendingSsid));
    }
    if (pass != nullptr && strlen(pass) > 0) {
        strlcpy(_wifiPass, pass, sizeof(_wifiPass));
    }
}

bool WifiManager::stageAndPersistErasure() {
    {
        std::lock_guard<FreeRTOSMutex> lock(_credMutex);
        memset(_pendingSsid, 0, sizeof(_pendingSsid));
        _hasPendingSsid = true;
    }

    bool ok = true;
    // --- Delete encrypted password from KeyManager ---
    if (_keyManager != nullptr) {
        KeyInfo info;
        const bool keyExisted = _keyManager->getKeyMetadata("wifi_password", info);
        if (_keyManager->deleteKey("wifi_password")) {
            ESP_LOGI(TAG, "WiFi password removed from KeyManager");
        } else if (keyExisted || hasPassword()) {
            ESP_LOGE(TAG, "CRITICAL: Failed to delete WiFi password from KeyManager!");
            ok = false;
        }
    }

    // --- Persist empty config via ConfigManager ---
    if (ok && _configManager != nullptr) {
        if (_configManager->saveModuleConfig(this)) {
            ESP_LOGI(TAG, "WiFi config cleared via ConfigManager");
        } else {
            ESP_LOGE(TAG, "CRITICAL: Failed to clear WiFi config via ConfigManager!");
            ok = false;
        }
    }

    return ok;
}

void WifiManager::commitStagedErasure() {
    std::lock_guard<FreeRTOSMutex> lock(_credMutex);
    memset(_wifiSsid, 0, sizeof(_wifiSsid));
    memset(_wifiPass, 0, sizeof(_wifiPass));
    memset(_pendingSsid, 0, sizeof(_pendingSsid));
    _hasPendingSsid = false;
}

/**
 * @brief Save new WiFi credentials and request a deferred reconnect.
 *
 * Persists SSID via ConfigManager and password via KeyManager (AES-256).
 * Sets _reconnectRequested so tick() will tear down and reconnect on the
 * main loop. This avoids destroying the WiFi stack from within an HTTP
 * handler (AsyncWebServer runs on the same connection being torn down).
 */
void WifiManager::updateWiFi(const char* ssid, const char* pass) {
    ProvisioningGuard guard(*this);
    if (guard.blockedByRestart) {
        ESP_LOGW(TAG, "updateWiFi: Restart already pending — ignoring update");
        return;
    }
    if (!stageAndPersistCredentials(ssid, pass)) {
        ESP_LOGE(TAG, "updateWiFi: Failed to persist credentials — aborting reconnect");
        return;
    }
    commitStagedCredentials(pass);

    // --- Request deferred reconnect (handled safely in tick()) ---
    _reconnectRequested.store(true);
    ESP_LOGI(TAG, "Credentials saved — deferred reconnect requested");
}

/**
 * @brief Test a WiFi connection without permanently changing stored credentials.
 */
bool WifiManager::testConnection(const char* ssid, const char* pass, std::string& ipOut) {
    if (ssid == nullptr || ssid[0] == '\0') {
        ESP_LOGW(TAG, "WIFI_TEST: Missing SSID");
        return false;
    }
    ESP_LOGI(TAG, "WIFI_TEST: Starting connection test...");
    char storedSsid[sizeof(_wifiSsid)];
    char storedPass[sizeof(_wifiPass)];
    {
        std::lock_guard<FreeRTOSMutex> guard(_credMutex);
        strlcpy(storedSsid, _wifiSsid, sizeof(storedSsid));
        strlcpy(storedPass, _wifiPass, sizeof(storedPass));
    }

    const char* realPass = pass;
    if (pass == nullptr || strlen(pass) == 0) {
        ESP_LOGI(TAG, "WIFI_TEST: Using stored credentials for test.");
        realPass = storedPass;
    }

    if (WiFi.status() == WL_CONNECTED && strcmp(WiFi.SSID().c_str(), ssid) == 0 && strcmp(realPass, storedPass) == 0) {
        ipOut = WiFi.localIP().toString().c_str();
        ESP_LOGI(TAG, "WIFI_TEST: Already connected to this network. IP: %s", ipOut.c_str());
        return true;
    }

    WiFi.disconnect(true);
    // HARDWARE SETTLING: ESP32 WiFi stack requires ~200ms between disconnect()
    // and subsequent begin() to release the radio hardware. Without this delay,
    // WiFi.begin() can silently fail or connect to the wrong BSSID. Verified
    // empirically on ESP32-S3; the ESP-IDF WiFi driver does not guarantee
    // immediate readiness after disconnect(true).
    delay(200);

    connectStation(ssid, realPass);
    
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        // BLOCKING POLL: testConnection() is intentionally synchronous — it
        // blocks the calling context (web handler) until the connection result
        // is known. 500ms × 20 iterations = 10s max wait, matching typical
        // WPA2 association + DHCP time.
        delay(500);
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        ipOut = WiFi.localIP().toString().c_str();
        ESP_LOGI(TAG, "WIFI_TEST: Success! IP: %s", ipOut.c_str());
        return true;
    } else {
        ESP_LOGW(TAG, "WIFI_TEST: Failed. Reconnecting to primary credentials.");
        {
            std::lock_guard<FreeRTOSMutex> guard(_credMutex);
            strlcpy(storedSsid, _wifiSsid, sizeof(storedSsid));
            strlcpy(storedPass, _wifiPass, sizeof(storedPass));
        }
        if (strlen(storedSsid) > 0) {
            WiFi.disconnect(true);
            // HARDWARE SETTLING: Short delay after disconnect() before
            // reconnecting to the primary network. 100ms is sufficient here
            // because the radio is transitioning to a known-good configuration
            // (the stored STA credentials) rather than a new network.
            delay(100);
            connectStation(storedSsid, storedPass);
        }
        return false;
    }
}

/**
 * @brief Update the internal URL string for the Web GUI.
 */
void WifiManager::updateMyUrl() {
    IPAddress ip = WiFi.localIP();
    snprintf(_myUrl, sizeof(_myUrl), "http://%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

/**
 * @brief Returns true if WiFi has been disconnected for longer than the timeout period (3 minutes).
 */
bool WifiManager::isWifiPersistentError() const {
    // The fallback AP (credentials stored) does not mask an STA outage.
    if (isSetupMode() || (WiFi.status() == WL_CONNECTED)) return false;
    if (_wifiDisconnectTimer.getInterval() == 0) return false;
    
    // Over 3 minutes of continuous disconnection
    return (_wifiDisconnectTimer.getInterval() > 0 && (millis() - _wifiDisconnectTimer.getLastFire() > 180000));
}
