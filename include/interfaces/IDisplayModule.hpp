/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/interfaces/IDisplayModule.hpp
 * Description: Unified interface for display provider modules.
 *              Extends IBootReporter for early hardware init and provides
 *              contracts for geometry, brightness, screensaver state,
 *              system state overlays, and PNG screenshot capture.
 *
 * Exported Classes:
 * - DisplayState: Well-known system display state tags.
 * - SystemDisplayParams: Parameters for system state overlays.
 * - IDisplayModule: Abstract base for display modules.
 */

#pragma once

#include "interfaces/IBootReporter.hpp"
#include <stdint.h>

class Print;

/**
 * @brief Well-known system display state tags.
 *
 * Framework modules use these constants; downstream consumers
 * define their own tags without editing this header.
 */
namespace DisplayState {
    constexpr const char* Normal      = "normal";       ///< Clear overlay
    constexpr const char* Error       = "error";        ///< Fatal error
    constexpr const char* Configuring = "configuring";  ///< Gate orchestration
}

/**
 * @brief Parameters for IDisplayModule::showSystemState().
 */
struct SystemDisplayParams {
    const char* state   = DisplayState::Normal;  ///< State tag
    const char* title   = nullptr;               ///< Primary text
    const char* message = nullptr;               ///< Secondary/detail text
    float progress      = -1.0f;                 ///< [0.0, 1.0]; negative=indeterminate
};

/**
 * @brief Unified interface for display modules.
 *
 * Extends IBootReporter to support early boot splashes, adding runtime
 * contracts for brightness, geometry, screenshots, and system overlays.
 */
class IDisplayModule : public IBootReporter {
public:
    /** @brief Destructor for IDisplayModule. */
    virtual ~IDisplayModule() = default;

    // --- Geometry ---

    /**
     * @brief Get the physical width of the display in pixels.
     * @return Width in pixels.
     */
    virtual uint16_t getDisplayWidth() const = 0;

    /**
     * @brief Get the physical height of the display in pixels.
     * @return Height in pixels.
     */
    virtual uint16_t getDisplayHeight() const = 0;

    // --- Brightness ---

    /**
     * @brief Set the backlight brightness level.
     * @param level Brightness level (0 = off, 255 = max).
     */
    virtual void setBrightness(uint8_t level) = 0;

    /**
     * @brief Get the current backlight brightness level.
     * @return Brightness level (0-255).
     */
    virtual uint8_t getBrightness() const = 0;

    // --- Screensaver / Activity ---

    /**
     * @brief Reset the screensaver activity timer.
     *
     * Wakes up the display if sleeping/dimmed and resets the timeout.
     */
    virtual void resetActivity() = 0;

    /**
     * @brief Check if the display is currently in a dimmed screensaver state.
     * @return true if dimmed, false otherwise.
     */
    virtual bool isDimmed() const = 0;

    /**
     * @brief Check if the display backlight is currently completely off (sleeping).
     * @return true if sleeping, false otherwise.
     */
    virtual bool isSleeping() const = 0;

    // --- Screenshot ---

    /**
     * @brief Capture the current framebuffer to a PNG image.
     *
     * Implementations must be safe to call from any task. LVGL implementations
     * should snapshot the framebuffer under the LVGL lock and encode from the copy.
     * Returns false if the display is uninitialized, the framebuffer cannot be
     * acquired, or encoding fails. On failure, partial data may have been
     * written to sink. Callers should discard sink contents if this method returns false.
     *
     * @param sink The Print stream to write the PNG file to.
     * @return true on successful capture and encoding; false otherwise.
     */
    virtual bool capturePNG(Print& sink) = 0;

    /**
     * @brief Capture the current framebuffer to a stored (uncompressed) PNG image.
     *
     * @param sink The Print stream to write the PNG file to.
     * @return true on successful capture; false otherwise.
     */
    virtual bool captureStoredPNG(Print& sink) = 0;

    // --- System Lifecycle Display ---

    /**
     * @brief Show a full-screen system state overlay.
     *
     * Implementations MUST handle DisplayState::Normal, Error, and Configuring.
     * Unknown state tags should display a generic message overlay or no-op.
     *
     * @param params Parameters for the state overlay (title, message, progress).
     */
    virtual void showSystemState(const SystemDisplayParams& params) = 0;
};
