/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/LVGLDisplay/ILVGLSystemScreens.hpp
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
 *   - onOtaProgress():  Called when OTA progress state updates
 *   - onError():        Called when an error state is reported
 *   - onConfiguring():  Called on each CONFIGURING re-publish to populate configuring widgets
 */

#ifndef LVGLDISPLAY_ILVGLSYSTEMSCREENS_HPP_
#define LVGLDISPLAY_ILVGLSYSTEMSCREENS_HPP_

#include "lvgl.h"
#include "events/AppConfiguringPayload.hpp"
#include "events/PreconditionHealthPayload.hpp"
#include "interfaces/IDataSource.hpp"

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
     * @brief Create or retrieve the boot screen root object.
     *
     * Returned during the BOOT phase so the framework can drive boot progress
     * onto the designer's boot screen (via onBootProgress()). Because boot runs
     * before most of the framework is up, the skin's screens must already exist
     * when this is first called (the module builds the SLS UI in bootstrap).
     *
     * @return Pointer to the boot screen, or nullptr to use the built-in FaultScreen.
     */
    virtual lv_obj_t* bootScreen() { return nullptr; }

    /**
     * @brief Set the application title displayed on the boot screen.
     *
     * Called by the framework after Application::getAppName() is available.
     * Default no-op for backward compatibility with custom skin providers.
     *
     * @param title The application title string (lifetime managed by caller).
     */
    virtual void setAppTitle(const char* title) {
        (void)title;
    }

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
     * @brief Callback notification when boot progress updates.
     *
     * Called on each boot status report so the provider can populate the boot
     * screen's progress bar and status label. Mirrors onOtaProgress()/onError().
     *
     * @param message  Boot status text (e.g. "Initializing WiFi...").
     * @param progress Fractional progress [0.0, 1.0] (or [0, 100]); negative = indeterminate.
     */
    virtual void onBootProgress(const char* message, float progress) {
        (void)message;
        (void)progress;
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

    /**
     * @brief Callback notification on each CONFIGURING re-publish.
     *
     * Called by the display module before it selects the configuring root via
     * configuringScreen() / wifiWizardScreen() / gateHelperScreen(), so the
     * provider can populate its configuring widgets (AP SSID, config-portal URL,
     * gate status/help text) from the payload. Mirrors onOtaProgress()/onError().
     *
     * @param payload Full configuring event payload (gate name, help text,
     *                action status, hostname/SSID, device IP). Never null when
     *                called by the framework.
     */
    virtual void onConfiguring(const AppConfiguringPayload* payload) {
        (void)payload;
    }

    /**
     * @brief Callback notification on each precondition health update.
     *
     * Called by the display module when a PreconditionHealthPayload event is received.
     *
     * @param payload Full precondition health payload (name, cause, severity, everMet).
     */
    virtual void onPreconditionHealth(const PreconditionHealthPayload* payload) {
        (void)payload;
    }

    /**
     * @brief Called when a data source's health state changes.
     * @param name Name of the data source.
     * @param state New health state.
     */
    virtual void onDataSourceHealth(const char* name, SourceHealthState state) {
        (void)name;
        (void)state;
    }
};

#endif  // LVGLDISPLAY_ILVGLSYSTEMSCREENS_HPP_
