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
 *              BOOTING → WIFI_SETUP → APP_SETUP → RUNNING lifecycle, the
 *              main-loop tick dispatch, and the soft-reset escape hatch.
 *              A single static instance is created by main.cpp (WO4); the
 *              class is NOT a singleton — there is no getInstance().
 *
 * Exported Classes:
 * - Application: Framework composition root and module registry
 *   - begin():                   Run the full boot sequence; returns false on fatal error
 *   - tick():                    Dispatch to all registered ITickable modules (RUNNING guard)
 *   - softReset():               Log reason and trigger esp_restart() (NATIVE_BUILD guarded)
 *   - registerTickable():        Add an ITickable module to the tick dispatch vector
 *   - registerConfigurable():    Add an iConfigurable module to the config-load vector
 *   - registerStatusReporter():  Set (or replace) the single IStatusReporter adapter
 *   - registerWebContributor():  Add an IWebContributor module to the route-registration vector
 *   - registerDataSource():      Add an iDataSource worker to the DataManager vector
 *   - getState():                Query the current AppState lifecycle phase
 */

#pragma once
#include <vector>
#include "AppState.hpp"
#include "FrameworkConfig.hpp"
#include "interfaces/ITickable.hpp"
#include "interfaces/iConfigurable.hpp"
#include "interfaces/iDataSource.hpp"
#include "interfaces/IStatusReporter.hpp"
#include "interfaces/IWebContributor.hpp"

#include "../eventBus/EventBus.hpp"
#include "../dataManager/dataManager.hpp"

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
 */
class Application {
public:
    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Execute the full framework boot sequence.
     *
     * Ordered steps:
     *   1. Log boot banner with APP_FOUNDATION_VERSION
     *   2. Set state to BOOTING
     *   3. Mount LittleFS (hardware only; stubbed in NATIVE_BUILD)
     *   4. Store FrameworkConfig fields internally
     *   5. Set state to WIFI_SETUP
     *   6. TODO: wifiManager.begin()  (PR-02)
     *   7. Set state to APP_SETUP
     *   8. Call loadConfig() on all registered iConfigurable modules
     *   9. TODO: DataManager and EventBus begin() stubs (PR-03)
     *  10. Call _compactAndLock() — vectors are locked from this point
     *  11. Call _reportHeap()
     *  12. Set state to RUNNING
     *  13. Return true
     *
     * @param config  Framework-level configuration struct (hostname, timezone, etc.)
     * @return true if boot completed successfully; false on a fatal error (e.g. LittleFS mount failure)
     */
    bool begin(const FrameworkConfig& config);

    /**
     * @brief Dispatch the periodic update tick to all registered ITickable modules.
     *
     * Guarded: does nothing unless the current state is AppState::RUNNING.
     * Each registered ITickable::tick() must complete within 10ms.
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
     * @brief Register a module that loads/persists configuration via LittleFS.
     * @param module  Non-null pointer to the iConfigurable implementation.
     * @return true if registration succeeded; false if locked or nullptr.
     */
    bool registerConfigurable(iConfigurable* module);

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
     * @brief Register a background data-fetch worker with the DataManager.
     * @param source  Non-null pointer to the iDataSource implementation.
     * @return true if registration succeeded; false if locked or nullptr.
     */
    bool registerDataSource(iDataSource* source);

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
     * @brief Log the current free-heap and minimum free-heap figures.
     *
     * Hardware only (wrapped in NATIVE_BUILD guard).
     */
    void _reportHeap();

    // -------------------------------------------------------------------------
    // Member variables
    // -------------------------------------------------------------------------

    AppState       _state   = AppState::BOOTING; ///< Current lifecycle phase
    bool           _locked  = false;             ///< True once _compactAndLock() has been called
    FrameworkConfig _config;                     ///< Cached framework configuration

    // Module registry vectors (two-tier model)
    std::vector<ITickable*>       _tickables;       ///< Modules receiving tick() dispatches
    std::vector<iConfigurable*>   _configurables;   ///< Modules whose loadConfig() is called during begin()
    std::vector<IWebContributor*> _webContributors; ///< Modules that contribute REST API routes
    std::vector<iDataSource*>     _dataSources;     ///< Background data-fetch workers

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

    // Core services
    EventBus _eventBus;           ///< Central event bus
    DataManager _dataManager;     ///< Background data manager
};
