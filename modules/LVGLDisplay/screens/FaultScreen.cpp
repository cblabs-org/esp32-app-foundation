/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/LVGLDisplay/screens/FaultScreen.cpp
 * Description: Implementation of the single built-in fault/trace screen.
 */

#if __has_include("screens/FaultScreen.hpp")
#include "screens/FaultScreen.hpp"
#else
#include "FaultScreen.hpp"
#endif

FaultScreen::~FaultScreen() {
    if (_screen) {
        lv_obj_delete(_screen);
        _screen = nullptr;
    }
}

void FaultScreen::init() {
    if (_screen) {
        return;
    }

    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_black(), LV_PART_MAIN);

    _titleLabel = lv_label_create(_screen);
    lv_label_set_text(_titleLabel, "DISPLAY FAULT");
    lv_obj_set_style_text_color(_titleLabel, lv_color_hex(0xE04040), LV_PART_MAIN);
    lv_obj_align(_titleLabel, LV_ALIGN_CENTER, 0, -14);

    _detailLabel = lv_label_create(_screen);
    lv_label_set_long_mode(_detailLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(_detailLabel, 200);
    lv_label_set_text(_detailLabel, "No UI screens loaded");
    lv_obj_set_style_text_color(_detailLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(_detailLabel, LV_ALIGN_CENTER, 0, 14);
}

void FaultScreen::setFault(const char* title, const char* detail) {
    if (!_screen) {
        init();
    }
    if (title && _titleLabel) {
        lv_label_set_text(_titleLabel, title);
    }
    if (detail && _detailLabel) {
        lv_label_set_text(_detailLabel, detail);
    }
}
