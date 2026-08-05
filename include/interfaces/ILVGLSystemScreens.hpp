/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/interfaces/ILVGLSystemScreens.hpp
 * Description: Interface for pluggable system-screen skin providers in LVGLDisplayModule.
 *              Allows applications to supply custom visual screens for OTA updates,
 *              error states, and CONFIGURING-phase gate orchestration while retaining
 *              framework lifecycle handling.
 *
 * Exported Classes:
 * - ILVGLSystemScreens: Abstract system screen provider interface for LVGL displays
 *
 *   Optional Screen Factory Methods (default nullptr):
 *   - otaScreen():           Return custom LVGL object tree for OTA progress screen
 *   - errorScreen():         Return custom LVGL object tree for error display screen
 *   - configuringScreen():   Return custom LVGL screen for CONFIGURING base underlay
 *   - wifiWizardScreen():    Return custom LVGL screen for WiFi wizard modal
 *   - gateHelperScreen():    Return custom LVGL screen for gate-specific helper modal
 *
 *   Optional Event Callback Methods (default no-op):
 *   - onOtaProgress(): Called when OTA progress state updates
 *   - onError():       Called when an error state is reported
 */

#ifndef INCLUDE_INTERFACES_ILVGLSYSTEMSCREENS_HPP_
#define INCLUDE_INTERFACES_ILVGLSYSTEMSCREENS_HPP_

#include "lvgl.h"
#include "events/AppConfiguringPayload.hpp"

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
     * @brief Create or retrieve custom configuring base screen.
     *
     * @return Pointer to custom lv_obj_t screen container, or nullptr to use default screen.
     */
    virtual lv_obj_t* configuringScreen() { return nullptr; }

    /**
     * @brief Create or retrieve custom WiFi wizard screen.
     *
     * @return Pointer to custom lv_obj_t screen container, or nullptr to use default screen.
     */
    virtual lv_obj_t* wifiWizardScreen() { return nullptr; }

    /**
     * @brief Custom helper modal screen for a specific config gate.
     *
     * Called by the display module when a gate needs user action.
     * Return nullptr to use the framework's built-in mapping or generic
     * helpText fallback.
     *
     * Pattern matches IU8G2SystemScreens::gateHelperBoard() — same opt-in
     * override model with the same ownership semantics.
     *
     * The payload pointer references Application::_configuringPayload, a member
     * variable valid for the Application's entire lifetime. The payload is stable
     * for the duration of the CONFIGURING phase and is updated in-place on each
     * re-publish.
     *
     * @param payload  Full configuring event payload with gate name,
     *                 help text, action status, hostname, and device IP.
     * @return Pointer to custom lv_obj_t screen, or nullptr.
     */
    virtual lv_obj_t* gateHelperScreen(const AppConfiguringPayload* payload) {
        (void)payload;
        return nullptr;
    }

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
