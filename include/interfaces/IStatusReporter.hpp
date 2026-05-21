/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/interfaces/IStatusReporter.hpp
 * Description: Pure interface allowing the Application hub to report
 *              boot-phase status (progress bars, error messages) to
 *              any attached display module without importing its header.
 *              When no display is registered, Application uses a silent
 *              NullStatusReporter no-op (added in PR-03).
 *
 * Exported Classes:
 * - IStatusReporter: Abstract base for display status overlays
 *   - reportStatus(): Display a one-line status string and optional progress
 *   - reportError(): Display a fatal error string and halt indicator
 *   - clearStatus(): Remove any active status overlay
 */

#pragma once

/**
 * @brief Contract for display modules providing boot-phase status overlays.
 *
 * The Application hub calls these methods during boot sequencing to show
 * progress without knowing the concrete display implementation.
 */
class IStatusReporter {
public:
    virtual ~IStatusReporter() = default;

    /**
     * @brief Report a status string with optional progress fraction.
     * @param message  Null-terminated status string (max ~40 chars recommended).
     * @param progress Fractional progress in [0.0, 1.0]; negative = indeterminate.
     */
    virtual void reportStatus(const char* message, float progress = -1.0f) = 0;

    /**
     * @brief Report a fatal error and display a halt indicator.
     * @param errorMessage Null-terminated error description.
     */
    virtual void reportError(const char* errorMessage) = 0;

    /**
     * @brief Remove any active status overlay from the display.
     */
    virtual void clearStatus() = 0;
};
