/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * @file ILVGLHardwareAdapter.hpp
 * @brief Hardware adaptation interface for LVGL display drivers
 * @author Matthew McNeill
 * 
 * Module: modules/LVGLDisplay/ILVGLHardwareAdapter.hpp
 * Project: esp32-app-foundation
 * 
 * This interface isolates the LVGLDisplay module from the specific details of
 * the underlying display and touch hardware. It is called during the LVGL boot
 * sequence to initialize the hardware, register drivers with LVGL, and wire up
 * flush and input read callbacks.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Exported Classes:
 * - ILVGLHardwareAdapter: Abstract hardware adapter interface for LVGL displays
 *
 *   Required Methods:
 *   - createDisplay():       Initialise display hardware and return lv_display_t pointer
 *   - setBrightness():       Set display backlight brightness level (0-255)
 *   - getScreenWidth():      Return screen resolution width in pixels
 *   - getScreenHeight():     Return screen resolution height in pixels
 *   - getBufferLineCount():  Return recommended draw buffer height in scan lines
 *
 *   Optional Input Methods (default nullptr):
 *   - createTouchInput():    Initialise touch controller and return pointer indev (default nullptr)
 *   - createEncoderInput():  Initialise rotary encoder and return encoder indev (default nullptr)
 *   - createKeypadInput():   Initialise keypad/buttons and return keypad indev (default nullptr)
 */

#ifndef INCLUDE_INTERFACES_ILVGLHARDWAREADAPTER_HPP_
#define INCLUDE_INTERFACES_ILVGLHARDWAREADAPTER_HPP_ ///< Include guard

#include <cstdint>
#include "lvgl.h"

/**
 * @brief Hardware adapter interface for LVGL display modules.
 *
 * Implement this interface to provide hardware-specific initialisation
 * for a particular display controller, touch panel, and input peripherals.
 * The generic LVGLDisplayModule consumes this interface without needing knowledge
 * of underlying display drivers or bus configurations.
 */
class ILVGLHardwareAdapter {
public:
    /** @brief Destructor for ILVGLHardwareAdapter. */
    virtual ~ILVGLHardwareAdapter() = default;

    // --- Required Interface Methods ---

    /**
     * @brief Initialise display hardware and create an LVGL display instance.
     *
     * @return Fully configured lv_display_t pointer with flush_cb wired.
     */
    virtual lv_display_t* createDisplay() = 0;

    /**
     * @brief Set display backlight brightness level.
     *
     * @param level Brightness from 0 (completely off) to 255 (maximum brightness).
     */
    virtual void setBrightness(uint8_t level) = 0;

    /**
     * @brief Get horizontal screen resolution in pixels.
     *
     * @return Width in pixels.
     */
    virtual uint16_t getScreenWidth() const = 0;

    /**
     * @brief Get vertical screen resolution in pixels.
     *
     * @return Height in pixels.
     */
    virtual uint16_t getScreenHeight() const = 0;

    /**
     * @brief Recommended scan line count for partial draw buffers.
     *
     * @return Line count per buffer chunk (e.g., 24 lines).
     */
    virtual uint16_t getBufferLineCount() const = 0;

    // --- Optional Interface Methods (Default nullptr) ---

    /**
     * @brief Initialise a touch/pointer input device.
     *
     * @return lv_indev_t pointer configured as LV_INDEV_TYPE_POINTER, or nullptr if unsupported.
     */
    virtual lv_indev_t* createTouchInput() { return nullptr; }

    /**
     * @brief Initialise a rotary encoder input device.
     *
     * @return lv_indev_t pointer configured as LV_INDEV_TYPE_ENCODER, or nullptr if unsupported.
     */
    virtual lv_indev_t* createEncoderInput() { return nullptr; }

    /**
     * @brief Initialise a keypad or navigation button input device.
     *
     * @return lv_indev_t pointer configured as LV_INDEV_TYPE_KEYPAD, or nullptr if unsupported.
     */
    virtual lv_indev_t* createKeypadInput() { return nullptr; }

    /**
     * @brief Poll for touch activity and publish EventBus events.
     *
     * Called by LVGLDisplayModule on every tick — including when the screensaver
     * has blanked the display (when LVGL's own input polling is suspended).
     * Implementations should detect touch-start transitions and publish
     * "touch.press_down" on the EventBus so the display module can wake.
     *
     * Default implementation does nothing (no touch hardware).
     */
    virtual void pollTouch() {}
};

#endif  // INCLUDE_INTERFACES_ILVGLHARDWAREADAPTER_HPP_
