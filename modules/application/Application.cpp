/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/application/Application.cpp
 * Description: Implementation of the Application composition root.
 *              All ESP-IDF API calls (esp_restart, heap APIs, LittleFS.begin)
 *              are wrapped in #ifndef NATIVE_BUILD guards so the GoogleTest
 *              native runner can compile and execute the class without hardware
 *              headers. The two-tier vector model is enforced by _compactAndLock().
 *
 *              ConfigManager is now the centralised configuration gateway.
 *              The old _configurables vector and registerConfigurable() are
 *              removed. Modules register with _configManager.registerModule()
 *              and config is loaded via _configManager.beginAll().
 *
 *              Core domain services (WifiManager, otaUpdateManager,
 *              TimeManager, KeyManager) are Application-owned members,
 *              wired into the boot sequence during begin().
 *
 * Exported Classes:
 * - Application: see Application.hpp
 */

#include "Application.hpp"
#include "../otaUpdateManager/otaUpdateManager.hpp"

// ---------------------------------------------------------------------------
// Platform-conditional includes
// ESP-IDF / Arduino headers are excluded from the native GoogleTest build.
// ---------------------------------------------------------------------------
#ifndef NATIVE_BUILD
    #include <Arduino.h>          // Serial, millis(), etc.
    #include <LittleFS.h>         // LittleFS.begin()
    #include "esp_system.h"       // esp_restart()
    #include "esp_heap_caps.h"    // heap_caps_get_free_size(), heap_caps_get_minimum_free_size()
#endif

#include "platform/LogShim.h"

static const char* TAG = "Application";

// Destructor — clean up heap-allocated otaUpdateManager
Application::~Application() {
    delete _otaManager;
    _otaManager = nullptr;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

/**
 * @brief Execute the full framework boot sequence.
 *
 * The boot order respects the dependency tree strictly:
 *   EventBus (no deps) → LittleFS → FrameworkConfig → DeviceCrypto →
 *   KeyManager → ConfigManager (register + beginAll) → WIFI_SETUP →
 *   WifiManager → APP_SETUP → TimeManager → DataManager → OTA →
 *   WebServer + MCP → _compactAndLock() → _reportHeap() → RUNNING
 *
 * @param config Framework-level configuration (hostname, timezone, etc.)
 * @return true on success; false if a fatal error prevents normal operation.
 */
bool Application::begin(const FrameworkConfig& config) {
    // --- Step 1: Boot banner ---
    // Print the firmware version so the first log line is always identifiable.
    ESP_LOGI(TAG, "ESP32 App Foundation v%s — booting", APP_FOUNDATION_VERSION);

    // --- Step 2: Enter BOOTING state ---
    setState(AppState::BOOTING);

    // --- Step 3: EventBus.begin() ---
    // FIRST service to init — zero dependencies, pure in-memory.
    // Starting early enables all subsequent services to publish boot-time events.
    _eventBus.begin();

    // --- Step 4: Mount LittleFS ---
    // Foundation for all persistence. Only available on hardware;
    // in NATIVE_BUILD we skip this.
#ifndef NATIVE_BUILD
    if (!LittleFS.begin(true, "/littlefs", 10, "littlefs")) {
        // formatOnFail = true: first-boot will auto-format, subsequent failures
        // indicate a hardware fault — abort and return false.
        ESP_LOGE(TAG, "LittleFS mount failed — cannot continue boot");
        return false;
    }
    ESP_LOGI(TAG, "LittleFS mounted successfully");
#else
    ESP_LOGI(TAG, "LittleFS mount skipped (NATIVE_BUILD)");
#endif

    // --- Step 5: Store FrameworkConfig internally ---
    _config = config;
    ESP_LOGI(TAG, "FrameworkConfig stored: hostname=%s timezone=%s",
             _config.hostname, _config.timezone);

    // --- Step 6: DeviceCrypto + KeyManager ---
    // DeviceCrypto needs filesystem for key material; KeyManager needs DeviceCrypto.
    _deviceCrypto.begin();
    _keyManager.begin(&_deviceCrypto);

    // --- Step 7: ConfigManager — register all framework iConfigurable modules ---
    // WifiManager: inject ConfigManager dependency, then register with it.
    _wifiManager.setConfigManager(&_configManager);
    _configManager.registerModule(&_wifiManager);
    // Downstream modules (U8g2DisplayModule, etc.) are registered by app code
    // via getConfigManager().registerModule() before begin() is called.

    // --- Step 8: ConfigManager.beginAll() — load all persisted config ---
    // Iterates the registry, loads each module's namespace file from
    // /mod/<ns>.json, runs version checks and migrations, and calls
    // applyConfig() on each module.
    // ALL persisted config is now available before any service starts.
    _configManager.beginAll();

    // --- Step 9: Enter WIFI_SETUP state ---
    setState(AppState::WIFI_SETUP);

    // --- Step 10: WifiManager.begin() ---
    // Now has persisted SSID/hostname from ConfigManager (step 8).
    _wifiManager.begin(_config.hostname);
    ESP_LOGI(TAG, "WiFi setup initiated");

    // --- Step 11: Enter APP_SETUP state ---
    setState(AppState::APP_SETUP);

    // --- Step 12: TimeManager — after WiFi, needs network for NTP ---
    _timeManager.begin(_config.timezone);

    // --- Step 13: DataManager — background data workers ---
    _dataManager.begin();

    // --- Step 14: OTA — heap-allocate (header is forward-declared to avoid LDF issues) ---
    _otaManager = new otaUpdateManager();
    _otaManager->begin(*this);

    // --- Step 15: Register core web contributors and tickables ---
    _webContributors.push_back(&_wifiManager);
    _webContributors.push_back(&_configManager);
    _webContributors.push_back(_otaManager);
    _webContributors.push_back(&_mcpServer);

    _tickables.push_back(&_wifiManager);
    _tickables.push_back(&_timeManager);

    // --- Step 16: WebServerManager + McpServer begin() ---
    _webServerManager.begin(_webContributors);

    // MCP server: pass the ConfigManager reference for config operations
    _mcpServer.begin(_mcpContributors, &_configManager, &_config);

    // --- Step 17: Lock all registration vectors ---
    // No further registrations are permitted after this point.
    _compactAndLock();

    // --- Step 18: Publish system heap event + heap report ---
#ifndef NATIVE_BUILD
    uint32_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    _eventBus.publish("system.heap", &freeHeap);
#endif
    _reportHeap();

    // --- Step 19: Enter RUNNING state ---
    setState(AppState::RUNNING);

    ESP_LOGI(TAG, "Boot sequence complete — entering RUNNING state");
    return true;
}

/**
 * @brief Dispatch the periodic update tick to all registered ITickable modules.
 *
 * The guard ensures tick() is a no-op until AppState::RUNNING so no module
 * receives a tick during the boot phase.
 */
void Application::tick() {
    // Guard: do not tick until fully running
    if (_state != AppState::RUNNING) return;

#ifdef NATIVE_BUILD
    // In native builds, drive DataManager synchronously for tests
    _dataManager.processNext();
#endif

    // Dispatch to all registered tickables in registration order
    for (auto* module : _tickables) {
        module->tick();
    }

    // MCP server: poll Serial for incoming JSON-RPC requests
    _mcpServer.tick();
}

/**
 * @brief Log the soft-reset reason and restart the device.
 *
 * esp_restart() is wrapped in NATIVE_BUILD so the GoogleTest suite can call
 * softReset() and verify it completes without crashing the test process.
 */
void Application::softReset() {
    ESP_LOGW(TAG, "Application::softReset() triggered — restarting device");

#ifndef NATIVE_BUILD
    esp_restart(); // Hardware restart — this call never returns
#endif
    // In NATIVE_BUILD: function returns normally (no-op for test verification)
}

// ---------------------------------------------------------------------------
// Module Registration (Tier 1 — pre-lock)
// All register* methods share the same null-check + locked-check guard.
// ---------------------------------------------------------------------------

/**
 * @brief Register a module that participates in the Application tick loop.
 */
bool Application::registerTickable(ITickable* module) {
    if (_locked) {
        ESP_LOGE(TAG, "registerTickable() rejected — registration is closed after begin()");
        return false;
    }
    if (module == nullptr) {
        ESP_LOGE(TAG, "registerTickable() rejected — null pointer");
        return false;
    }
    _tickables.push_back(module);
    return true;
}

/**
 * @brief Set (or replace pre-lock) the single status-reporter adapter.
 *
 * Last-write-wins semantics; only one IStatusReporter is active at a time.
 */
bool Application::registerStatusReporter(IStatusReporter* reporter) {
    if (_locked) {
        ESP_LOGE(TAG, "registerStatusReporter() rejected — registration is closed after begin()");
        return false;
    }
    if (reporter == nullptr) {
        ESP_LOGE(TAG, "registerStatusReporter() rejected — null pointer");
        return false;
    }
    // Last-write-wins: replace any previously registered reporter
    _statusReporter = reporter;
    return true;
}

/**
 * @brief Register a module that contributes REST API routes to the web server.
 */
bool Application::registerWebContributor(IWebContributor* contributor) {
    if (_locked) {
        ESP_LOGE(TAG, "registerWebContributor() rejected — registration is closed after begin()");
        return false;
    }
    if (contributor == nullptr) {
        ESP_LOGE(TAG, "registerWebContributor() rejected — null pointer");
        return false;
    }
    _webContributors.push_back(contributor);
    return true;
}

/**
 * @brief Register a module that contributes MCP tools to the MCP server.
 */
bool Application::registerMcpContributor(IMcpContributor* contributor) {
    if (_locked) {
        ESP_LOGE(TAG, "registerMcpContributor() rejected — registration is closed after begin()");
        return false;
    }
    if (contributor == nullptr) {
        ESP_LOGE(TAG, "registerMcpContributor() rejected — null pointer");
        return false;
    }
    _mcpContributors.push_back(contributor);
    return true;
}

/**
 * @brief Register a background data-fetch worker with the DataManager.
 */
bool Application::registerDataSource(iDataSource* source) {
    if (_locked) {
        ESP_LOGE(TAG, "registerDataSource() rejected — registration is closed after begin()");
        return false;
    }
    if (source == nullptr) {
        ESP_LOGE(TAG, "registerDataSource() rejected — null pointer");
        return false;
    }
    _dataSources.push_back(source);
    return true;
}

// ---------------------------------------------------------------------------
// Core Services Accessors
// ---------------------------------------------------------------------------

EventBus& Application::getEventBus() {
    return _eventBus;
}

DataManager& Application::getDataManager() {
    return _dataManager;
}

WebServerManager& Application::getWebServerManager() {
    return _webServerManager;
}

McpServer& Application::getMcpServer() {
    return _mcpServer;
}

ConfigManager& Application::getConfigManager() {
    return _configManager;
}

WifiManager& Application::getWifiManager() {
    return _wifiManager;
}

otaUpdateManager& Application::getOtaManager() {
    return *_otaManager;
}

TimeManager& Application::getTimeManager() {
    return _timeManager;
}

KeyManager& Application::getKeyManager() {
    return _keyManager;
}

const FrameworkConfig& Application::getConfig() const {
    return _config;
}

// ---------------------------------------------------------------------------
// State Accessors
// ---------------------------------------------------------------------------

/**
 * @brief Query the current application lifecycle phase.
 */
AppState Application::getState() const {
    return _state;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

/**
 * @brief Advance the lifecycle state and log the transition.
 */
void Application::setState(AppState state) {
    _state = state;

    // Emit a human-readable state name for log correlation
    const char* name = "UNKNOWN";
    switch (state) {
        case AppState::BOOTING:    name = "BOOTING";    break;
        case AppState::WIFI_SETUP: name = "WIFI_SETUP"; break;
        case AppState::APP_SETUP:  name = "APP_SETUP";  break;
        case AppState::RUNNING:    name = "RUNNING";    break;
    }
    ESP_LOGI(TAG, "State → %s", name);
}

/**
 * @brief Lock all registration vectors after the boot phase completes.
 *
 * Calls shrink_to_fit() on every std::vector to release excess heap capacity
 * reserved during push_back() reallocation, then sets _locked = true to
 * enforce Tier-2 rejection on any further registration attempt.
 */
void Application::_compactAndLock() {
    // Release excess vector capacity to reclaim heap before entering RUNNING
    _tickables.shrink_to_fit();
    _webContributors.shrink_to_fit();
    _mcpContributors.shrink_to_fit();
    _dataSources.shrink_to_fit();

    _locked = true;

    ESP_LOGI(TAG, "Registry locked — tickables=%zu "
             "webContributors=%zu mcpContributors=%zu dataSources=%zu statusReporter=%s",
             _tickables.size(),
             _webContributors.size(),
             _mcpContributors.size(),
             _dataSources.size(),
             (_statusReporter != nullptr) ? "registered" : "none");
}

/**
 * @brief Log the current free-heap and minimum free-heap figures.
 *
 * Uses the ESP-IDF heap_caps API which is only available on hardware.
 * Wrapped in NATIVE_BUILD guard to keep the native test build self-contained.
 */
void Application::_reportHeap() {
#ifndef NATIVE_BUILD
    // heap_caps_get_free_size returns available bytes in a specific memory region.
    uint32_t freeHeap    = heap_caps_get_free_size(MALLOC_CAP_8BIT);      // Current free bytes
    uint32_t minFreeHeap = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT); // Historical low-watermark

    ESP_LOGI(TAG, "Heap report — free: %" PRIu32 " B  min-free: %" PRIu32 " B",
             freeHeap, minFreeHeap);
#else
    ESP_LOGI(TAG, "Heap report skipped (NATIVE_BUILD)");
#endif
}
