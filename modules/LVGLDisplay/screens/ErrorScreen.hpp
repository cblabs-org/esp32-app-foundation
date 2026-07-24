/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/LVGLDisplay/screens/ErrorScreen.hpp
 * Description: Built-in system error display screen.
 *              Displays error title and error message using native LVGL widgets.
 *
 * Exported Classes:
 * - ErrorScreen: Managed built-in error screen LVGL object container
 */

#ifndef MODULES_LVGLDISPLAY_SCREENS_ERRORSCREEN_HPP_
#define MODULES_LVGLDISPLAY_SCREENS_ERRORSCREEN_HPP_

#include "lvgl.h"

/**
 * @brief Managed container for built-in LVGL error notification screen.
 */
class ErrorScreen {
public:
    ErrorScreen() = default;
    ~ErrorScreen();

    ErrorScreen(const ErrorScreen&) = delete;
    ErrorScreen& operator=(const ErrorScreen&) = delete;

    /**
     * @brief Initialize LVGL widget hierarchy for error screen.
     */
    void init();

    /**
     * @brief Set error title and detailed message.
     *
     * @param title   Short error summary.
     * @param message Detailed error description.
     */
    void setError(const char* title, const char* message);

    /**
     * @brief Get root LVGL screen object pointer.
     * @return lv_obj_t* pointer, or nullptr if not initialized.
     */
    lv_obj_t* getScreen() const { return _screen; }

private:
    lv_obj_t* _screen = nullptr;
    lv_obj_t* _titleLabel = nullptr;
    lv_obj_t* _msgLabel = nullptr;
};

#endif  // MODULES_LVGLDISPLAY_SCREENS_ERRORSCREEN_HPP_
