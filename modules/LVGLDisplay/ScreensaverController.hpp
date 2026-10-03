/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/LVGLDisplay/ScreensaverController.hpp
 * Description: Three-state inactivity manager for the LVGL display subsystem.
 *              States: Active → Dimmed → Blanked (power-save).
 *
 * Exported Classes:
 * - ScreensaverController: Manages display inactivity timeouts, dimming, and sleep.
 */

#ifndef MODULES_LVGLDISPLAY_SCREENSAVERCONTROLLER_HPP_
#define MODULES_LVGLDISPLAY_SCREENSAVERCONTROLLER_HPP_ ///< Include guard

#include <cstdint>

// Forward declarations
class ILVGLHardwareAdapter;
class EventBus;

/**
 * @brief Controls screensaver behaviour on inactivity timeout.
 *
 * DimOnly:   Reduce brightness to minimum level; display remains powered on.
 * FullSleep: Dim first, then set brightness to 0 and enter sleep after second timeout.
 */
enum class ScreensaverPolicy : uint8_t {
    DimOnly,   ///< Reduce brightness to minimum level only
    FullSleep  ///< Dim then cut power/brightness after second timeout
};

/**
 * @brief Manages inactivity dimming and power-save sleep for LVGL display.
 */
class ScreensaverController {
public:
    /**
     * @brief Enable or disable the screensaver.
     *
     * @param enabled True to enable, false to disable.
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check whether the screensaver is enabled.
     * @return True if enabled.
     */
    bool isEnabled() const;

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
     * @brief Get inactivity timeout before dimming.
     * @return Timeout in milliseconds.
     */
    unsigned long getDimTimeout() const { return _dimTimeoutMs; }

    /**
     * @brief Get inactivity timeout before full sleep (FullSleep only).
     * @return Timeout in milliseconds.
     */
    unsigned long getBlankTimeout() const { return _blankTimeoutMs; }

    /**
     * @brief Set dimmed brightness level.
     * @param level Brightness level (0-255).
     */
    void setDimLevel(uint8_t level);

    /**
     * @brief Get dimmed brightness level.
     * @return Brightness level (0-255).
     */
    uint8_t getDimLevel() const { return _dimLevel; }

    /**
     * @brief Set dim-only vs full-sleep policy.
     * @param policy The screensaver policy to use.
     */
    void setPolicy(ScreensaverPolicy policy);

    /**
     * @brief Get current screensaver policy.
     * @return Current policy.
     */
    ScreensaverPolicy getPolicy() const { return _policy; }

    /**
     * @brief Reset inactivity timer; wake if sleeping or dimmed.
     */
    void resetActivity();

    /**
     * @brief Force an immediate timeout (sleep on next tick).
     */
    void forceTimeout();

    /**
     * @brief Check if screensaver is in any non-active state (dimmed or blanked).
     * @return True if active.
     */
    bool isActive() const;

    /**
     * @brief Check if display is in dimmed state.
     * @return True if dimmed.
     */
    bool isDimmed() const;

    /**
     * @brief Check if display is in power-save blanked state.
     * @return True if blanked.
     */
    bool isBlanked() const;

    /**
     * @brief Main loop tick — drives the Active → Dimmed → Blanked state machine.
     *
     * @param currentMs        Current time in milliseconds (e.g. millis()).
     * @param adapter          Reference to hardware adapter for brightness control.
     * @param activeBrightness Active (non-dimmed) brightness level to restore on wake.
     * @param eventBus         Optional EventBus pointer for publishing sleep/wake events.
     */
    void tick(unsigned long currentMs, ILVGLHardwareAdapter& adapter, uint8_t activeBrightness, EventBus* eventBus = nullptr);

private:
    bool              _enabled = true;                   ///< Master enable flag
    unsigned long     _dimTimeoutMs = 150000;            ///< Inactivity timeout before dimming (2.5 min default)
    unsigned long     _blankTimeoutMs = 300000;          ///< Inactivity timeout before blanking (5 min default)
    uint8_t           _dimLevel = 1;                     ///< Brightness level during dim state
    ScreensaverPolicy _policy = ScreensaverPolicy::FullSleep; ///< Dim-only vs full-sleep policy
    unsigned long     _lastActivityMs = 0;              ///< Timestamp of last user interaction
    bool              _initialized = false;             ///< True once tick() initialized activity timestamp
    bool              _dimmed = false;                   ///< True when display is dimmed
    bool              _blanked = false;                  ///< True when display is blanked
    bool              _wakeRequested = false;            ///< True when wake requested for next tick

    void _dim(ILVGLHardwareAdapter& adapter, uint8_t activeBrightness);
    void _blank(ILVGLHardwareAdapter& adapter, EventBus* eventBus);
    void _wake(ILVGLHardwareAdapter& adapter, uint8_t activeBrightness, EventBus* eventBus);
};

#endif  // MODULES_LVGLDISPLAY_SCREENSAVERCONTROLLER_HPP_
