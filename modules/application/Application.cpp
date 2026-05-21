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
 * Exported Classes:
 * - Application: see Application.hpp
 */

#include "Application.hpp"

// ---------------------------------------------------------------------------
// Platform-conditional includes
// ESP-IDF / Arduino headers are excluded from the native GoogleTest build.
// ---------------------------------------------------------------------------
#ifndef NATIVE_BUILD
    #include <Arduino.h>          // Serial, millis(), etc.
    #include <LittleFS.h>         // LittleFS.begin()
    #include "esp_system.h"       // esp_restart()
    #include "esp_heap_caps.h"    // heap_caps_get_free_size(), heap_caps_get_minimum_free_size()
#else
    // Minimal stdio for native logging stub
    #include <cstdio>
#endif

// ---------------------------------------------------------------------------
// Logging helpers
// On hardware we use the ESP-IDF log macros.
// In the native build we map them to printf so tests capture output without
// pulling in Arduino/ESP-IDF headers.
// ---------------------------------------------------------------------------
#ifndef NATIVE_BUILD
    #include "esp_log.h"
    static const char* TAG = "Application"; // ESP-IDF log tag
    #define APP_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
    #define APP_LOGW(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
    #define APP_LOGE(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#else
    // Thin printf shims — keep messages human-readable in GoogleTest output
    #define APP_LOGI(fmt, ...) printf("[I] Application: " fmt "\n", ##__VA_ARGS__)
    #define APP_LOGW(fmt, ...) printf("[W] Application: " fmt "\n", ##__VA_ARGS__)
    #define APP_LOGE(fmt, ...) printf("[E] Application: " fmt "\n", ##__VA_ARGS__)
#endif

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

/**
 * @brief Execute the full framework boot sequence.
 *
 * The boot order is strictly:
 *   BOOTING → (LittleFS mount) → (config load) → WIFI_SETUP →
 *   APP_SETUP → (loadConfig dispatch) → _compactAndLock() →
 *   _reportHeap() → RUNNING
 *
 * @param config Framework-level configuration (hostname, timezone, etc.)
 * @return true on success; false if a fatal error prevents normal operation.
 */
bool Application::begin(const FrameworkConfig& config) {
    // --- Step 1: Boot banner ---
    // Print the firmware version so the first log line is always identifiable.
    APP_LOGI("ESP32 App Foundation v%s — booting", APP_FOUNDATION_VERSION);

    // --- Step 2: Enter BOOTING state ---
    setState(AppState::BOOTING);

    // --- Step 3: Mount LittleFS ---
    // LittleFS is only available on hardware; in NATIVE_BUILD we skip this.
#ifndef NATIVE_BUILD
    if (!LittleFS.begin(true)) {
        // formatOnFail = true: first-boot will auto-format, subsequent failures
        // indicate a hardware fault — abort and return false.
        APP_LOGE("LittleFS mount failed — cannot continue boot");
        return false;
    }
    APP_LOGI("LittleFS mounted successfully");
#else
    APP_LOGI("LittleFS mount skipped (NATIVE_BUILD)");
#endif

    // --- Step 4: Store FrameworkConfig internally ---
    _config = config;
    APP_LOGI("FrameworkConfig stored: hostname=%s timezone=%s",
             _config.hostname, _config.timezone);

    // --- Step 5: Enter WIFI_SETUP state ---
    setState(AppState::WIFI_SETUP);

    // --- Step 6: WiFi bring-up (PR-02 placeholder) ---
    // TODO: wifiManager.begin(_config.hostname);
    APP_LOGI("WiFi setup deferred (PR-02)");

    // --- Step 7: Enter APP_SETUP state ---
    setState(AppState::APP_SETUP);

    // --- Step 8: Dispatch loadConfig() to all registered iConfigurable modules ---
    // Modules receive their stored configuration before tick() begins.
    APP_LOGI("Loading config for %zu configurable module(s)", _configurables.size());
    for (auto* module : _configurables) {
        if (!module->loadConfig()) {
            // A failed config load is non-fatal — the module applies its own defaults.
            APP_LOGW("loadConfig() returned false for a configurable module (defaults applied)");
        }
    }

    // --- Step 9: Core services begin() ---
    _dataManager.begin();
    _eventBus.begin();
    _webServerManager.begin(_webContributors);

    // --- Step 10: Lock all registration vectors ---
    // No further registrations are permitted after this point.
    _compactAndLock();

    // Publish system heap event
#ifndef NATIVE_BUILD
    uint32_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    _eventBus.publish("system.heap", &freeHeap);
#endif

    // --- Step 11: Log heap figures ---
    _reportHeap();

    // --- Step 12: Enter RUNNING state ---
    setState(AppState::RUNNING);

    APP_LOGI("Boot sequence complete — entering RUNNING state");
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
}

/**
 * @brief Log the soft-reset reason and restart the device.
 *
 * esp_restart() is wrapped in NATIVE_BUILD so the GoogleTest suite can call
 * softReset() and verify it completes without crashing the test process.
 */
void Application::softReset() {
    APP_LOGW("Application::softReset() triggered — restarting device");

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
        APP_LOGE("registerTickable() rejected — registration is closed after begin()");
        return false;
    }
    if (module == nullptr) {
        APP_LOGE("registerTickable() rejected — null pointer");
        return false;
    }
    _tickables.push_back(module);
    return true;
}

/**
 * @brief Register a module that loads/persists configuration via LittleFS.
 */
bool Application::registerConfigurable(iConfigurable* module) {
    if (_locked) {
        APP_LOGE("registerConfigurable() rejected — registration is closed after begin()");
        return false;
    }
    if (module == nullptr) {
        APP_LOGE("registerConfigurable() rejected — null pointer");
        return false;
    }
    _configurables.push_back(module);
    return true;
}

/**
 * @brief Set (or replace pre-lock) the single status-reporter adapter.
 *
 * Last-write-wins semantics; only one IStatusReporter is active at a time.
 */
bool Application::registerStatusReporter(IStatusReporter* reporter) {
    if (_locked) {
        APP_LOGE("registerStatusReporter() rejected — registration is closed after begin()");
        return false;
    }
    if (reporter == nullptr) {
        APP_LOGE("registerStatusReporter() rejected — null pointer");
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
        APP_LOGE("registerWebContributor() rejected — registration is closed after begin()");
        return false;
    }
    if (contributor == nullptr) {
        APP_LOGE("registerWebContributor() rejected — null pointer");
        return false;
    }
    _webContributors.push_back(contributor);
    return true;
}

/**
 * @brief Register a background data-fetch worker with the DataManager.
 */
bool Application::registerDataSource(iDataSource* source) {
    if (_locked) {
        APP_LOGE("registerDataSource() rejected — registration is closed after begin()");
        return false;
    }
    if (source == nullptr) {
        APP_LOGE("registerDataSource() rejected — null pointer");
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
    APP_LOGI("State → %s", name);
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
    _configurables.shrink_to_fit();
    _webContributors.shrink_to_fit();
    _dataSources.shrink_to_fit();

    _locked = true;

    APP_LOGI("Registry locked — tickables=%zu configurables=%zu "
             "webContributors=%zu dataSources=%zu statusReporter=%s",
             _tickables.size(),
             _configurables.size(),
             _webContributors.size(),
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

    APP_LOGI("Heap report — free: %" PRIu32 " B  min-free: %" PRIu32 " B",
             freeHeap, minFreeHeap);
#else
    APP_LOGI("Heap report skipped (NATIVE_BUILD)");
#endif
}
