/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/LVGLDisplay/screens/OTAProgressScreen.cpp
 * Description: Implementation of built-in OTA update progress screen.
 */

#if __has_include("screens/OTAProgressScreen.hpp")
#include "screens/OTAProgressScreen.hpp"
#else
#include "OTAProgressScreen.hpp"
#endif

OTAProgressScreen::~OTAProgressScreen() {
    if (_screen) {
        lv_obj_delete(_screen);
        _screen = nullptr;
    }
}

void OTAProgressScreen::init() {
    if (_screen) {
        return;
    }

    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_black(), LV_PART_MAIN);

    _titleLabel = lv_label_create(_screen);
    lv_label_set_text(_titleLabel, "Firmware Update");
    lv_obj_set_style_text_color(_titleLabel, lv_color_make(100, 200, 255), LV_PART_MAIN);
    lv_obj_align(_titleLabel, LV_ALIGN_CENTER, 0, -40);

    _statusLabel = lv_label_create(_screen);
    lv_label_set_text(_statusLabel, "Updating...");
    lv_obj_set_style_text_color(_statusLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(_statusLabel, LV_ALIGN_CENTER, 0, -10);

    _progressBar = lv_bar_create(_screen);
    lv_obj_set_size(_progressBar, 180, 12);
    lv_bar_set_range(_progressBar, 0, 100);
    lv_bar_set_value(_progressBar, 0, LV_ANIM_OFF);
    lv_obj_align(_progressBar, LV_ALIGN_CENTER, 0, 15);

    _percentLabel = lv_label_create(_screen);
    lv_label_set_text(_percentLabel, "0%");
    lv_obj_set_style_text_color(_percentLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(_percentLabel, LV_ALIGN_CENTER, 0, 40);
}

void OTAProgressScreen::setProgress(float progress, const char* status) {
    if (!_screen) {
        init();
    }
    if (status) {
        lv_label_set_text(_statusLabel, status);
    }
    int32_t val = (progress <= 1.0f) ? static_cast<int32_t>(progress * 100.0f)
                                     : static_cast<int32_t>(progress);
    if (val > 100) val = 100;
    if (val < 0) val = 0;
    lv_bar_set_value(_progressBar, val, LV_ANIM_OFF);
    lv_label_set_text_fmt(_percentLabel, "%d%%", static_cast<int>(val));
}
