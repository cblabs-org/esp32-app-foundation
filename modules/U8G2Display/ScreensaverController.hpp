/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/ScreensaverController.hpp
 * Description: Extracts screensaver / sleep / inactivity logic.
 *
 * Exported Classes:
 * - ScreensaverController: Manages display inactivity timeouts, dimming, and sleep.
 */

#pragma once

#include <cstdint>
#include "boards/ScreensaverBoard.hpp"

class U8G2;

/**
 * @brief Controls screensaver behaviour on inactivity timeout.
 *
 * DimOnly:   Reduce brightness to minimum; display remains powered on.
 * FullSleep: Dim first, then power-save the display (setPowerSave(1)).
 */
enum class ScreensaverPolicy : uint8_t {
    DimOnly,   // Dim the display brightness to minimum level only
    FullSleep  // Dim then cut power-save after second timeout
};

/**
 * @brief Manages inactivity dimming and power-save sleep for the display.
 */
class ScreensaverController {
public:
    /**
     * @brief Set inactivity timeout before dimming.
     * @param ms Timeout in milliseconds.
     */
    void setDimTimeout(unsigned long ms);

    /**
     * @brief Set inactivity timeout before full sleep (FullSleep only).
     * @param ms Timeout in milliseconds.
     */
    void setBlankTimeout(unsigned long ms);

    /**
     * @brief Get inactivity timeout before full sleep (FullSleep only).
     * @return ms Timeout in milliseconds.
     */
    unsigned long getBlankTimeout() const { return _blankTimeoutMs; }

    /**
     * @brief Set dimmed brightness level.
     * @param level Brightness level (0-255).
     */
    void setDimLevel(uint8_t level);

    /**
     * @brief Set dim-only vs full-sleep policy.
     * @param policy The screensaver policy to use.
     */
    void setPolicy(ScreensaverPolicy policy);

    /**
     * @brief Set the screensaver board to display when blanked.
     * @param board Pointer to the screensaver board.
     */
    void setBoard(IDisplayBoard* board);

    /**
     * @brief Reset inactivity timer; wake if sleeping.
     */
    void resetActivity();

    /**
     * @brief Force an immediate timeout (sleep on next tick).
     */
    void forceTimeout();

    /**
     * @brief Check if screensaver is currently displayed.
     * @return True if active.
     */
    bool isActive() const;

    /**
     * @brief Check if display is in dim state.
     * @return True if dimmed.
     */
    bool isDimmed() const;

    /**
     * @brief Check if display is in power-save sleep state.
     * @return True if blanked.
     */
    bool isBlanked() const;

    /**
     * @brief Tick - call once per main-loop iteration.
     * @param currentMs Current time in milliseconds.
     * @param display Reference to the U8G2 driver.
     * @param activeBrightness The active (non-dimmed) brightness level to restore on wake.
     */
    void tick(unsigned long currentMs, U8G2& display, uint8_t activeBrightness);

    /**
     * @brief Return the screensaver board (for rendering).
     * @return Pointer to the board.
     */
    IDisplayBoard* board() const;

private:
    unsigned long _dimTimeoutMs  = 300000;   ///< Inactivity timeout before dimming (5 min default)
    unsigned long _blankTimeoutMs = 600000;  ///< Inactivity timeout before power-save (10 min default)
    uint8_t  _dimLevel = 1;                  ///< Brightness level applied during dim state (1 = near-off)
    ScreensaverPolicy _policy = ScreensaverPolicy::FullSleep; ///< Dim-only vs full-sleep policy
    IDisplayBoard* _board = nullptr;         ///< The screensaver board displayed when blanked
    unsigned long _lastActivityMs = 0;       ///< Timestamp of last user interaction
    bool _dimmed = false;                    ///< True when display is in dim state
    bool _blanked = false;                   ///< True when display is in power-save sleep
    bool _diagLogged = false;                ///< One-shot flag for sleep timer diagnostic log
    bool _wakeRequested = false;             ///< True when resetActivity() requires a wake during next tick()

    void _dim(U8G2& display);    // Enter dim state - reduce contrast, set _dimmed
    void _blank(U8G2& display);  // Enter blank/sleep state - setPowerSave(1), set _blanked
    void _wake(U8G2& display, uint8_t activeBrightness);   // Exit all screensaver states - restore contrast, setPowerSave(0)
};
