/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/application/Application.cpp
 * Description: Implementation of the Application composition root.
 *              All ESP-IDF API calls (esp_restart, heap APIs, LittleFS.begin)
 *              are wrapped in #ifndef native test stubs so the GoogleTest
 *              native runner can compile and execute the class without hardware
 *              headers. The two-tier vector model is enforced by _compactAndLock().
 *
 *              ConfigManager is now the centralised configuration gateway.
 *              Modules register via registerConfigurable(), which delegates
 *              to _configManager.registerModule() internally. Config is
 *              loaded via _configManager.beginAll().
 *
 *              Core domain services (WifiManager, OTAUpdateManager,
 *              TimeManager, KeyManager) are Application-owned members,
 *              wired into the boot sequence during begin().
 *
 * Exported Classes:
 * - Application: see Application.hpp
 */

#include "Application.hpp"
#include "Redactor.h"
#include "LogPipeline.h"
#include "CompositeSanitizer.h"
#include "StructuralScrubber.h"
#include "events/AppConfiguringPayload.hpp"
#include "events/PreconditionHealthPayload.hpp"


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Core Includes
// ---------------------------------------------------------------------------
#include <inttypes.h>         // PRIu32
#include <Arduino.h>          // Serial, millis(), etc.
#ifndef NATIVE_BUILD
#include <LittleFS.h>
#include "esp_partition.h"
#endif
#include "esp_system.h"       // esp_restart()
#include "esp_heap_caps.h"    // heap_caps_get_free_size(), heap_caps_get_minimum_free_size()
#include <cstdlib>            // For native-build stubs

#include <cstring>                // memset()

#include "platform/LogShim.h"
#include "DeviceCrypto.hpp"

#ifdef WITH_AUDIT
#include "AuditModule.hpp"
static AuditModule& auditModule = AuditModule::instance();
#endif

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
        _bootReporter->setAppTitle(getAppName());
        _statusReporter = _bootReporter;
    } else {
        _statusReporter = &_nullReporter;

    }

    // --- Step 1: Boot banner ---
    // Print the firmware version so the first log line is always identifiable.
    ESP_LOGI(TAG, "ESP32 App Foundation v%s — booting", BUILD_VERSION);

    // --- Step 1b: Install log pipeline ---
    // Intercepts all ESP_LOG* output for global secret redaction.
    // Must be early — all subsequent log output is redacted.
    static CompositeSanitizer s_compositeSanitizer;
    s_compositeSanitizer.add(&Redactor::instance());
#if defined(CONFIG_LOG_MAXIMUM_LEVEL) && CONFIG_LOG_MAXIMUM_LEVEL >= 4
    s_compositeSanitizer.add(&StructuralScrubber::instance());
#endif
    LogPipeline::begin(&s_compositeSanitizer, [](const char* str) -> int { return fputs(str, stdout); });

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
#ifndef NATIVE_BUILD
    const esp_partition_t* config_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "config");

    // Mount storage partition (default)
    if (!LittleFS.begin(true, "/littlefs", 10, "storage")) {
        ESP_LOGE(TAG, "LittleFS mount failed — cannot continue boot");
        _statusReporter->reportError("Filesystem mount failed");
        return false;
    }
    ESP_LOGI(TAG, "LittleFS (storage) mounted successfully");

    // We can allocate another LittleFSFS for the config partition
    static fs::LittleFSFS configFS;

    if (config_part != NULL) {
        if (configFS.begin(true, "/config", 10, "config")) {
            ESP_LOGI(TAG, "LittleFS (config) mounted successfully");
            ConfigRoot::setMountBase("/config");
        } else {
            ESP_LOGE(TAG, "LittleFS (config) mount failed, falling back to /littlefs");
            ConfigRoot::setMountBase("/littlefs");
        }
    } else {
        ESP_LOGI(TAG, "No config partition found, using /littlefs for config");
        ConfigRoot::setMountBase("/littlefs");
    }
#else
    ConfigRoot::setMountBase("test/tmp/config");
#endif
    _statusReporter->reportStatus("Filesystem mounted", 0.10f);

    // --- Step 5: Store FrameworkConfig internally ---
    _config = config;

    // Apply configurable gate re-publish throttle (F5: review feedback)
    if (_config.configuringPublishIntervalMs > 0) {
        _configuringPublishThrottle.setInterval(_config.configuringPublishIntervalMs);
    }

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
    _authManager.begin(&_configManager, &_keyManager);



    // --- Module registration (auto-detects all interfaces per module) ---
    registerModule(&_wifiManager);        // ITickable + IConfigurable + IWebContributor + IMCPContributor
    registerModule(&_authManager);        // IConfigurable + IWebContributor
    registerModule(&_configManager);      // IWebContributor + IMCPContributor
    registerModule(&_keyManager);         // IWebContributor + IMCPContributor
    _preconditionWebContributor.app = this;
    registerModule(&_preconditionWebContributor); // IWebContributor
    
    _dataSourceWebContributor.app = this;
    registerModule(&_dataSourceWebContributor); // IWebContributor

    registerModule(&_webServerManager);   // ITickable + IWebContributor (Status tab)
    if (_enableDefaultAppTab) {
        registerModule(&_appTabContributor);  // IWebContributor
    }

#ifdef WITH_AUDIT
    auditModule.begin(&Redactor::instance());
    registerModule(&auditModule);
#endif

    _wifiPrecondition.registerPreconditions(_preconditionEngine);
    _redactorPrecondition.registerPreconditions(_preconditionEngine);

    // --- Step 8: ConfigManager.beginAll() — load all persisted config ---
    // Iterates the registry, loads each module's namespace file from
    // /mod/<ns>.json, runs version checks and migrations, and calls
    // applyConfig() on each module.
    // ALL persisted config is now available before any service starts.
    _configManager.beginAll();
    _statusReporter->reportStatus("Module configuration loaded", 0.35f);

    // --- Step 9: (Removed — entire begin() runs under BOOT) ---
    _statusReporter->reportStatus("Initializing WiFi...", 0.40f);

    // --- Step 10: WifiManager.begin() ---
    // Now has persisted SSID/hostname from ConfigManager (step 8), or the
    // factory default seeded by resetConfig() (from setDefaultHostname()).
    _wifiManager.begin();
    ESP_LOGI(TAG, "WiFi setup initiated");

    // Sync the AuthManager AP-mode bypass with WifiManager. The bypass applies
    // ONLY to first-boot setup (AP up and no stored credentials). A fallback AP
    // raised during an STA outage on a configured device keeps portal auth
    // enforced (#236). Re-evaluated whenever the AP starts and whenever STA
    // connects (which also tears any fallback AP down), so it is never left
    // true once credentials exist.
    _eventBus.subscribe(WifiEvents::WIFI_AP_STARTED,
        [](const char* /*topic*/, const void* /*payload*/, void* ctx) {
            auto* self = static_cast<Application*>(ctx);
            const bool setup = self->_wifiManager.isSetupMode();
            self->_authManager.setAPMode(setup);
            ESP_LOGI(TAG, "AP started (%s) — AuthManager auth bypass %s",
                     setup ? "setup" : "fallback", setup ? "enabled" : "NOT enabled");
        }, this);
    _eventBus.subscribe(WifiEvents::WIFI_CONNECTED,
        [](const char* /*topic*/, const void* /*payload*/, void* ctx) {
            auto* self = static_cast<Application*>(ctx);
            self->_authManager.setAPMode(self->_wifiManager.isSetupMode());
        }, this);

    _statusReporter->reportStatus("WiFi initialized", 0.55f);

    // --- Step 11: (Removed — TimeManager extracted to explicit composition) ---

    // --- Step 12: DataManager — background data workers ---
    _dataManager.setEventBus(&_eventBus);
    _dataManager.begin();
    for (IDataSource* source : _dataSources) {
        _dataManager.registerSource(source);
    }
    _statusReporter->reportStatus("Data services ready", 0.70f);



    // --- Step 14: (Merged into Step 7 module registration) ---

    // --- Step 15: WebServerManager begin() ---
    _authManager.setAPMode(_wifiManager.isSetupMode());
    _webServerManager.setAppName(getAppName());
    if (_appStylesheet) {
        _webServerManager.setAppStylesheet(_appStylesheet);
    }
    _webServerManager.begin(_webContributors, &_authManager, &_wifiManager);
    _statusReporter->reportStatus("Web portal starting...", 0.90f);



    // --- Step 16: (Merged into Step 7 module registration) ---

    // --- Step 17: Lock all registration vectors ---
    // No further registrations are permitted after this point.
    _compactAndLock();
    if (!_preconditionEngine.resolveEdges()) {
        ESP_LOGE(TAG, "PreconditionEngine: edge resolution failed (unknown name or cycle)");
    }
    _statusReporter->reportStatus("Boot complete", 1.0f);

    // --- Step 17b: Bless firmware (auto-rollback barrier) ---
    // Note: With OTAUpdateManager extracted, consumers must manually bless
    // the firmware by calling otaManager.markAppValid() after app.begin().
    ESP_LOGI(TAG, "✅ Firmware self-test PASSED — (app validation delegated to consumer)");

    // --- Step 18: Publish system heap event + heap report ---
    _systemHeapPayload = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    _eventBus.publish(ApplicationEvents::SYSTEM_HEAP, &_systemHeapPayload);
    _reportHeap();

    // --- Step 19: Clear boot status ---
    _statusReporter->clearStatus();

    // --- Step 20: Check preconditions → RUNNING or CONFIGURING ---
    if (_preconditionEngine.allStartupMet()) {
        setState(AppState::Running);
        _onEnterRunning();
        ESP_LOGI(TAG, "Boot sequence complete — all startup preconditions satisfied, entering RUNNING state");
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
 * (WiFi, display, web portal) and preconditions are ticked each cycle.
 * Once all startup preconditions are satisfied, state advances to RUNNING.
 */
void Application::tick() {
    // Guard: do not tick during boot phases
    if (_state != AppState::Configuring && _state != AppState::Running) return;

    uint32_t currentMillis = millis();
    _preconditionEngine.tick(currentMillis);

    // Extract once — Periodic::due() is consumable (returns true once, resets).
    bool throttleDue = _configuringPublishThrottle.due(currentMillis);

    // --- CONFIGURING: precondition re-publishing + transition check ---
    if (_state == AppState::Configuring) {
        // Precondition re-publishing (500ms throttle)
        if (throttleDue) {
            const PreconditionState* blocking = nullptr;
            for (size_t i = 0; i < _preconditionEngine.getPreconditionCount(); ++i) {
                const auto& s = _preconditionEngine.getStateAt(i);
                if ((s.role == PreconditionRole::Startup || s.role == PreconditionRole::Both) && !s.dampedStatus.met) {
                    blocking = &s;
                    break;
                }
            }

            const char* currentGate = blocking ? blocking->name : nullptr;
            Cause currentCause = blocking ? blocking->dampedStatus.cause : Cause::Met;

            bool changed = (currentGate != _lastPublishedGate) ||
                           (currentCause != _lastPublishedCause);

            if (!changed) {
                // Also re-publish if IP changed (initial event may have 0.0.0.0)
                IPAddress ip = WiFi.localIP();
                char currentIP[16];
                snprintf(currentIP, sizeof(currentIP), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
                if (strcmp(currentIP, _deviceIPBuf) != 0) {
                    changed = true;
                }
            }

            if (changed && blocking != nullptr) {
                _publishConfiguringEvent(false);
            }
        }

        // Check if all startup preconditions now satisfied
        if (_preconditionEngine.allStartupMet()) {
            setState(AppState::Running);
            _onEnterRunning();
            _eventBus.publish(ApplicationEvents::APP_RUNNING);
            ESP_LOGI(TAG, "All startup preconditions satisfied — entering RUNNING state");
        }
    }

#ifndef ESP_PLATFORM
    // In native builds, drive DataManager synchronously (no FreeRTOS worker task).
    _dataManager.processNext();
#endif

    // Dispatch to all registered tickables in registration order
#ifndef NDEBUG
    for (size_t i = 0; i < _tickables.size(); ++i) {
        auto* module = _tickables[i];
        uint32_t startMs = millis();
        module->tick(currentMillis);
        uint32_t elapsedMs = millis() - startMs;
        uint32_t budgetMs = module->getTickBudgetMs();
        if (elapsedMs > budgetMs) {
            ESP_LOGW(TAG, "Module [%" PRIu32 "] (%p) tick budget exceeded: %" PRIu32 " ms (budget: %" PRIu32 " ms)",
                     (uint32_t)i, static_cast<const void*>(module), elapsedMs, budgetMs);
        }
    }
#else
    for (auto* module : _tickables) {
        module->tick(currentMillis);
    }
#endif



    // Periodic heartbeat — free heap + uptime every 30s.
    // Also reports internal DRAM specifically: on PSRAM boards, free_heap
    // (MALLOC_CAP_8BIT) is dominated by PSRAM and can read many MB "free"
    // while internal DRAM is one allocation from exhaustion. dram_min is the
    // low-water mark since boot — it matters more than dram, which is only
    // an instantaneous snapshot and can hide a transient trough.
    if (throttleDue) {
        for (size_t i = 0; i < _preconditionEngine.getPreconditionCount(); ++i) {
            const auto& state = _preconditionEngine.getStateAt(i);
            auto& last = _lastPreconditionStatus[i];
            bool changed = !last.initialized || last.met != state.dampedStatus.met ||
                           last.cause != state.dampedStatus.cause ||
                           last.severity != state.severity ||
                           last.everMet != state.everMet;
            if (changed) {
                last.initialized = true;
                last.met = state.dampedStatus.met;
                last.cause = state.dampedStatus.cause;
                last.severity = state.severity;
                last.everMet = state.everMet;
                _preconditionPayloads[i] = {state.name, state.dampedStatus.cause, state.severity, state.everMet, state.role};
                _eventBus.publish(ApplicationEvents::PRECONDITION_HEALTH, &_preconditionPayloads[i]);
            }
            if (_state == AppState::Running && (state.role == PreconditionRole::Startup || state.role == PreconditionRole::Both) && state.suppressed) {
                ESP_LOGW(TAG, "Precondition '%s' suppressed — regressing to CONFIGURING", state.name);
                setState(AppState::Configuring);
                _publishConfiguringEvent(true);
                _onEnterConfiguring(true);
                break;
            }
        }
    }

    if (_heartbeatPeriodic.due(currentMillis)) {
        uint32_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        uint32_t freeDram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        uint32_t minDram = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        uint32_t uptimeSec = currentMillis / 1000;
        ESP_LOGI(TAG, "♥ uptime=%lus  free_heap=%luB  dram=%luB  dram_min=%luB",
                 (unsigned long)uptimeSec, (unsigned long)freeHeap, (unsigned long)freeDram,
                 (unsigned long)minDram);
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
 * @brief Register a module that participates in config persistence.
 */
bool Application::registerConfigurable(IConfigurable* module) {
    if (_locked) {
        ESP_LOGE(TAG, "registerConfigurable() rejected — registration is closed after begin()");
        return false;
    }
    if (module == nullptr) {
        ESP_LOGE(TAG, "registerConfigurable() rejected — null pointer");
        return false;
    }
    _configManager.registerModule(module);
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



ConfigManager& Application::getConfigManager() {
    return _configManager;
}

WifiManager& Application::getWifiManager() {
    return _wifiManager;
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

    _locked = true;

    ESP_LOGI(TAG, "Registry locked — tickables=%u "
             "webContributors=%u mcpContributors=%u dataSources=%u "
             "statusReporter=%s",
             (unsigned)_tickables.size(),
             (unsigned)_webContributors.size(),
             (unsigned)_mcpContributors.size(),
             (unsigned)_dataSources.size(),
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

/**
 * @brief Publish an "app.configuring" EventBus event for the first
 *        blocking unsatisfied precondition.
 */
void Application::_publishConfiguringEvent(bool isRegression) {
    const PreconditionState* blocking = nullptr;
    for (size_t i = 0; i < _preconditionEngine.getPreconditionCount(); ++i) {
        const auto& s = _preconditionEngine.getStateAt(i);
        if ((s.role == PreconditionRole::Startup || s.role == PreconditionRole::Both) && !s.dampedStatus.met) {
            blocking = &s;
            break;
        }
    }
    if (blocking == nullptr) return;

    _helpTextBuf[0] = '\0';
    if (blocking->describe) {
        blocking->describe(blocking->dampedStatus.cause, _helpTextBuf, sizeof(_helpTextBuf), blocking->context);
    }

    _lastPublishedGate   = blocking->name;
    _lastPublishedCause  = blocking->dampedStatus.cause;

    _configuringPayload.preconditionName     = blocking->name;
    _configuringPayload.helpText     = _helpTextBuf[0] != '\0' ? _helpTextBuf : nullptr;
    _configuringPayload.cause        = blocking->dampedStatus.cause;
    _configuringPayload.severity     = blocking->severity;
    _configuringPayload.isRegression = isRegression;
    _configuringPayload.hostname     = _wifiManager.getHostname();

    if (_wifiManager.getAPMode()) {
        strncpy(_deviceIPBuf, "192.168.4.1", sizeof(_deviceIPBuf));
    } else {
        strncpy(_deviceIPBuf, WiFi.localIP().toString().c_str(), sizeof(_deviceIPBuf));
    }
    _deviceIPBuf[sizeof(_deviceIPBuf) - 1] = '\0';
    _configuringPayload.deviceIP = _deviceIPBuf;

    _eventBus.publish(ApplicationEvents::APP_CONFIGURING, &_configuringPayload);
    ESP_LOGI(TAG, "Precondition unsatisfied: %s (cause=%d, severity=%d)%s",
             blocking->name,
             static_cast<int>(blocking->dampedStatus.cause),
             static_cast<int>(blocking->severity),
             isRegression ? " (regression)" : "");
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
}

/**
 * @brief Entering RUNNING — clear overlays and resume carousel.
 *
 * Clears any modal or base board override so the carousel becomes the
 * visible layer again, then resumes automatic rotation.
 */
void Application::_onEnterRunning() {
}

// ---------------------------------------------------------------------------
// Precondition web routes (WO5)
// ---------------------------------------------------------------------------

namespace {
    const char* roleToString(PreconditionRole role) {
        switch (role) {
            case PreconditionRole::Operational: return "Operational";
            case PreconditionRole::Startup:     return "Startup";
            case PreconditionRole::Both:        return "Both";
            default:                            return "Operational";
        }
    }

    const char* causeToString(Cause cause) {
        switch (cause) {
            case Cause::Met:             return "Met";
            case Cause::NotConfigured:   return "NotConfigured";
            case Cause::Connecting:      return "Connecting";
            case Cause::Rejected:        return "Rejected";
            case Cause::Unreachable:     return "Unreachable";
            case Cause::RateLimited:     return "RateLimited";
            case Cause::Implausible:     return "Implausible";
            case Cause::DependencyUnmet: return "DependencyUnmet";
            case Cause::FallbackEngaged: return "FallbackEngaged";
            case Cause::Unavailable:     return "Unavailable";
            case Cause::Indeterminate:   return "Indeterminate";
            case Cause::InternalError:   return "InternalError";
            default:                     return "NotConfigured";
        }
    }

    const char* severityToString(Severity severity) {
        switch (severity) {
            case Severity::Transient:  return "Transient";
            case Severity::NeedsUser:  return "NeedsUser";
            case Severity::Blocked:    return "Blocked";
            case Severity::Degraded:   return "Degraded";
            case Severity::Inactive:   return "Inactive";
            case Severity::Persistent: return "Persistent";
            default:                   return "Transient";
        }
    }

    const char* breakerToString(BreakerState breaker) {
        switch (breaker) {
            case BreakerState::Closed:   return "Closed";
            case BreakerState::Open:     return "Open";
            case BreakerState::HalfOpen: return "HalfOpen";
            default:                     return "Closed";
        }
    }
}

void Application::PreconditionWebContributor::registerRoutes(PsychicHttpServer& server) {
    if (!app) return;

    server.on("/api/preconditions", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();

        PreconditionEngine& engine = app->getPreconditionEngine();
        size_t count = engine.getPreconditionCount();
        for (size_t i = 0; i < count; ++i) {
            const auto& state = engine.getStateAt(i);
            JsonObject obj = arr.add<JsonObject>();
            obj["name"] = state.name;
            obj["role"] = roleToString(state.role);
            obj["met"] = state.dampedStatus.met;
            obj["cause"] = causeToString(state.dampedStatus.cause);
            obj["severity"] = severityToString(state.severity);
            obj["everMet"] = state.everMet;
            obj["breaker"] = breakerToString(state.breaker);
            obj["suppressed"] = state.suppressed;
        }

        std::string out;
        serializeJson(doc, out);
        return response->send(200, "application/json", out.c_str());
    });

    server.on("/api/preconditions/retry", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        if (request->contentLength() > 1024) {
            response->send(413, "application/json", "{\"error\":\"Payload too large\"}");
            return ESP_FAIL;
        }

        const char* body = request->bodyCStr();
        std::string name;
        if (body && strlen(body) > 0) {
            JsonDocument doc;
            if (deserializeJson(doc, body) == DeserializationError::Ok) {
                if (doc["name"].is<const char*>()) {
                    name = doc["name"].as<const char*>();
                }
            }
        }
        if (name.empty() && request->hasParam("name")) {
            name = request->getParam("name")->value().c_str();
        }
        if (name.empty()) {
            response->send(400, "application/json", "{\"error\":\"Precondition name required\"}");
            return ESP_FAIL;
        }

        app->getPreconditionEngine().requestRetry(name.c_str());
        return response->send(200, "application/json", "{\"status\":\"ok\"}");
    });
}



void Application::DataSourceWebContributor::registerRoutes(PsychicHttpServer& server) {
    if (!app) return;

    server.on("/api/data/sources", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();

        DataManager& dm = app->getDataManager();
        size_t count = dm.getSourceCount();
        for (size_t i = 0; i < count; ++i) {
            SourceHealth sh;
            if (dm.getSourceHealthAt(i, sh)) {
                JsonObject obj = arr.add<JsonObject>();
                obj["name"] = sh.name ? sh.name : "";
                obj["state"] = sourceStateToString(sh.state);
                obj["lastStatus"] = static_cast<int>(sh.lastStatus);
                obj["consecutiveFailures"] = sh.consecutiveFailures;
                obj["everSucceeded"] = sh.everSucceeded;
                obj["currentBackoffMs"] = sh.currentBackoffMs;
            }
        }

        std::string out;
        serializeJson(doc, out);
        return response->send(200, "application/json", out.c_str());
    });

    server.on("/api/data/sources/reset", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        if (request->contentLength() > 1024) {
            response->send(413, "application/json", "{\"error\":\"Payload too large\"}");
            return ESP_FAIL;
        }

        const char* body = request->bodyCStr();
        std::string name;
        if (body && strlen(body) > 0) {
            JsonDocument doc;
            if (deserializeJson(doc, body) == DeserializationError::Ok) {
                if (doc["name"].is<const char*>()) {
                    name = doc["name"].as<const char*>();
                }
            }
        }
        if (name.empty() && request->hasParam("name")) {
            name = request->getParam("name")->value().c_str();
        }
        if (name.empty()) {
            response->send(400, "application/json", "{\"error\":\"Source name required\"}");
            return ESP_FAIL;
        }

        DataManager& dm = app->getDataManager();
        IDataSource* target = nullptr;
        
        // Iterate Application's _dataSources to find the IDataSource* by name.
        for (IDataSource* src : app->_dataSources) {
            if (src && src->getSourceName() && strcmp(src->getSourceName(), name.c_str()) == 0) {
                target = src;
                break;
            }
        }
        
        if (target) {
            dm.resetSourceHealth(target);
            return response->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            return response->send(404, "application/json", "{\"error\":\"Source not found\"}");
        }
    });
}
