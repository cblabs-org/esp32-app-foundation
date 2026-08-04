/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/LVGLDisplay/screens/OTAProgressScreen.hpp
 * Description: Built-in OTA update progress screen.
 *              Displays update status, progress bar, and percentage label.
 *
 * Exported Classes:
 * - OTAProgressScreen: Managed built-in OTA progress screen LVGL object container
 */

#ifndef MODULES_LVGLDISPLAY_SCREENS_OTAPROGRESSSCREEN_HPP_
#define MODULES_LVGLDISPLAY_SCREENS_OTAPROGRESSSCREEN_HPP_

#include "lvgl.h"

/**
 * @brief Managed container for built-in LVGL OTA progress screen.
 */
class OTAProgressScreen {
public:
    OTAProgressScreen() = default;
    ~OTAProgressScreen();

    OTAProgressScreen(const OTAProgressScreen&) = delete;
    OTAProgressScreen& operator=(const OTAProgressScreen&) = delete;

    /**
     * @brief Initialize LVGL widget hierarchy for OTA progress screen.
     */
    void init();

    /**
     * @brief Update OTA progress and status text.
     *
     * @param progress Fractional progress in [0.0, 1.0] (or 0-100%).
     * @param status   Null-terminated status phase description.
     */
    void setProgress(float progress, const char* status);

    /**
     * @brief Get root LVGL screen object pointer.
     * @return lv_obj_t* pointer, or nullptr if not initialized.
     */
    lv_obj_t* getScreen() const { return _screen; }

private:
    lv_obj_t* _screen = nullptr;
    lv_obj_t* _titleLabel = nullptr;
    lv_obj_t* _statusLabel = nullptr;
    lv_obj_t* _progressBar = nullptr;
    lv_obj_t* _percentLabel = nullptr;
};

#endif  // MODULES_LVGLDISPLAY_SCREENS_OTAPROGRESSSCREEN_HPP_
