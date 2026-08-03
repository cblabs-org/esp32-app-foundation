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

#include "interfaces/ILVGLSystemScreens.hpp"

/**
 * @brief SquareLine Studio system screen skin provider.
 *
 * Bridges SLS-exported widget globals (ui_uiSystemScreen, ui_uiSystemErrorGroup,
 * ui_uiSystemOTAGroup, etc.) to the framework ILVGLSystemScreens interface.
 */
class SLSSystemScreens : public ILVGLSystemScreens {
public:
    SLSSystemScreens() = default;
    ~SLSSystemScreens() override = default;

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

private:
    /**
     * @brief Hide all container visibility groups on ui_uiSystemScreen.
     */
    void hideAllGroups();
};

#endif  // MODULES_LVGLDISPLAY_SCREENS_SLSSYSTEMSCREENS_HPP_
