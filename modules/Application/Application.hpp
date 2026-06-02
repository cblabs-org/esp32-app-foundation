/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/application/Application.hpp
 * Description: Composition root for the ESP32 App Foundation framework.
 *              Owns the module registry (two-tier vector model), the
 *              BOOT → CONFIGURING ↔ RUNNING lifecycle, the
 *              main-loop tick dispatch, and the soft-reset escape hatch.
 *              A single static instance is created by main.cpp (WO4); the
 *              class is NOT a singleton — there is no getInstance().
 *
 *              ConfigManager is owned by Application and serves as the
 *              centralised gateway for all module configuration I/O.
 *
 *              Core domain services (WifiManager, OTAUpdateManager,
 *              TimeManager, KeyManager) are also Application-owned members,
 *              wired into the boot sequence and registered with ConfigManager
 *              and the tick/web contributor vectors where applicable.
 *
 * Exported Classes:
 * - Application: Framework composition root and module registry
 *   - begin():                   Run the full boot sequence; returns false on fatal error
 *   - tick():                    Dispatch to all registered ITickable modules (RUNNING guard)
 *   - softReset():               Log reason and trigger esp_restart() (NATIVE_BUILD guarded)
 *   - registerTickable():        Add an ITickable module to the tick dispatch vector
 *   - registerStatusReporter():  Set (or replace) the single IStatusReporter adapter
 *   - registerWebContributor():  Add an IWebContributor module to the route-registration vector
 *   - registerMcpContributor():  Add an IMCPContributor module to the MCP tools vector
 *   - registerDataSource():      Add an IDataSource worker to the DataManager vector
 *   - getConfigManager():        Access the ConfigManager gateway
 *   - getWifiManager():          Access the WifiManager domain service
 *   - getOtaManager():           Access the OTAUpdateManager domain service
 *   - getTimeManager():          Access the TimeManager domain service
 *   - getKeyManager():           Access the KeyManager domain service
 *   - getState():                Query the current AppState lifecycle phase
 *   - getMCPServer():            Access the MCP JSON-RPC server instance
 *   - registerConfigGate():      Add an IConfigGate prerequisite for RUNNING entry
 */

#pragma once
#include <vector>
#include "AppState.hpp"
#include "FrameworkConfig.hpp"
#include "interfaces/ITickable.hpp"
#include "interfaces/IConfigurable.hpp"
#include "interfaces/IDataSource.hpp"
#include "interfaces/IStatusReporter.hpp"
#include "interfaces/IWebContributor.hpp"
#include "interfaces/IMCPContributor.hpp"
#include "interfaces/IConfigGate.hpp"

#include "../EventBus/EventBus.hpp"
#include "../DataManager/DataManager.hpp"
#include "../WebServer/WebServerManager.hpp"
#include "../MCPServer/MCPServer.hpp"
#include "../ConfigManager/ConfigManager.hpp"

// Core domain services (owned by value — full includes required)
#include "../WifiManager/WifiManager.hpp"
#include "../WifiManager/WifiConfigGate.hpp"
#include "../TimeManager/TimeManager.hpp"
#include "../TimeManager/TimeConfigGate.hpp"
#include "../KeyManager/KeyManager.hpp"
#include "../../lib/DeviceCrypto/DeviceCrypto.hpp"

// Forward declaration only — OTAUpdateManager.hpp pulls in WiFiClientSecure
// which causes LDF to discover NetworkClientSecure with missing SSL backend.
// Full include deferred to Application.cpp; member held via unique_ptr.
class OTAUpdateManager;

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
    /// Destructor — cleans up heap-allocated OTAUpdateManager.
    ~Application();

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Execute the full framework boot sequence.
     *
     * Ordered steps:
     *   1. Log boot banner with APP_FOUNDATION_VERSION
     *   2. Set state to BOOT
     *   3. EventBus.begin() — first service, zero deps, enables boot-time events
     *   4. Mount LittleFS (hardware only; stubbed in NATIVE_BUILD)
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
     * @param config  Framework-level configuration struct (hostname, timezone, etc.)
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
     * On hardware this calls esp_restart() (wrapped in NATIVE_BUILD guard).
     * In native GoogleTest builds the call is a no-op so tests can verify
     * that the method does not throw or crash.
     */
    void softReset();

    // -------------------------------------------------------------------------
    // Module Registration (Tier 1 — pre-lock only)
    // -------------------------------------------------------------------------

    /**
     * @brief Register a module that participates in the Application tick loop.
     * @param module  Non-null pointer to the ITickable implementation.
     * @return true if registration succeeded; false if locked or nullptr.
     */
    bool registerTickable(ITickable* module);

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

    /**
     * @brief Register a configuration prerequisite gate.
     *
     * Gates are evaluated after boot completes. If any gate is unsatisfied,
     * the app enters CONFIGURING instead of RUNNING. During CONFIGURING,
     * tick() polls all gates each cycle and transitions to RUNNING when
     * all return isSatisfied() == true.
     *
     * Pre-lock only (same as other registration methods).
     *
     * @param gate  Non-null pointer to the IConfigGate implementation.
     * @return true if registration succeeded; false if locked or nullptr.
     */
    bool registerConfigGate(IConfigGate* gate);

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
     * @brief Get the application's MCPServer instance.
     * @return Reference to the MCPServer.
     */
    MCPServer& getMCPServer();

    /**
     * @brief Get the application's ConfigManager instance.
     * @return Reference to the ConfigManager gateway.
     */
    ConfigManager& getConfigManager();

    // -------------------------------------------------------------------------
    // Core Domain Service Accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Get the application's WifiManager instance.
     * @return Reference to the WifiManager.
     */
    WifiManager& getWifiManager();

    /**
     * @brief Get the application's OTAUpdateManager instance.
     * @return Reference to the OTAUpdateManager.
     */
    OTAUpdateManager& getOtaManager();

    /**
     * @brief Get the application's TimeManager instance.
     * @return Reference to the TimeManager.
     */
    TimeManager& getTimeManager();

    /**
     * @brief Get the application's TimeConfigGate for downstream opt-in.
     *
     * Downstream apps that require timezone configuration can register
     * this gate: app.registerConfigGate(&app.getTimeGate())
     *
     * @return Reference to the built-in TimeConfigGate.
     */
    TimeConfigGate& getTimeGate();

    /**
     * @brief Get the application's KeyManager instance.
     * @return Reference to the KeyManager.
     */
    KeyManager& getKeyManager();

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

    /**
     * @brief Return a const reference to the registered config gates vector.
     *
     * Used by the display module to bind ConfiguringStatusBoard to the
     * gate list for rendering gate-by-gate progress during CONFIGURING.
     *
     * @return Const reference to the config gates vector.
     */
    const std::vector<IConfigGate*>& getConfigGates() const { return _configGates; }


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
     * @brief Check whether all registered config gates are satisfied.
     * @return true if no gates are registered or all return isSatisfied().
     */
    bool _allGatesSatisfied() const;

    /**
     * @brief Publish an "app.configuring" EventBus event for the first
     *        unsatisfied gate, allowing display modules to react.
     * @param isRegression true if this CONFIGURING entry is a regression from RUNNING.
     */
    void _publishConfiguringEvent(bool isRegression = false);

    /**
     * @brief Log the current free-heap and minimum free-heap figures.
     *
     * Hardware only (wrapped in NATIVE_BUILD guard).
     */
    void _reportHeap();

    // -------------------------------------------------------------------------
    // Member variables
    // -------------------------------------------------------------------------

    AppState       _state   = AppState::Boot;    ///< Current lifecycle phase
    bool           _locked  = false;             ///< True once _compactAndLock() has been called
    FrameworkConfig _config;                     ///< Cached framework configuration

    // Module registry vectors (two-tier model)
    std::vector<ITickable*>        _tickables;       ///< Modules receiving tick() dispatches
    std::vector<IWebContributor*>  _webContributors; ///< Modules that contribute REST API routes
    std::vector<IMCPContributor*>  _mcpContributors; ///< Modules that contribute MCP tools
    std::vector<IDataSource*>      _dataSources;     ///< Background data-fetch workers
    std::vector<IConfigGate*>       _configGates;     ///< Configuration prerequisite gates

    // Regression gate-check members
    static constexpr uint32_t GATE_CHECK_INTERVAL_MS = 5000; ///< Throttle interval for runtime gate checks
    unsigned long _lastGateCheckMs = 0;                      ///< Timestamp of last gate regression check
    static constexpr uint8_t MAX_CONFIG_GATES = 8;           ///< Maximum number of config gates
    unsigned long _gateFailedSinceMs[MAX_CONFIG_GATES] = {0}; ///< Per-gate unsatisfied-since timestamps

    // Single-pointer registrations (no vector needed)
    
    /**
     * @brief Silent fallback reporter used when no display is registered.
     */
    class NullStatusReporter : public IStatusReporter {
    public:
        void reportStatus(const char* /*message*/, float /*progress*/ = -1.0f) override {}
        void reportError(const char* /*errorMessage*/) override {}
        void clearStatus() override {}
    };

    NullStatusReporter _nullReporter;                               ///< Fallback status reporter instance
    IStatusReporter* _statusReporter = &_nullReporter;              ///< Display adapter for boot-phase status overlays

    // Core infrastructure services
    ConfigManager _configManager;       ///< Centralised config gateway (owned by Application)
    EventBus _eventBus;                 ///< Central event bus
    DataManager _dataManager;           ///< Background data manager
    WebServerManager _webServerManager; ///< Local HTTP Server
    MCPServer _mcpServer;               ///< MCP JSON-RPC server

    // Core domain services
    DeviceCrypto _deviceCrypto;         ///< Hardware-bound AES-256 crypto service
    KeyManager _keyManager;             ///< Encrypted credential registry
    WifiManager _wifiManager;           ///< WiFi connection state machine
    WifiConfigGate _wifiGate{_wifiManager}; ///< Built-in WiFi STA config gate
    OTAUpdateManager* _otaManager = nullptr;  ///< OTA firmware update orchestrator (heap, fwd-declared)
    TimeManager _timeManager;           ///< NTP synchronization service
    TimeConfigGate _timeGate{_timeManager}; ///< Optional timezone config gate (downstream opt-in)
};
