/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/interfaces/ILVGLSystemScreens.hpp
 * Description: Interface for pluggable system-screen skin providers in LVGLDisplayModule.
 *              Allows applications to supply custom visual screens for OTA updates
 *              and error states while retaining framework lifecycle handling.
 *
 * Exported Classes:
 * - ILVGLSystemScreens: Abstract system screen provider interface for LVGL displays
 *
 *   Optional Screen Factory Methods (default nullptr):
 *   - otaScreen():    Return custom LVGL object tree for OTA progress screen
 *   - errorScreen():  Return custom LVGL object tree for error display screen
 *
 *   Optional Event Callback Methods (default no-op):
 *   - onOtaProgress(): Called when OTA progress state updates
 *   - onError():       Called when an error state is reported
 */

#ifndef INCLUDE_INTERFACES_ILVGLSYSTEMSCREENS_HPP_
#define INCLUDE_INTERFACES_ILVGLSYSTEMSCREENS_HPP_

#include "lvgl.h"

/**
 * @brief Pluggable system screen visual provider interface for LVGL displays.
 *
 * Applications can implement this interface to substitute custom UI object trees
 * for framework system states (OTA progress, system errors) instead of the default screens.
 */
class ILVGLSystemScreens {
public:
    virtual ~ILVGLSystemScreens() = default;

    /**
     * @brief Create or retrieve custom OTA progress screen root object.
     *
     * @return Pointer to custom lv_obj_t screen container, or nullptr to use default screen.
     */
    virtual lv_obj_t* otaScreen() { return nullptr; }

    /**
     * @brief Create or retrieve custom error screen root object.
     *
     * @return Pointer to custom lv_obj_t screen container, or nullptr to use default screen.
     */
    virtual lv_obj_t* errorScreen() { return nullptr; }

    /**
     * @brief Callback notification when OTA progress updates.
     *
     * @param progress Progress ratio from 0.0f to 100.0f (or 0.0f to 1.0f).
     * @param status   Status text summary.
     */
    virtual void onOtaProgress(float progress, const char* status) {
        (void)progress;
        (void)status;
    }

    /**
     * @brief Callback notification when system error updates.
     *
     * @param title   Short error summary title.
     * @param message Detailed error description text.
     */
    virtual void onError(const char* title, const char* message) {
        (void)title;
        (void)message;
    }
};

#endif  // INCLUDE_INTERFACES_ILVGLSYSTEMSCREENS_HPP_
