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
     * @brief How long (ms) a gate can remain unsatisfied during RUNNING
     *        before triggering regression to CONFIGURING.
     *
     * Return 0 for immediate regression.
     * Return UINT32_MAX to never regress (app handles recovery internally).
     *
     * @return Tolerance threshold in milliseconds. Default: 30000 (30s).
     */
    virtual uint32_t getRegressionToleranceMs() const { return 30000; } // 30s default
};
