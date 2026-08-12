/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/LVGLDisplay/screens/SLSSystemScreens.hpp
 * Description: Implementation of ILVGLSystemScreens interface for SquareLine Studio
 *              exported UI screens. Maps framework lifecycle events (OTA progress, system
 *              errors) to SLS-exported widget visibility groups and labels.
 *
 * Exported Classes:
 * - SLSSystemScreens: Pluggable ILVGLSystemScreens provider for SLS exported UI
 */

#ifndef MODULES_LVGLDISPLAY_SCREENS_SLSSYSTEMSCREENS_HPP_
#define MODULES_LVGLDISPLAY_SCREENS_SLSSYSTEMSCREENS_HPP_

#include "ILVGLSystemScreens.hpp"

/**
 * @brief SquareLine Studio system screen skin provider.
 *
 * Bridges SLS-exported widget globals to the framework ILVGLSystemScreens
 * interface. The System screen (ui_uiSystemScreen) hosts visibility groups —
 * Error (ui_uiSystemErrorGroup), Setup (ui_uiSystemSetupGroup), OTA
 * (ui_uiSystemOTAGroup), Busy (ui_uiSystemBusyGroup), and Message
 * (ui_uiSystemMessageGroup) — shown one at a time via hideAllGroups().
 */
class SLSSystemScreens : public ILVGLSystemScreens {
public:
    SLSSystemScreens() = default;
    ~SLSSystemScreens() override = default;

    /**
     * @brief Retrieve the SLS boot screen root object.
     * @return Pointer to ui_uiBootScreen (nullptr if the UI failed to load).
     */
    lv_obj_t* bootScreen() override;

    /**
     * @brief Update the boot screen's progress bar and status label.
     * @param message  Boot status text.
     * @param progress Fractional progress [0.0, 1.0] or [0, 100]; negative = leave bar unchanged.
     */
    void onBootProgress(const char* message, float progress) override;

    /**
     * @brief Retrieve error screen root object with error visibility group active.
     * @return Pointer to ui_uiSystemScreen.
     */
    lv_obj_t* errorScreen() override;

    /**
     * @brief Retrieve OTA screen root object with OTA visibility group active.
     * @return Pointer to ui_uiSystemScreen.
     */
    lv_obj_t* otaScreen() override;

    /**
     * @brief Callback notification when system error occurs. Updates error labels.
     * @param title   Short error title string.
     * @param message Detailed error description.
     */
    void onError(const char* title, const char* message) override;

    /**
     * @brief Callback notification when OTA progress updates. Updates progress bar and text.
     * @param progress Progress ratio in [0.0, 100.0] or [0.0, 1.0].
     * @param status   Status text summary.
     */
    void onOtaProgress(float progress, const char* status) override;

    /**
     * @brief Base configuring screen — activates the Busy group (indeterminate wait).
     * @return Pointer to ui_uiSystemScreen.
     */
    lv_obj_t* configuringScreen() override;

    /**
     * @brief WiFi wizard screen — activates the Setup group (AP SSID + portal URL).
     * @return Pointer to ui_uiSystemScreen.
     */
    lv_obj_t* wifiWizardScreen() override;

    /**
     * @brief Generic gate-helper modal — activates the Message group.
     *
     * Returns nullptr for the WiFi station gate so the module's Tier-2 WiFi wizard
     * (wifiWizardScreen) handles it; otherwise shows the Message group.
     *
     * @param payload Configuring payload (gate name selects behaviour).
     * @return Pointer to ui_uiSystemScreen, or nullptr to defer to the WiFi wizard.
     */
    lv_obj_t* gateHelperScreen(const AppConfiguringPayload* payload) override;

    /**
     * @brief Populate configuring widgets from the payload on each re-publish.
     *
     * Sets the Setup SSID/URL, Busy message, and Message title/body from the
     * payload. Visibility is chosen separately by the screen-factory methods.
     * @param payload Configuring event payload (hostname, deviceIP, gateName, helpText).
     */
    void onConfiguring(const AppConfiguringPayload* payload) override;

private:
    /**
     * @brief Hide all container visibility groups on ui_uiSystemScreen.
     */
    void hideAllGroups();

    /**
     * @brief Hide all groups then reveal one, returning the System screen root.
     * @param group Visibility-group container to show (may be nullptr).
     * @return Pointer to ui_uiSystemScreen.
     */
    lv_obj_t* showGroup(lv_obj_t* group);
};

#endif  // MODULES_LVGLDISPLAY_SCREENS_SLSSYSTEMSCREENS_HPP_
