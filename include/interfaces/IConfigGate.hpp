/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/interfaces/IConfigGate.hpp
 * Description: Interface for extensible configuration prerequisites. Each gate
 *              represents a condition that must be satisfied before the
 *              Application transitions from CONFIGURING to RUNNING. The
 *              framework provides a built-in WiFi gate; downstream apps
 *              register their own gates for app-specific requirements
 *              (API keys, board configuration, etc.).
 *
 * Exported Classes:
 * - IConfigGate: Abstract interface for configuration prerequisites
 *   - isSatisfied():  Check whether this gate's condition is met
 *   - getGateName():  Return a human-readable identifier for logging
 *   - getHelpText():  Return user-facing guidance (optional, default empty)
 */

#pragma once

#include <cstdint>

/// Action status for a config gate during the CONFIGURING phase.
/// Used by Application to populate AppConfiguringPayload and by display
/// modules to decide whether to show a modal helper or a progress indicator.
enum class GateActionStatus : uint8_t {
    Satisfied       = 0,  ///< Gate condition is met — no action needed
    InProgress      = 1,  ///< Gate is actively working toward satisfaction (e.g., WiFi connecting)
    NeedsUserAction = 2   ///< Gate requires user intervention (e.g., AP mode, no config)
};

/// Well-known gate name constants. These are part of the public API —
/// display modules use string matching for built-in modal dispatch.
/// Changing a gate name is a breaking change.
namespace GateNames {
    static constexpr const char* WIFI_STATION = "WiFi Station";
    static constexpr const char* TIMEZONE     = "Timezone";
}

/**
 * @brief Abstract interface for application configuration prerequisites.
 *
 * Modules or downstream apps implement this interface to define conditions
 * that must be met before the application can enter the RUNNING state.
 * During the CONFIGURING phase, Application::tick() polls all registered
 * gates each cycle. Once every gate's isSatisfied() returns true, the
 * state advances to RUNNING.
 *
 * Example gates:
 * - WiFi in station mode (framework-provided)
 * - API keys configured (app-specific)
 * - Data sources selected (app-specific)
 *
 * Gates are registered via Application::registerConfigGate() before
 * app.begin() is called (pre-lock, same as ITickable/IWebContributor).
 */
class IConfigGate {
public:
    virtual ~IConfigGate() = default;

    /**
     * @brief Check whether this prerequisite is currently met.
     *
     * Called every tick() cycle during CONFIGURING. Implementations should
     * return a cached value (cheap boolean read) — do not perform I/O here.
     *
     * @return true if the gate is satisfied and should not block RUNNING.
     */
    virtual bool isSatisfied() const = 0;

    /**
     * @brief Return a human-readable gate name for logging and diagnostics.
     * @return Short identifier, e.g. "WiFi Station", "API Keys", "Board Config".
     */
    virtual const char* getGateName() const = 0;

    /**
     * @brief Return user-facing guidance for satisfying this gate.
     *
     * Displayed on system boards or via the web portal to help the user
     * understand what configuration is needed.
     *
     * @return Help text, e.g. "Connect to the device AP and configure WiFi".
     *         Default implementation returns an empty string.
     */
    virtual const char* getHelpText() const { return ""; }

    /**
     * @brief Return the names of gates this gate depends on.
     *
     * During the CONFIGURING phase, a gate is only "actionable" if all of its
     * dependency gates are already satisfied. This enables ordered presentation
     * (e.g., "Connect WiFi" before "Set Timezone").
     *
     * Default: no dependencies (gate is always actionable when unsatisfied).
     *
     * @param[out] count Number of dependency names in the returned array.
     * @return Pointer to a static array of C-string gate names, or nullptr if none.
     */
    virtual const char* const* getDependencies(uint8_t& count) const {
        count = 0;
        return nullptr;
    }

    /**
     * @brief How long (ms) a gate can remain unsatisfied during RUNNING
     *        before triggering regression to CONFIGURING.
     *
     * Return 0 for immediate regression.
     * Return UINT32_MAX to never regress (app handles recovery internally).
     *
     * @return Tolerance threshold in milliseconds. Default: 30000 (30s).
     */
    virtual uint32_t getRegressionToleranceMs() const { return 30000; } // 30s default

    /// Return the current action status of this gate.
    ///
    /// Called every tick() cycle during CONFIGURING (same threading contract
    /// as isSatisfied() — cheap read, no I/O). The framework uses this to
    /// decide whether to show a modal helper screen (NeedsUserAction) or
    /// just the base progress underlay (InProgress).
    ///
    /// Default: returns Satisfied if isSatisfied() is true,
    /// NeedsUserAction otherwise.
    ///
    /// @return GateActionStatus reflecting current gate state.
    virtual GateActionStatus getActionStatus() const {
        return isSatisfied() ? GateActionStatus::Satisfied
                             : GateActionStatus::NeedsUserAction;
    }
};

// =============================================================================
// Extensibility Guide
// =============================================================================

/**
 * @page config_gate_extensibility Extending the Configuration Gate System
 *
 * ## Registration Model
 *
 * Only gates registered via `Application::registerConfigGate()` are mandatory.
 * Anything NOT registered is implicitly optional — the app handles it at
 * runtime without blocking the CONFIGURING → RUNNING transition.
 *
 * ## Framework Auto-Registration
 *
 * The framework automatically registers `WifiConfigGate` when WiFi is enabled.
 * Downstream apps do NOT need to register it manually.
 *
 * ## Downstream App Usage
 *
 * Add your own gates in `main.cpp` before calling `app.begin()`:
 *
 * ```cpp
 * #include "interfaces/IConfigGate.hpp"
 *
 * class ApiKeyGate : public IConfigGate {
 * public:
 *     bool isSatisfied() const override { return _hasApiKey; }
 *     const char* getGateName() const override { return "API Keys"; }
 *     const char* getHelpText() const override {
 *         return "Enter your API key via the web portal";
 *     }
 *
 *     // Depend on WiFi being connected first
 *     const char* const* getDependencies(uint8_t& count) const override {
 *         static const char* deps[] = {GateNames::WIFI_STATION};
 *         count = 1;
 *         return deps;
 *     }
 *
 *     // Allow 60s of regression before forcing re-configuration
 *     uint32_t getRegressionToleranceMs() const override { return 60000; }
 *
 *     void setHasApiKey(bool val) { _hasApiKey = val; }
 * private:
 *     bool _hasApiKey = false;
 * };
 *
 * // In main.cpp setup():
 * static ApiKeyGate apiKeyGate;
 * app.registerConfigGate(&apiKeyGate);
 * app.begin();
 * ```
 *
 * ## Gate Ordering
 *
 * Use `getDependencies()` to declare gate ordering. A gate is only "actionable"
 * when all of its dependency gates are already satisfied. This controls the
 * order in which the configuring status board presents gates to the user.
 *
 * ## Gate Action Status
 *
 * Override `getActionStatus()` to report the gate's current action status:
 * - `GateActionStatus::Satisfied` — gate condition is met (default when
 *   `isSatisfied()` returns true)
 * - `GateActionStatus::InProgress` — gate is actively working toward
 *   satisfaction (e.g., WiFi connecting with saved credentials)
 * - `GateActionStatus::NeedsUserAction` — gate requires user intervention
 *   (e.g., AP mode active, no credentials configured)
 *
 * The default implementation returns `Satisfied` when `isSatisfied()` is true
 * and `NeedsUserAction` otherwise. Override to report `InProgress` for gates
 * that have an autonomous resolution path (e.g., WiFi connecting).
 *
 * Threading contract: same as `isSatisfied()` — called from the main loop
 * only, must be a cheap read with no I/O.
 *
 * ## Gate Name Stability
 *
 * Gate names are part of the **public API**. Display modules use string
 * matching against well-known gate names (e.g., `GateNames::WIFI_STATION`)
 * for built-in modal dispatch. Changing a gate name is a **breaking change**.
 * Always use the `GateNames::` constants when declaring dependencies.
 *
 * ## Regression Tolerance
 *
 * Use `getRegressionToleranceMs()` to control how long a gate can remain
 * unsatisfied during RUNNING before triggering regression to CONFIGURING:
 * - Return `0` for immediate regression
 * - Return `UINT32_MAX` to never regress (app handles recovery internally)
 * - Default: `30000` (30 seconds)
 */
