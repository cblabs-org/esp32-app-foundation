/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/application/Application.hpp
 * Description: Composition root for the ESP32 App Foundation framework.
 *              Owns the module registry (two-tier vector model), the
 *              BOOT → CONFIGURING ↔ RUNNING lifecycle, the
 *              main-loop tick dispatch, the display subsystem
 *              (U8G2DisplayModule + DiagnosticsBoard), and the soft-reset
 *              escape hatch.
 *              A single static instance is created by main.cpp (WO4); the
 *              class is NOT a singleton — there is no getInstance().
 *
 *              ConfigManager is owned by Application and serves as the
 *              centralised gateway for all module configuration I/O.
 *
 *              Core domain services (WifiManager, OTAUpdateManager,
 *              KeyManager, AuthManager) are also Application-owned
 *              members, wired into the boot sequence and registered with ConfigManager
 *              and the tick/web contributor vectors where applicable.
 *
 * Exported Classes:
 * - Application: Framework composition root and module registry
 *   - begin():                   Run the full boot sequence; returns false on fatal error
 *   - tick():                    Dispatch to all registered ITickable modules (RUNNING guard)
 *   - softReset():               Log reason and trigger esp_restart() 
 *   - registerTickable():        Add an ITickable module to the tick dispatch vector
 *   - registerConfigurable():    Add an IConfigurable module to the config persistence system
 *   - registerStatusReporter():  Set (or replace) the single IStatusReporter adapter
 *   - registerWebContributor():  Add an IWebContributor module to the route-registration vector
 *   - registerMcpContributor():  Add an IMCPContributor module to the MCP tools vector
 *   - registerDataSource():      Add an IDataSource worker to the DataManager vector
 *   - getConfigManager():        Access the ConfigManager gateway
 *   - getWifiManager():          Access the WifiManager domain service
 *   - getKeyManager():           Access the KeyManager domain service
 *   - getAuthManager():          Access the AuthManager security service
 *   - getState():                Query the current AppState lifecycle phase
 *   - getMcpContributors():      Access registered MCP contributor vector
 *   - setAppName():              Set the application name displayed in portal title
 *   - getAppName():              Get the application name
 *   - setManufacturer():         Set the manufacturer name for device identity
 *   - getManufacturer():         Get the manufacturer name (defaults to "CB Labs")
 */

#pragma once
#include <vector>
#include <type_traits>
#include "platform/LogShim.h"
#include "types/AppState.hpp"
#include "types/FrameworkConfig.hpp"
#include "utils/Periodic.hpp"
#include "interfaces/ITickable.hpp"
#include "interfaces/IConfigurable.hpp"
#include "interfaces/IDataSource.hpp"
#include "interfaces/IStatusReporter.hpp"
#include "interfaces/IWebContributor.hpp"
#include "interfaces/IMCPContributor.hpp"
#include "interfaces/IBootReporter.hpp"
#include "interfaces/IPreconditionContributor.hpp"
#include "events/AppConfiguringPayload.hpp"
#include "events/PreconditionHealthPayload.hpp"
#include "events/DataSourceHealthPayload.hpp"
#include "PreconditionEngine/PreconditionEngine.hpp"

#include "EventBus.hpp"
#include "DataManager.hpp"
#include "WebServer.hpp"
#include "ConfigManager.hpp"

// Core domain services (owned by value — full includes required)
#include "WifiManager.hpp"
#include "WifiPreconditionContributor.hpp"
#include "RedactorPreconditionContributor.hpp"
#include "KeyManager.hpp"
#include "AuthManager.hpp"
#include "DeviceCrypto.hpp"





// ---------------------------------------------------------------------------
// Application Events
// ---------------------------------------------------------------------------

namespace ApplicationEvents {
    /// @brief Published periodically to report current memory heap usage.
    /// Payload: const uint32_t* (free heap bytes)
    constexpr const char* SYSTEM_HEAP = "system.heap";

    /// @brief Published when the system transition to RUNNING state finishes.
    /// Payload: nullptr
    constexpr const char* APP_RUNNING = "app.running";

    /// @brief Published when a precondition changes status.
    /// Payload: const PreconditionHealthPayload*
    constexpr const char* PRECONDITION_HEALTH = "precondition.health";

    /// @brief Published when a data source changes health status.
    /// Payload: const DataSourceHealthPayload*
    constexpr const char* DATA_SOURCE_HEALTH = "data.source.health";

    /// @brief Published when entering or updating CONFIGURING state.
    /// Payload: const AppConfiguringPayload*
    constexpr const char* APP_CONFIGURING = "app.configuring";

#ifdef WITH_OTA
    /// @brief Published during OTA progress updates.
    /// Payload: const OtaProgressPayload*
    constexpr const char* OTA_PROGRESS = "ota.progress";

    struct OtaProgressPayload {
        float progress;       // 0.0 to 1.0 download fraction
        const char* status;   // Human-readable status message
        bool complete;        // True when OTA cycle is finished
        bool success;         // True if OTA succeeded (only valid when complete)
    };
#else
    // EO-07: Legible absent-module error for consumers attempting to use OTA without the flag
    struct OTA_PROGRESS_requires_WITH_OTA {};
    constexpr OTA_PROGRESS_requires_WITH_OTA OTA_PROGRESS{}; ///< Sentinel — compile error if used without WITH_OTA
    
    struct OtaProgressPayload_requires_WITH_OTA {};
    typedef OtaProgressPayload_requires_WITH_OTA OtaProgressPayload;
#endif

#ifndef WITH_AUDIT
    // Legible absent-module error for consumers attempting to use AuditModule without the flag
    struct AuditModule_requires_WITH_AUDIT {};
#endif
}

/**
 * @brief Composition root for the ESP32 App Foundation framework.
 *
 * Application owns the full module registry and drives the Hollywood-principle
 * lifecycle: `setup()` and `loop()` in main.cpp delegate entirely to this class.
 * No application code should call `loop()` directly; modules register themselves
 * and the hub dispatches tick() to every ITickable in registration order.
 *
 * Two-tier vector model (CA-05):
 *  - Tier 1 (Boot): Vectors accept push_back() during begin() registration phase.
 *  - Tier 2 (Lock): After _compactAndLock() is called, vectors are shrink_to_fit'd
 *    and a _locked flag is set. Any subsequent registration attempt is rejected with
 *    an error log and returns false. This provides functionally-static runtime
 *    behaviour without requiring MAX_* compile-time constants.
 *
 * ConfigManager is owned directly by Application and registered as a web
 * contributor. Modules register with _configManager via registerModule()
 * and their config is loaded during beginAll().
 */
class Application {
public:
    /** @brief Destructor. */
    ~Application();

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Prevent the framework from rendering the default "App" bottom-nav tab.
     * Use this if your application registers its own top-level navigation tab.
     * Must be called before `begin()`.
     */
    void disableDefaultAppTab() { _enableDefaultAppTab = false; }

    /** @brief Set the application name displayed in the portal title bar. */
    void setAppName(const char* name) { _appName = name; }

    /** @brief Get the application name. Returns APP_TITLE if not set. */
    const char* getAppName() const { return _appName ? _appName : APP_TITLE; }

    /**
     * @brief Convert a display name to a valid hostname.
     * Lowercase, spaces/underscores → hyphens, strip non-[a-z0-9-], collapse
     * consecutive hyphens, trim leading/trailing hyphens, truncate to 32 chars.
     * Returns "esp32-device" if the result is empty.
     */
    static String sanitiseHostname(const char* displayName);

    /** @brief Set the manufacturer name for device identity (DIS, portal). */
    void setManufacturer(const char* name) { _manufacturer = name; }

    /** @brief Get the manufacturer name. Returns "CB Labs" if not set. */
    const char* getManufacturer() const { return _manufacturer ? _manufacturer : "CB Labs"; }

    /** @brief Set an optional CSS stylesheet path for portal branding. */
    void setAppStylesheet(const char* path) { _appStylesheet = path; }

    /** @brief Get the app stylesheet path. Returns nullptr if not set. */
    const char* getAppStylesheet() const { return _appStylesheet; }

    /**
     * @brief Execute the full framework boot sequence.
     *
     * Ordered steps:
     *   1. Log boot banner with BUILD_VERSION
     *   2. Set state to BOOT
     *   3. EventBus.begin() — first service, zero deps, enables boot-time events
     *   4. Mount LittleFS (hardware only; stubbed in native builds)
     *   5. Store FrameworkConfig fields internally
     *   6. DeviceCrypto.begin() + KeyManager.begin()
     *   7. ConfigManager — register framework IConfigurable modules (WifiManager)
     *   8. ConfigManager.beginAll() — loads persisted config for ALL registered modules
     *   9. (Removed — entire begin() runs under BOOT)
     *  10. WifiManager.begin() — now has persisted SSID/hostname from step 8
     *  11. TimeManager.begin() — after WiFi, needs network for NTP
     *  12. DataManager.begin() — background data workers
     *  13. OTA — heap-allocate and begin()
     *  14. Register core web contributors and tickables
     *  15. WebServerManager + MCPServer begin()
     *  16. Register built-in WiFi config gate
     *  17. _compactAndLock() — vectors are locked from this point
     *  18. Publish system.heap event + _reportHeap()
     *  19. clearStatus()
     *  20. Check config gates → RUNNING or CONFIGURING
     *
     * Boot progress is reported via IStatusReporter at each milestone.
     * Downstream apps can call registerStatusReporter() before begin()
     * to enable visual boot progress on a display. When no reporter is
     * registered, the NullStatusReporter fallback absorbs all calls.
     *
     * @param config  Framework-level configuration struct (OTA settings, etc.)
     * @return true if boot completed successfully; false on a fatal error (e.g. LittleFS mount failure)
     */
    bool begin(const FrameworkConfig& config);

    /**
     * @brief Dispatch the periodic update tick to all registered ITickable modules.
     *
     * Guarded: does nothing unless the current state is AppState::Configuring
 *     or AppState::Running. During CONFIGURING, polls config gates each cycle
 *     and transitions to RUNNING when all are satisfied.
     * Each registered ITickable::tick() must complete within 10ms.
     * Also ticks the MCPServer for JSON-RPC serial polling.
     */
    void tick();

    /**
     * @brief Log the soft-reset reason and restart the device.
     *
     * On hardware this calls esp_restart() .
     * In native GoogleTest builds the call is a no-op so tests can verify
     * that the method does not throw or crash.
     */
    void softReset();

    // -------------------------------------------------------------------------
    // Module Registration (Tier 1 — pre-lock only)
    // -------------------------------------------------------------------------

    /**
     * @brief Universal module registration — auto-detects all framework interfaces.
     *
     * Uses compile-time if constexpr to check which interfaces T implements,
     * and registers the module with every applicable subsystem in a single call.
     * This is the primary, recommended registration API for both framework
     * internals and downstream application modules.
     *
     * @tparam T  Concrete module type (deduced from pointer).
     * @param module  Pointer to the module instance. Must not be null.
     * @return true if at least one interface was detected and registered.
     */
    template<typename T>
    bool registerModule(T* module) {
        if (!module) return false;
        bool any = false;
        if constexpr (std::is_base_of_v<ITickable, T>)        { registerTickable(module);              any = true; }
        if constexpr (std::is_base_of_v<IConfigurable, T>)    { registerConfigurable(module);          any = true; }
        if constexpr (std::is_base_of_v<IWebContributor, T>)  { registerWebContributor(module);        any = true; }
        if constexpr (std::is_base_of_v<IMCPContributor, T>)  { registerMcpContributor(module);        any = true; }
        if constexpr (std::is_base_of_v<IDataSource, T>)      { registerDataSource(module);            any = true; }
        if constexpr (std::is_base_of_v<IStatusReporter, T>)  { registerStatusReporter(module);        any = true; }
        if constexpr (std::is_base_of_v<IBootReporter, T>)    { registerBootReporter(module);          any = true; }
        if constexpr (std::is_base_of_v<IPreconditionContributor, T>) { module->registerPreconditions(_preconditionEngine); any = true; }
        if (!any) {
            ESP_LOGW("Application", "registerModule: type implements no known framework interface");
        }
        return any;
    }

    /**
     * @brief Register a module that participates in the Application tick loop.
     * @param module  Non-null pointer to the ITickable implementation.
     * @return true if registration succeeded; false if locked or nullptr.
     */
    bool registerTickable(ITickable* module);

    /**
     * @brief Register a module that participates in config persistence.
     * @param module  Non-null pointer to the IConfigurable implementation.
     * @return true if registration succeeded; false if locked or nullptr.
     */
    bool registerConfigurable(IConfigurable* module);

    /**
     * @brief Register a boot reporter for early hardware init.
     * @param reporter  Non-null pointer to the IBootReporter implementation.
     * @return true if registration succeeded; false if locked or nullptr.
     */
    bool registerBootReporter(IBootReporter* reporter);

    /**
     * @brief Set (or replace pre-lock) the single status-reporter adapter.
     *
     * Last-write-wins before the lock; typically called once by the display module.
     * @param reporter  Non-null pointer to the IStatusReporter implementation.
     * @return true if registration succeeded; false if locked or nullptr.
     */
    bool registerStatusReporter(IStatusReporter* reporter);

    /**
     * @brief Register a module that contributes REST API routes to the web server.
     * @param contributor  Non-null pointer to the IWebContributor implementation.
     * @return true if registration succeeded; false if locked or nullptr.
     */
    bool registerWebContributor(IWebContributor* contributor);

    /**
     * @brief Register a module that contributes MCP tools to the MCP server.
     * @param contributor Non-null pointer to the IMCPContributor implementation.
     * @return true if registration succeeded; false if locked or nullptr.
     */
    bool registerMcpContributor(IMCPContributor* contributor);

    /**
     * @brief Register a background data-fetch worker with the DataManager.
     * @param source  Non-null pointer to the IDataSource implementation.
     * @return true if registration succeeded; false if locked or nullptr.
     */
    bool registerDataSource(IDataSource* source);

    // -------------------------------------------------------------------------
    // Core Services Accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Get the application's EventBus instance.
     * @return Reference to the EventBus.
     */
    EventBus& getEventBus();

    /**
     * @brief Get the application's DataManager instance.
     * @return Reference to the DataManager.
     */
    DataManager& getDataManager();

    /**
     * @brief Get the application's WebServerManager instance.
     * @return Reference to the WebServerManager.
     */
    WebServerManager& getWebServerManager();

    /**
     * @brief Get registered MCP contributors vector.
     * @return Const reference to the IMCPContributor vector.
     */
    const std::vector<IMCPContributor*>& getMcpContributors() const { return _mcpContributors; }

    /**
     * @brief Get the application's ConfigManager instance.
     * @return Reference to the ConfigManager gateway.
     */
    ConfigManager& getConfigManager();

    /**
     * @brief Get the application's PreconditionEngine instance.
     * @return Reference to the PreconditionEngine.
     */
    PreconditionEngine& getPreconditionEngine() { return _preconditionEngine; }

    // -------------------------------------------------------------------------
    // Core Domain Service Accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Get the application's WifiManager instance.
     * @return Reference to the WifiManager.
     */
    WifiManager& getWifiManager();

    /**
     * @brief Get the application's KeyManager instance.
     * @return Reference to the KeyManager.
     */
    KeyManager& getKeyManager();

    /**
     * @brief Get the application's AuthManager instance.
     * @return Reference to the AuthManager security service.
     */
    AuthManager& getAuthManager();

    /**
     * @brief Get the application's FrameworkConfig (read-only).
     * @return Const reference to the stored FrameworkConfig.
     */
    const FrameworkConfig& getConfig() const;



    // -------------------------------------------------------------------------
    // State Accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Query the current application lifecycle phase.
     * @return Current AppState enum value.
     */
    AppState getState() const;

private:
    // -------------------------------------------------------------------------
    // Private lifecycle helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Advance the lifecycle state and log the transition.
     * @param state  The new AppState to enter.
     */
    void setState(AppState state);

    /**
     * @brief Lock all registration vectors after the boot phase completes.
     *
     * Calls shrink_to_fit() on every std::vector to release excess capacity,
     * then sets _locked = true so any post-lock registration attempt is rejected.
     */
    void _compactAndLock();

    /**
     * @brief Publish an "app.configuring" EventBus event for the first
     *        unsatisfied precondition, allowing display modules to react.
     * @param isRegression true if this CONFIGURING entry is a regression from RUNNING.
     */
    void _publishConfiguringEvent(bool isRegression = false);

    /**
     * @brief Log the current free-heap and minimum free-heap figures.
     *
     * Hardware only .
     */
    void _reportHeap();

    /**
     * @brief Display-driven state transition: entering CONFIGURING.
     *
     * Suspends carousel, shows the ConfiguringStatusBoard as base.
     * Called from tick() when a precondition regression is detected, and from
     * begin() when initial precondition check fails.
     *
     * @param isRegression true if regressing from RUNNING (vs initial boot).
     */
    void _onEnterConfiguring(bool isRegression = false);

    /**
     * @brief Display-driven state transition: entering RUNNING.
     *
     * Clears modal/base overlays and resumes carousel rotation.
     * Called from tick() when all startup preconditions become satisfied.
     */
    void _onEnterRunning();

    // -------------------------------------------------------------------------
    // Member variables
    // -------------------------------------------------------------------------

    AppState       _state   = AppState::Boot;    ///< Current lifecycle phase
    bool           _locked  = false;             ///< True once _compactAndLock() has been called
    // Note on const-correctness (M-04): _config cannot be declared const because
    // Application::begin() assigns it post-construction. Const-correctness is
    // enforced through the public API: begin() accepts const FrameworkConfig&,
    // and getConfig() returns const FrameworkConfig&. No method mutates _config
    // after begin() completes.
    FrameworkConfig _config;                     ///< Cached framework configuration

    // Module registry vectors (two-tier model)
    std::vector<ITickable*>        _tickables;       ///< Modules receiving tick() dispatches
    std::vector<IWebContributor*>  _webContributors; ///< Modules that contribute REST API routes
    std::vector<IMCPContributor*>  _mcpContributors; ///< Modules that contribute MCP tools
    std::vector<IDataSource*>      _dataSources;     ///< Background data-fetch workers

    // Single-pointer registrations (no vector needed)

    /**
     * @brief Silent fallback reporter used when no display is registered.
     *
     * In native builds, no hardware display is available, so the null reporter
     * absorbs all reportStatus/reportError/clearStatus calls silently.
     * On hardware, _statusReporter is reassigned to &_display in begin().
     */
    class NullStatusReporter : public IStatusReporter {
    public:
        /** @brief No-op status report (null reporter). */
        void reportStatus(const char* /*message*/, float /*progress*/ = -1.0f) override {}
        /** @brief No-op error report (null reporter). */
        void reportError(const char* /*errorMessage*/) override {}
        /** @brief No-op status clear (null reporter). */
        void clearStatus() override {}
    };

    NullStatusReporter _nullReporter;                               ///< Fallback status reporter instance
    IStatusReporter* _statusReporter = &_nullReporter;              ///< Display adapter for boot-phase status overlays
    IBootReporter* _bootReporter = nullptr;                         ///< Boot reporter for early hardware init

    // Core infrastructure services
    PreconditionEngine _preconditionEngine;                ///< Central precondition engine
    struct PreconditionStatusRecord {
        bool initialized = false;
        bool met = false;
        Cause cause = Cause::NotConfigured;
        Severity severity = Severity::Transient;
        bool everMet = false;
    };
    PreconditionStatusRecord _lastPreconditionStatus[MAX_PRECONDITIONS];
    PreconditionHealthPayload _preconditionPayloads[MAX_PRECONDITIONS];  ///< Persistent payloads for EventBus (WO7)
    ConfigManager _configManager;       ///< Centralised config gateway (owned by Application)
    EventBus _eventBus;                 ///< Central event bus
    uint32_t _systemHeapPayload = 0;    ///< Persistent payload for system.heap event
    AppConfiguringPayload _configuringPayload{}; ///< Persistent payload for app.configuring event

    // Gate-transition re-publishing state (PR-66)
    const char*      _lastPublishedGate   = nullptr;                     ///< Gate name from last APP_CONFIGURING
    Cause            _lastPublishedCause  = Cause::Met;                  ///< Cause from last publish
    Periodic         _configuringPublishThrottle{500};                    ///< Gate re-publish debounce (default 500ms, overridable via FrameworkConfig)
    /// Stable buffer for device IP pointer — IPv4 only (max "255.255.255.255\0" = 16 bytes).
    /// If IPv6 support is added, increase to INET6_ADDRSTRLEN (46).
    char             _deviceIPBuf[16] = {};
    /// Stable buffer for precondition help text pointer in configuring payloads.
    char             _helpTextBuf[128] = {};
    /// Stable buffer for WiFi hostname pointer in configuring payloads.
    char             _hostnameBuf[33] = {};

    DataManager _dataManager;           ///< Background data manager
    WebServerManager _webServerManager; ///< Local HTTP Server


    // Core domain services
    DeviceCrypto _deviceCrypto;         ///< Hardware-bound AES-256 crypto service
    KeyManager _keyManager;             ///< Encrypted credential registry
    AuthManager _authManager;           ///< Portal + MCP authentication manager (WO1 skeleton)
    WifiManager _wifiManager;           ///< WiFi connection state machine
    WifiPreconditionContributor _wifiPrecondition{_wifiManager}; ///< Built-in WiFi STA precondition contributor
    RedactorPreconditionContributor _redactorPrecondition;       ///< Built-in Redactor health precondition contributor

    // Heartbeat
    Periodic _heartbeatPeriodic{30000};                              ///< Throttle for periodic heartbeat log (30s)

    // -------------------------------------------------------------------------
    // App tab contributor — registers the top-level "App" bottom-nav tab.
    // Application owns this tab; it is not the responsibility of any hardware
    // module. App-level configuration (e.g., clock font size) lives here.
    // -------------------------------------------------------------------------

    struct AppTabContributor : public IWebContributor {
        /** @brief No-op — routes registered inline in begin(). */
        void        registerRoutes(PsychicHttpServer& /*server*/) override {}
        /** @brief Register the top-level App tab. */
        void registerTabs(ITabRegistry& registry) override {
            registry.addTab("App", nullptr, 30);
        }
    };
    AppTabContributor _appTabContributor;  ///< Contributes "App" top-level nav tab
    bool _enableDefaultAppTab = true;      ///< Guard for default App tab registration
    const char* _appName = nullptr;        ///< Runtime-settable application name (overrides APP_TITLE)
    const char* _manufacturer = nullptr;   ///< Runtime-settable manufacturer name (defaults to "CB Labs")
    const char* _appStylesheet = nullptr;  ///< Optional CSS path for portal branding

    // -------------------------------------------------------------------------
    // Precondition web contributor — exposes GET /api/preconditions snapshot
    // and POST /api/preconditions/retry endpoint.
    // -------------------------------------------------------------------------
    struct PreconditionWebContributor : public IWebContributor {
        Application* app = nullptr;
        /** @brief Register routes. */
        void        registerRoutes(PsychicHttpServer& server) override;
        /** @brief No-op — tabs registered by Application. */
        void        registerTabs(ITabRegistry& /*registry*/) override {}
        const char* getApiPrefix() const override { return "/api/preconditions"; }
        /** @brief Return max POST body size for precondition endpoints. */
        size_t      getMaxPostBody() const override { return 1024; }
    };
    PreconditionWebContributor _preconditionWebContributor;

    // -------------------------------------------------------------------------
    // Data source web contributor — exposes GET /api/data/sources snapshot
    // and POST /api/data/sources/reset endpoint.
    // -------------------------------------------------------------------------
    struct DataSourceWebContributor : public IWebContributor {
        Application* app = nullptr;
        /** @brief Register routes. */
        void        registerRoutes(PsychicHttpServer& server) override;
        /** @brief No-op — tabs registered by Application. */
        void        registerTabs(ITabRegistry& /*registry*/) override {}
        const char* getApiPrefix() const override { return "/api/data"; }
        /** @brief Return max POST body size for data source endpoints. */
        size_t      getMaxPostBody() const override { return 1024; }
    };
    DataSourceWebContributor _dataSourceWebContributor;

};
