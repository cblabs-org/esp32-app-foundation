/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/LVGLDisplay/screens/ErrorScreen.cpp
 * Description: Implementation of built-in system error display screen.
 */

#if __has_include("screens/ErrorScreen.hpp")
#include "screens/ErrorScreen.hpp"
#else
#include "ErrorScreen.hpp"
#endif

ErrorScreen::~ErrorScreen() {
    if (_screen) {
        lv_obj_delete(_screen);
        _screen = nullptr;
    }
}

void ErrorScreen::init() {
    if (_screen) {
        return;
    }

    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_black(), LV_PART_MAIN);

    _titleLabel = lv_label_create(_screen);
    lv_label_set_text(_titleLabel, "SYSTEM ERROR");
    lv_obj_set_style_text_color(_titleLabel, lv_color_make(255, 60, 60), LV_PART_MAIN);
    lv_obj_align(_titleLabel, LV_ALIGN_CENTER, 0, -20);

    _msgLabel = lv_label_create(_screen);
    lv_label_set_text(_msgLabel, "An unknown error occurred");
    lv_obj_set_style_text_color(_msgLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(_msgLabel, LV_ALIGN_CENTER, 0, 20);
}

void ErrorScreen::setError(const char* title, const char* message) {
    if (!_screen) {
        init();
    }
    if (title) {
        lv_label_set_text(_titleLabel, title);
    }
    if (message) {
        lv_label_set_text(_msgLabel, message);
    }
}
