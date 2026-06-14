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
 *              Core domain services (WifiManager, OTAUpdateManager,
 *              TimeManager, KeyManager) are Application-owned members,
 *              wired into the boot sequence during begin().
 *
 * Exported Classes:
 * - Application: see Application.hpp
 */

#include "Application.hpp"
#include "../../include/events/AppConfiguringPayload.hpp"
#include "../OTAUpdateManager/OTAUpdateManager.hpp"

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
    #include <cstdlib>            // For native-build stubs
    // millis() stub — returns 0 in native builds; sufficient for compilation
    // and basic test execution. Regression logic won't trigger (always time=0).
    static unsigned long millis() { return 0; }
#endif

#include <cstring>                // memset()

#include "platform/LogShim.h"

static const char* TAG = "Application";

// Destructor — clean up heap-allocated OTAUpdateManager
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
 *   KeyManager → ConfigManager (register + beginAll) → APP_SETUP →
 *   WiFi → Time → Data → OTA → Web → Lock → Gate check →
 *   CONFIGURING (if unsatisfied) or RUNNING (if all gates pass)
 *
 * @param config Framework-level configuration (OTA settings, etc.)
 * @return true on success; false if a fatal error prevents normal operation.
 */
bool Application::begin(const FrameworkConfig& config) {
    // --- Phase 1: Bootstrap display hardware (zero framework deps) ---
    // SPI + U8G2 init, boot splash rendered immediately.
    // Must happen before any reportStatus() calls so the boot splash is visible.
#ifndef NATIVE_BUILD
    _display.bootstrapHardware();
    _statusReporter = &_display;  // Route all boot progress to the display
#endif

    // --- Step 1: Boot banner ---
    // Print the firmware version so the first log line is always identifiable.
    ESP_LOGI(TAG, "ESP32 App Foundation v%s — booting", BUILD_VERSION);

    // --- Step 2: Enter BOOT state ---
    setState(AppState::Boot);
    _statusReporter->reportStatus("Initializing...", 0.0f);

    // --- Step 3: EventBus.begin() ---
    // FIRST service to init — zero dependencies, pure in-memory.
    // Starting early enables all subsequent services to publish boot-time events.
    _eventBus.begin();
    _statusReporter->reportStatus("Event system ready", 0.05f);

    // --- Step 4: Mount LittleFS ---
    // Foundation for all persistence. Only available on hardware;
    // in NATIVE_BUILD we skip this.
#ifndef NATIVE_BUILD
    if (!LittleFS.begin(true, "/littlefs", 10, "littlefs")) {
        // formatOnFail = true: first-boot will auto-format, subsequent failures
        // indicate a hardware fault — abort and return false.
        ESP_LOGE(TAG, "LittleFS mount failed — cannot continue boot");
        _statusReporter->reportError("Filesystem mount failed");
        return false;
    }
    ESP_LOGI(TAG, "LittleFS mounted successfully");
    _statusReporter->reportStatus("Filesystem mounted", 0.10f);
#else
    ESP_LOGI(TAG, "LittleFS mount skipped (NATIVE_BUILD)");
#endif

    // --- Step 5: Store FrameworkConfig internally ---
    _config = config;
    ESP_LOGI(TAG, "FrameworkConfig stored");
    _statusReporter->reportStatus("Configuration defaults loaded", 0.15f);

    // --- Step 6: DeviceCrypto + KeyManager ---
    // DeviceCrypto needs filesystem for key material; KeyManager needs DeviceCrypto.
    _deviceCrypto.begin();
    _keyManager.begin(&_deviceCrypto);
    _statusReporter->reportStatus("Security services ready", 0.20f);

    // --- Step 7: ConfigManager — register all framework IConfigurable modules ---
    // WifiManager: inject ConfigManager and KeyManager dependencies, then register.
    // TimeManager: inject ConfigManager dependency, then register.
    // MCPServer: registered directly.
    // AuthManager: inject ConfigManager and KeyManager, then register. Must be
    //   registered before beginAll() so that persisted auth flags are loaded on boot.
    // OTAUpdateManager: heap-allocated here (header is forward-declared to avoid
    //   LDF circular-include issues). Registered with ConfigManager BEFORE beginAll()
    //   so that OTA channel/autoCheck config is loaded from /mod/ota.json on boot.
    _wifiManager.setConfigManager(&_configManager);
    _wifiManager.setKeyManager(&_keyManager);
    _configManager.registerModule(&_wifiManager);
    _configManager.registerModule(&_mcpServer);
    _timeManager.setConfigManager(&_configManager);
    _configManager.registerModule(&_timeManager);
    _authManager.begin(&_configManager, &_keyManager);
    _configManager.registerModule(&_authManager);
    _otaManager = new OTAUpdateManager();
    _configManager.registerModule(_otaManager);
#ifndef NATIVE_BUILD
    _configManager.registerModule(&_display);
#endif

    // --- Step 8: ConfigManager.beginAll() — load all persisted config ---
    // Iterates the registry, loads each module's namespace file from
    // /mod/<ns>.json, runs version checks and migrations, and calls
    // applyConfig() on each module.
    // ALL persisted config is now available before any service starts.
    _configManager.beginAll();
    _statusReporter->reportStatus("Module configuration loaded", 0.35f);

    // --- Step 9: (Removed — entire begin() runs under BOOT) ---
    _statusReporter->reportStatus("Connecting to WiFi...", 0.40f);

    // --- Step 10: WifiManager.begin() ---
    // Now has persisted SSID/hostname from ConfigManager (step 8), or the
    // factory default seeded by resetConfig() (from setDefaultHostname()).
    _wifiManager.begin();
    ESP_LOGI(TAG, "WiFi setup initiated");
    _statusReporter->reportStatus("WiFi initialized", 0.55f);

    // --- Step 11: TimeManager — after WiFi, needs network for NTP ---
    // Uses the persisted timezone from ConfigManager, or the factory default
    // ("GMT0") seeded by resetConfig().
    _timeManager.begin();
    _statusReporter->reportStatus("Time synchronization started", 0.65f);

    // --- Step 12: DataManager — background data workers ---
    _dataManager.begin();
    _statusReporter->reportStatus("Data services ready", 0.70f);

    // --- Step 13: OTA — wire Application dependency (allocated in Step 7) ---
    _otaManager->begin(*this);
    _statusReporter->reportStatus("Update service ready", 0.80f);

    // --- Step 14: Register core web contributors and tickables ---
    _webContributors.push_back(&_authManager);   // Security tab — registered first (order: 5)
    _webContributors.push_back(&_wifiManager);
    _webContributors.push_back(&_timeManager);
    _webContributors.push_back(&_configManager);
    _webContributors.push_back(&_keyManager);
    _webContributors.push_back(_otaManager);
    _webContributors.push_back(&_mcpServer);
    _webContributors.push_back(&_appTabContributor);
#ifndef NATIVE_BUILD
    _webContributors.push_back(&_display);
    _webContributors.push_back(_display.settingsTabStub());
    _webContributors.push_back(_display.screenshotTabStub());
    _mcpContributors.push_back(&_display);
#endif

    _tickables.push_back(&_wifiManager);
    _tickables.push_back(&_timeManager);
#ifndef NATIVE_BUILD
    _tickables.push_back(&_display);
#endif
    _tickables.push_back(_otaManager); // Boot auto-check + daily scheduled check


    // --- Step 15: WebServerManager + MCPServer begin() ---
    _webServerManager.begin(_webContributors);

    // MCP server: pass the ConfigManager reference for config operations,
    // plus TimeManager (built-in time tools) and WifiManager (status hostname).
    _mcpServer.begin(_mcpContributors, &_configManager, &_timeManager, &_wifiManager, &_authManager);
    _statusReporter->reportStatus("Web portal starting...", 0.90f);

    // --- Phase 2: Complete display integration ---
    // EventBus subscriptions, config persistence, carousel board registration,
    // MCP framebuffer binding. Exits bootstrap mode — tick() handles rendering.
#ifndef NATIVE_BUILD
    _display.begin(&_eventBus);
    _display.setConfigManager(&_configManager);
    _diagnosticsBoard.configure(&_wifiManager);
    _display.carousel.add(&_diagnosticsBoard);
    _mcpServer.setDisplayInfo(_display.getDisplayInfo());
#endif

    // --- Step 16: Register built-in WiFi config gate ---
    _configGates.push_back(&_wifiGate);

    // --- Step 17: Lock all registration vectors ---
    // No further registrations are permitted after this point.
    _compactAndLock();
    _statusReporter->reportStatus("Boot complete", 1.0f);

    // --- Step 17b: Bless firmware (auto-rollback barrier) ---
    // If the firmware crashes before reaching this point, the bootloader
    // automatically reverts to the previous partition. This barrier covers:
    // display bootstrap, EventBus, LittleFS, Crypto, Config, WiFi, Time,
    // Data, OTA, Web, display integration, and registry lock.
#ifndef NATIVE_BUILD
    _otaManager->markAppValid();
    ESP_LOGI(TAG, "✅ Firmware self-test PASSED — partition blessed");
#endif

    // --- Step 18: Publish system heap event + heap report ---
#ifndef NATIVE_BUILD
    uint32_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    _eventBus.publish("system.heap", &freeHeap);
#endif
    _reportHeap();

    // --- Step 19: Clear boot status ---
    _statusReporter->clearStatus();

    // --- Step 20: Check config gates → RUNNING or CONFIGURING ---
    if (_allGatesSatisfied()) {
        setState(AppState::Running);
        _dataManager.setAppRunning(true);
        _onEnterRunning();
        ESP_LOGI(TAG, "Boot sequence complete — all gates satisfied, entering RUNNING state");
    } else {
        setState(AppState::Configuring);
        _publishConfiguringEvent();
        _onEnterConfiguring(false);
        ESP_LOGI(TAG, "Boot sequence complete — awaiting configuration");
    }

    return true;
}

/**
 * @brief Dispatch the periodic update tick to all registered ITickable modules.
 *
 * The guard ensures tick() is a no-op until AppState::Configuring or
 * AppState::Running. During CONFIGURING, all modules still receive ticks
 * (WiFi, display, web portal) but config gates are polled each cycle.
 * Once all gates are satisfied, state advances to RUNNING.
 */
void Application::tick() {
    // Guard: do not tick during boot phases
    if (_state != AppState::Configuring && _state != AppState::Running) return;

    // --- RUNNING: throttled gate regression check ---
    if (_state == AppState::Running) {
        unsigned long now = millis();
        if ((now - _lastGateCheckMs) >= GATE_CHECK_INTERVAL_MS) {
            _lastGateCheckMs = now;
            for (uint8_t i = 0; i < _configGates.size(); i++) {
                if (!_configGates[i]->isSatisfied()) {
                    uint32_t tolerance = _configGates[i]->getRegressionToleranceMs();
                    if (tolerance == UINT32_MAX) continue; // App handles recovery

                    if (_gateFailedSinceMs[i] == 0) {
                        _gateFailedSinceMs[i] = now; // Start timer
                    } else if ((now - _gateFailedSinceMs[i]) >= tolerance) {
                        ESP_LOGW(TAG, "Gate '%s' unsatisfied for %lums — regressing to CONFIGURING",
                                 _configGates[i]->getGateName(), (now - _gateFailedSinceMs[i]));
                        setState(AppState::Configuring);
                        _dataManager.setAppRunning(false);
                        _publishConfiguringEvent(true); // isRegression = true
                        _onEnterConfiguring(true);
                        memset(_gateFailedSinceMs, 0, sizeof(_gateFailedSinceMs));
                        break;
                    }
                } else {
                    _gateFailedSinceMs[i] = 0; // Gate re-satisfied, reset timer
                }
            }
        }
    }

    // --- CONFIGURING: poll config gates for transition ---
    if (_state == AppState::Configuring) {
        if (_allGatesSatisfied()) {
            setState(AppState::Running);
            _dataManager.setAppRunning(true);
            _onEnterRunning();
            _eventBus.publish("app.running");
            ESP_LOGI(TAG, "All config gates satisfied — entering RUNNING state");
        }
#ifndef NATIVE_BUILD
        // --- WiFi Wizard modal: show once when AP mode becomes active ---
        // _onEnterConfiguring fires before the WiFi state machine has run, so
        // AP mode detection must happen here in the tick loop instead.
        else if (_wifiManager.getAPMode() && !_wifiWizardShown) {
            const char* apSsid = _wifiManager.getSSID();
            // In AP mode the SSID is the hostname (portal name), not the stored STA SSID
            const char* hostname = _wifiManager.getHostname();
            _display.boards.wifiWizard.configure(
                (hostname && strlen(hostname) > 0) ? hostname : apSsid,
                "192.168.4.1"
            );
            _display.showModal(&_display.boards.wifiWizard);
            _wifiWizardShown = true;
            ESP_LOGI(TAG, "WiFi wizard modal: AP active, showing connection guide");
        }
#endif
    }

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
bool Application::registerMcpContributor(IMCPContributor* contributor) {
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
bool Application::registerDataSource(IDataSource* source) {
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

/**
 * @brief Register a configuration prerequisite gate.
 */
bool Application::registerConfigGate(IConfigGate* gate) {
    if (_locked) {
        ESP_LOGE(TAG, "registerConfigGate() rejected — registration is closed after begin()");
        return false;
    }
    if (gate == nullptr) {
        ESP_LOGE(TAG, "registerConfigGate() rejected — null pointer");
        return false;
    }
    _configGates.push_back(gate);
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

MCPServer& Application::getMCPServer() {
    return _mcpServer;
}

ConfigManager& Application::getConfigManager() {
    return _configManager;
}

WifiManager& Application::getWifiManager() {
    return _wifiManager;
}

OTAUpdateManager& Application::getOtaManager() {
    return *_otaManager;
}

TimeManager& Application::getTimeManager() {
    return _timeManager;
}

TimeConfigGate& Application::getTimeGate() {
    return _timeGate;
}

KeyManager& Application::getKeyManager() {
    return _keyManager;
}

AuthManager& Application::getAuthManager() {
    return _authManager;
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
        case AppState::Boot:        name = "BOOT";        break;
        case AppState::Configuring: name = "CONFIGURING"; break;
        case AppState::Running:     name = "RUNNING";     break;
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
    _configGates.shrink_to_fit();

    _locked = true;

    ESP_LOGI(TAG, "Registry locked — tickables=%u "
             "webContributors=%u mcpContributors=%u dataSources=%u "
             "configGates=%u statusReporter=%s",
             (unsigned)_tickables.size(),
             (unsigned)_webContributors.size(),
             (unsigned)_mcpContributors.size(),
             (unsigned)_dataSources.size(),
             (unsigned)_configGates.size(),
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

// ---------------------------------------------------------------------------
// Config Gate Helpers
// ---------------------------------------------------------------------------

/**
 * @brief Check whether all registered config gates are satisfied.
 *
 * Returns true if no gates are registered (zero-gate fast path) or if
 * every gate's isSatisfied() returns true.
 */
bool Application::_allGatesSatisfied() const {
    for (const auto* gate : _configGates) {
        if (!gate->isSatisfied()) return false;
    }
    return true;
}

/**
 * @brief Publish an "app.configuring" EventBus event for the first
 *        **actionable** unsatisfied gate.
 *
 * A gate is actionable when it is unsatisfied AND all of its declared
 * dependencies (getDependencies()) are satisfied. Gates whose dependencies
 * are not yet met are silently skipped, ensuring the user sees only the
 * gate they can act on next.
 */
void Application::_publishConfiguringEvent(bool isRegression) {
    for (const auto* gate : _configGates) {
        if (gate->isSatisfied()) continue;

        // Check if all dependencies are satisfied
        uint8_t depCount = 0;
        const char* const* deps = gate->getDependencies(depCount);
        bool depsOk = true;
        for (uint8_t d = 0; d < depCount; d++) {
            // Find the dependency gate by name and check if satisfied
            bool found = false;
            for (const auto* other : _configGates) {
                if (strcmp(other->getGateName(), deps[d]) == 0) {
                    if (!other->isSatisfied()) depsOk = false;
                    found = true;
                    break;
                }
            }
            if (!found || !depsOk) { depsOk = false; break; }
        }

        if (!depsOk) continue; // Skip — deps not met

        // This gate is actionable
        AppConfiguringPayload payload;
        payload.gateName = gate->getGateName();
        payload.helpText = gate->getHelpText();
        payload.isRegression = isRegression;
        _eventBus.publish("app.configuring", &payload);
        ESP_LOGI(TAG, "Config gate unsatisfied: %s%s", gate->getGateName(),
                 isRegression ? " (regression)" : "");
        return;
    }
}

// ---------------------------------------------------------------------------
// Display-driven state transition helpers (WO5)
// ---------------------------------------------------------------------------

/**
 * @brief Entering CONFIGURING — suspend carousel and show gate-status board.
 *
 * This is the Application-layer orchestration described in §7.8.5 of the
 * design spec. The display module is a pure service — it doesn't know
 * about AppState. Application drives it.
 */
void Application::_onEnterConfiguring(bool /*isRegression*/) {
#ifndef NATIVE_BUILD
    _display.carousel.suspend();
    _display.showBase(&_display.boards.configuringStatus);
#endif
}

/**
 * @brief Entering RUNNING — clear overlays and resume carousel.
 *
 * Clears any modal or base board override so the carousel becomes the
 * visible layer again, then resumes automatic rotation.
 */
void Application::_onEnterRunning() {
#ifndef NATIVE_BUILD
    _display.clearModal();
    _display.showBase(nullptr);  // clear base → carousel becomes visible
    _display.carousel.resume();
    _wifiWizardShown = false;    // reset so wizard can show again if device re-enters CONFIGURING
#endif
}

// ---------------------------------------------------------------------------
// Display accessor
// ---------------------------------------------------------------------------

#ifndef NATIVE_BUILD
/**
 * @brief Get the application's display module instance.
 */
U8G2DisplayModule& Application::getDisplay() {
    return _display;
}
#endif
