/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/ScreensaverController.cpp
 * Description:
 *   Three-state inactivity manager for the display subsystem.
 *   States: Active → Dimmed → Blanked (power-save).
 *
 *   The ScreensaverPolicy enum selects between DimOnly (reduce
 *   brightness only) and FullSleep (dim then power-save after a
 *   second timeout).  resetActivity() is the sole wake trigger.
 *
 *   This is a display-intrinsic concern — the application layer
 *   need only call resetActivity() on user interaction; the
 *   controller handles all state transitions internally.
 */

#include "ScreensaverController.hpp"
#include "platform/LogShim.h"

#include <U8g2lib.h>

static constexpr const char* TAG = "ScreensaverController";

/**
 * @brief Enable or disable the screensaver.
 *
 * When disabling while in a dimmed or blanked state, requests a wake
 * so the display restores on the next tick().
 *
 * @param enabled True to enable, false to disable.
 */
void ScreensaverController::setEnabled(bool enabled) {
    _enabled = enabled;
    if (!_enabled && (_dimmed || _blanked)) {
        _wakeRequested = true;
    }
}

/** @brief Returns true if the screensaver is enabled. */
bool ScreensaverController::isEnabled() const {
    return _enabled;
}

/** @brief Set inactivity timeout (ms) before the display enters dim state. */
void ScreensaverController::setDimTimeout(unsigned long ms) {
    _dimTimeoutMs = ms;
}

/** @brief Set inactivity timeout (ms) before full sleep (FullSleep policy only). */
void ScreensaverController::setBlankTimeout(unsigned long ms) {
    _blankTimeoutMs = ms;
}

/** @brief Set the brightness level used during dim state (0-255). */
void ScreensaverController::setDimLevel(uint8_t level) {
    _dimLevel = level;
}

/** @brief Select DimOnly or FullSleep screensaver policy. */
void ScreensaverController::setPolicy(ScreensaverPolicy policy) {
    _policy = policy;
}

/** @brief Assign the ScreensaverBoard displayed when the screensaver is active. */
void ScreensaverController::setBoard(ScreensaverBoard* board) {
    _board = board;
}

/**
 * @brief Reset the inactivity timer and request a wake if currently dimmed/blanked.
 *
 * Called by the application on any user interaction.  If the display is
 * in dim or blanked state, sets _wakeRequested so that the next tick()
 * restores full brightness.
 */
void ScreensaverController::resetActivity() {
    _lastActivityMs = millis();
    _diagLogged = false;
    
    if (_dimmed || _blanked) {
        _wakeRequested = true;
    }
}

/**
 * @brief Force an immediate timeout so the screensaver triggers on the next tick().
 *
 * Moves _lastActivityMs far enough into the past that the blank-timeout
 * threshold is already exceeded.
 */
void ScreensaverController::forceTimeout() {
    // Set last activity far in the past to trigger timeout on next tick
    _lastActivityMs = millis() - (_blankTimeoutMs + 1);
}

/** @brief Returns true if the screensaver is in any non-active state (dimmed or blanked). */
bool ScreensaverController::isActive() const {
    return _dimmed || _blanked;
}

/** @brief Returns true if the display is currently in dim state. */
bool ScreensaverController::isDimmed() const {
    return _dimmed;
}

/** @brief Returns true if the display is in power-save blanked state. */
bool ScreensaverController::isBlanked() const {
    return _blanked;
}

/** @brief Return the assigned ScreensaverBoard (may be nullptr). */
ScreensaverBoard* ScreensaverController::board() const {
    return _board;
}

/**
 * @brief Main loop tick — drives the Active → Dimmed → Blanked state machine.
 *
 * Checks elapsed time since last activity and transitions through states.
 * A pending _wakeRequested (from resetActivity()) is serviced first.
 *
 * @param currentMs    Current time in milliseconds (e.g. millis()).
 * @param display      Reference to the U8G2 driver.
 * @param activeBrightness Brightness to restore on wake.
 */
void ScreensaverController::tick(unsigned long currentMs, U8G2& display, uint8_t activeBrightness) {
    // --- Service pending wake requests first (even if disabled) ---
    // setEnabled(false) while dimmed/blanked sets _wakeRequested so the
    // display restores immediately; this MUST be checked before the
    // !_enabled early-return.
    if (_wakeRequested) {
        _wake(display, activeBrightness);
        _wakeRequested = false;
        return;
    }

    if (_dimTimeoutMs == 0) {
        return; // Screensaver disabled (zero timeout)
    }

    if (!_enabled) {
        return; // Screensaver disabled (master switch)
    }

    unsigned long elapsed = currentMs - _lastActivityMs;

    if (!_diagLogged && elapsed >= (_dimTimeoutMs / 2)) {
        ESP_LOGW(TAG, "Sleep diagnostic: elapsed=%lu timeout=%lu (50%% reached)",
                 elapsed, _dimTimeoutMs);
        _diagLogged = true;
    }

    if (!_dimmed && elapsed >= _dimTimeoutMs) {
        _dim(display);
    } else if (_dimmed && !_blanked && _policy == ScreensaverPolicy::FullSleep && elapsed >= _blankTimeoutMs) {
        _blank(display);
    }
}

/**
 * @brief Transition to dim state — lower contrast and activate the screensaver board.
 */
void ScreensaverController::_dim(U8G2& display) {
    ESP_LOGI(TAG, "Display dimmed (inactivity)");
    display.setContrast(_dimLevel);
    _dimmed = true;
    
    if (_board) {
        _board->activate();
    }
}

/**
 * @brief Transition to blanked state — enable hardware power-save on the display.
 */
void ScreensaverController::_blank(U8G2& display) {
    // Use hardware power-save (setPowerSave(1) = display controller sleep).
    // This is the correct approach — it deenergises the OLED segments and
    // shuts down the DC-DC boost converter, unlike clearBuffer/sendBuffer
    // which only visually blanks while the controller keeps running.
    //
    // Previously this crashed (TWDT) but the root cause was the tight main
    // loop after blanking starving IDLE1 — not the SPI command itself.
    // The vTaskDelay(33ms) in U8G2DisplayModule::tick() fixes that.
    ESP_LOGI(TAG, "Display blanked (entering power save)");
    display.setPowerSave(1);
    _blanked = true;
}



/**
 * @brief Wake from any screensaver state — restore brightness and deactivate the board.
 */
void ScreensaverController::_wake(U8G2& display, uint8_t activeBrightness) {
    ESP_LOGI(TAG, "Display waking from sleep");
    display.setPowerSave(0);  // Wake from hardware sleep before restoring contrast
    display.setContrast(activeBrightness);
    _dimmed = false;
    _blanked = false;

    if (_board) {
        _board->deactivate();
    }
}
