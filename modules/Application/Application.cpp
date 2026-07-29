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
 *              are wrapped in #ifndef native test stubs so the GoogleTest
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
// ---------------------------------------------------------------------------
// Core Includes
// ---------------------------------------------------------------------------
#include <inttypes.h>         // PRIu32
#include <Arduino.h>          // Serial, millis(), etc.
#include <LittleFS.h>         // LittleFS.begin()
#include "esp_system.h"       // esp_restart()
#include "esp_heap_caps.h"    // heap_caps_get_free_size(), heap_caps_get_minimum_free_size()
#include <cstdlib>            // For native-build stubs

#include <cstring>                // memset()

#include "platform/LogShim.h"

static constexpr const char* TAG = "Application";

// --- PSRAM configuration guard (Issue #36) ---
// BOARD_HAS_PSRAM is an Arduino-layer hint set in platformio.ini build_flags.
// It does NOT enable the ESP-IDF SPIRAM driver — CONFIG_SPIRAM=y in sdkconfig
// is required for that. Catch the misconfiguration at compile time.
#if defined(BOARD_HAS_PSRAM) && !defined(CONFIG_SPIRAM)
  #error "BOARD_HAS_PSRAM is defined but CONFIG_SPIRAM is not enabled in sdkconfig. " \
         "PSRAM will be silently disabled. Add CONFIG_SPIRAM=y to your sdkconfig.defaults."
#endif

// Destructor — clean up heap-allocated modules
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
    if (_bootReporter) {
        _bootReporter->beginEarly();
        _statusReporter = _bootReporter;
    } else {
        _statusReporter = &_nullReporter;

    }

    // --- Step 1: Boot banner ---
    // Print the firmware version so the first log line is always identifiable.
    ESP_LOGI(TAG, "ESP32 App Foundation v%s — booting", BUILD_VERSION);

#if defined(BOARD_HAS_PSRAM)
    // Runtime PSRAM initialization check (Issue #36)
    // Catches the case where CONFIG_SPIRAM is set but PSRAM physically
    // failed to initialize (e.g., hardware fault, timing issue).
    if (heap_caps_get_total_size(MALLOC_CAP_SPIRAM) == 0) {
        ESP_LOGE(TAG, "PSRAM expected (BOARD_HAS_PSRAM) but none detected! "
                      "Check CONFIG_SPIRAM=y and PSRAM hardware.");
    }
#endif

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
    // in native builds we skip this.
    if (!LittleFS.begin(true, "/littlefs", 10, "storage")) {
        // formatOnFail = true: first-boot will auto-format, subsequent failures
        // indicate a hardware fault — abort and return false.
        ESP_LOGE(TAG, "LittleFS mount failed — cannot continue boot");
        _statusReporter->reportError("Filesystem mount failed");
        return false;
    }
    ESP_LOGI(TAG, "LittleFS mounted successfully");
    _statusReporter->reportStatus("Filesystem mounted", 0.10f);

    // --- Step 5: Store FrameworkConfig internally ---
    _config = config;
    ESP_LOGI(TAG, "FrameworkConfig stored");
    _statusReporter->reportStatus("Configuration defaults loaded", 0.15f);

    // --- Step 6: DeviceCrypto + KeyManager ---
    // DeviceCrypto needs filesystem for key material; KeyManager needs DeviceCrypto.
    _deviceCrypto.begin();
    _keyManager.begin(&_deviceCrypto);
    _statusReporter->reportStatus("Security services ready", 0.20f);

    // --- Step 7: Inject dependencies and register framework modules ---
    // Dependency injection wires up core services before registration or boot.
    _wifiManager.setConfigManager(&_configManager);
    _wifiManager.setKeyManager(&_keyManager);
    _wifiManager.setEventBus(&_eventBus);
    _timeManager.setConfigManager(&_configManager);
    _authManager.begin(&_configManager, &_keyManager);

    _otaManager = new (std::nothrow) OTAUpdateManager();
    if (_otaManager == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate OTAUpdateManager");
        return false;
    }

    // --- Module registration (auto-detects all interfaces per module) ---
    registerModule(&_wifiManager);        // ITickable + IConfigurable + IWebContributor + IMCPContributor
    registerModule(&_timeManager);        // ITickable + IConfigurable + IWebContributor
    registerModule(&_authManager);        // IConfigurable + IWebContributor
    registerModule(_authManager.passwordTabStub());  // IWebContributor (sub-pill)
    registerModule(_authManager.tlsTabStub());       // IWebContributor (sub-pill)
    registerModule(&_configManager);      // IWebContributor + IMCPContributor
    registerModule(&_keyManager);         // IWebContributor
    registerModule(&_mcpServer);          // IConfigurable + IWebContributor
    registerModule(_otaManager);          // ITickable + IConfigurable + IWebContributor + IMCPContributor
    registerModule(&_webServerManager);   // ITickable
    if (_enableDefaultAppTab) {
        registerModule(&_appTabContributor);  // IWebContributor
    }



    registerModule(&_wifiGate);           // IConfigGate

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
    for (IDataSource* source : _dataSources) {
        _dataManager.registerSource(source);
    }
    _statusReporter->reportStatus("Data services ready", 0.70f);

    // --- Step 13: OTA — wire Application dependency (allocated in Step 7) ---
    _otaManager->begin(*this);
    _statusReporter->reportStatus("Update service ready", 0.80f);

    // --- Step 14: (Merged into Step 7 module registration) ---

    // --- Step 15: WebServerManager + MCPServer begin() ---
    _authManager.setAPMode(_wifiManager.getAPMode());
    _webServerManager.setAppName(getAppName());
    _webServerManager.begin(_webContributors, &_authManager, &_wifiManager);

    // MCP server: pass the ConfigManager reference for config operations,
    // plus TimeManager (built-in time tools) and WifiManager (status hostname).
    _mcpServer.begin(_mcpContributors, &_configManager, &_timeManager, &_wifiManager, &_authManager);
    _statusReporter->reportStatus("Web portal starting...", 0.90f);



    // --- Step 16: (Merged into Step 7 module registration) ---

    // --- Step 17: Lock all registration vectors ---
    // No further registrations are permitted after this point.
    _compactAndLock();
    _statusReporter->reportStatus("Boot complete", 1.0f);

    // --- Step 17b: Bless firmware (auto-rollback barrier) ---
    // If the firmware crashes before reaching this point, the bootloader
    // automatically reverts to the previous partition. This barrier covers:
    // display bootstrap, EventBus, LittleFS, Crypto, Config, WiFi, Time,
    // Data, OTA, Web, display integration, and registry lock.
    _otaManager->markAppValid();
    ESP_LOGI(TAG, "✅ Firmware self-test PASSED — partition blessed");

    // --- Step 18: Publish system heap event + heap report ---
    _systemHeapPayload = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    _eventBus.publish(ApplicationEvents::SYSTEM_HEAP, &_systemHeapPayload);
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

    uint32_t currentMillis = millis();

    // --- RUNNING: throttled gate regression check ---
    if (_state == AppState::Running) {
        unsigned long now = currentMillis;
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
            _eventBus.publish(ApplicationEvents::APP_RUNNING);
            ESP_LOGI(TAG, "All config gates satisfied — entering RUNNING state");
        }

    }

    // In native builds, drive DataManager synchronously for tests
    _dataManager.processNext();

    // Dispatch to all registered tickables in registration order
    for (auto* module : _tickables) {
        module->tick(currentMillis);
    }

    // MCP server: poll Serial for incoming JSON-RPC requests
    _mcpServer.tick();

    // Periodic heartbeat — free heap + uptime every 30s
    if (currentMillis - _lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
        _lastHeartbeatMs = currentMillis;
        uint32_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        uint32_t uptimeSec = currentMillis / 1000;
        ESP_LOGI(TAG, "♥ uptime=%lus  free_heap=%luB", (unsigned long)uptimeSec, (unsigned long)freeHeap);
    }
}

/**
 * @brief Log the soft-reset reason and restart the device.
 *
 * esp_restart() is wrapped in native builds so the GoogleTest suite can call
 * softReset() and verify it completes without crashing the test process.
 */
void Application::softReset() {
    ESP_LOGW(TAG, "Application::softReset() triggered — restarting device");

    esp_restart(); // Hardware restart — this call never returns
    // In native builds: function returns normally (no-op for test verification)
}

// ---------------------------------------------------------------------------
// Module Registration (Tier 1 — pre-lock)
// All register* methods share the same null-check + locked-check guard.
// ---------------------------------------------------------------------------

/**
 * @brief Register a boot reporter for early hardware init.
 */
bool Application::registerBootReporter(IBootReporter* reporter) {
    if (_locked) {
        ESP_LOGE(TAG, "registerBootReporter() rejected — registration is closed after begin()");
        return false;
    }
    if (reporter == nullptr) {
        ESP_LOGE(TAG, "registerBootReporter() rejected — null pointer");
        return false;
    }
    _bootReporter = reporter;
    return true;
}

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
 * Using native test stubs to keep the native test build self-contained.
 */
void Application::_reportHeap() {
    // heap_caps_get_free_size returns available bytes in a specific memory region.
    uint32_t freeHeap    = heap_caps_get_free_size(MALLOC_CAP_8BIT);      // Current free bytes
    uint32_t minFreeHeap = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT); // Historical low-watermark

    ESP_LOGI(TAG, "Heap report — free: %" PRIu32 " B  min-free: %" PRIu32 " B",
             freeHeap, minFreeHeap);
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
        _configuringPayload.gateName = gate->getGateName();
        _configuringPayload.helpText = gate->getHelpText();
        _configuringPayload.isRegression = isRegression;
        _eventBus.publish(ApplicationEvents::APP_CONFIGURING, &_configuringPayload);
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
#ifdef HAS_DISPLAY
    // _display.carousel.suspend();
    // _display.showBase(&_display.boards.configuringStatus);
#endif
}

/**
 * @brief Entering RUNNING — clear overlays and resume carousel.
 *
 * Clears any modal or base board override so the carousel becomes the
 * visible layer again, then resumes automatic rotation.
 */
void Application::_onEnterRunning() {
#ifdef HAS_DISPLAY
    // _display.clearModal();
    // _display.showBase(nullptr);  // clear base → carousel becomes visible
    // _display.carousel.resume();
    // _wifiWizardShown = false;    // reset so wizard can show again if device re-enters CONFIGURING
#endif
}

// ---------------------------------------------------------------------------
// Display accessor
// ---------------------------------------------------------------------------


