/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/timeManager/TimeConfigGate.hpp
 * Description: Optional IConfigGate implementation that blocks RUNNING until
 *              the user has explicitly configured a timezone. Downstream apps
 *              where time is critical (clocks, schedules, etc.) register this
 *              gate via app.registerConfigGate(&app.getTimeGate()).
 *
 *              The gate checks whether applyConfig() has been called with a
 *              persisted config (i.e. /mod/time.json exists). It does NOT
 *              check NTP sync status — that is handled asynchronously by
 *              TimeManager::tick().
 *
 *              Depends on "WiFi Station" — the timezone portal can be used
 *              during AP mode (cached DB), but the gate ordering ensures
 *              WiFi is presented as the first actionable gate.
 *
 * Exported Classes:
 * - TimeConfigGate: Timezone configuration prerequisite gate
 *   - isSatisfied():  Returns true when timezone has been explicitly configured
 *   - getGateName():  Returns "Timezone"
 *   - getHelpText():  Returns setup instructions
 */

#pragma once

#include "../../include/interfaces/IConfigGate.hpp"
#include "TimeManager.hpp"

/**
 * @brief Config gate that requires the user to explicitly set a timezone.
 *
 * When the device boots without a persisted time config (/mod/time.json),
 * TimeManager::isConfigured() returns false. This gate blocks the RUNNING
 * transition until the user selects a timezone via the web portal's Time
 * sub-pill.
 *
 * This gate is NOT auto-registered by the framework. Downstream apps
 * where accurate time is critical must opt-in by calling:
 *   app.registerConfigGate(&app.getTimeGate());
 *
 * Regression tolerance is UINT32_MAX — once a timezone is configured,
 * it remains valid indefinitely (it doesn't "un-set" during operation).
 */
class TimeConfigGate : public IConfigGate {
public:
    /**
     * @brief Construct with a reference to the TimeManager to query.
     * @param tm Reference to the Application's TimeManager instance.
     */
    explicit TimeConfigGate(const TimeManager& tm) : _timeManager(tm) {}

    /**
     * @brief Check whether the user has explicitly configured a timezone.
     *
     * Returns true when /mod/time.json has been loaded by ConfigManager
     * and applyConfig() has set _configured = true. Returns false when
     * the device is still using the factory default timezone seeded by
     * resetConfig() ("GMT0" — no explicit user choice yet).
     *
     * @return true when timezone has been explicitly configured.
     */
    bool isSatisfied() const override { return _timeManager.isConfigured(); }

    /**
     * @brief Return the gate name for logging and display.
     * @return "Timezone"
     */
    const char* getGateName() const override { return GateNames::TIMEZONE; }

    /**
     * @brief Return user-facing help text.
     * @return Instructions for configuring the timezone.
     */
    const char* getHelpText() const override {
        return "Set your timezone via the Time section in the web portal";
    }

    /**
     * @brief Declare dependency on WiFi Station gate.
     *
     * Timezone can be configured during AP mode (from the cached timezone
     * database), but the gate ordering ensures WiFi is presented first
     * in the configuring status board.
     *
     * @param[out] count Set to 1.
     * @return Pointer to static array containing "WiFi Station".
     */
    const char* const* getDependencies(uint8_t& count) const override {
        static const char* deps[] = {GateNames::WIFI_STATION};
        count = 1;
        return deps;
    }

    /**
     * @brief Return regression tolerance.
     *
     * UINT32_MAX = never regress. Once a timezone is configured, it
     * remains valid indefinitely — the setting doesn't disappear at runtime.
     *
     * @return UINT32_MAX
     */
    uint32_t getRegressionToleranceMs() const override { return UINT32_MAX; }

private:
    const TimeManager& _timeManager; ///< Reference to TimeManager for config query
};
