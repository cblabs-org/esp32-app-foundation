/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/LVGLDisplay/screens/BootScreen.cpp
 * Description: Implementation of built-in Stage-1 boot splash screen.
 */

#if __has_include("screens/BootScreen.hpp")
#include "screens/BootScreen.hpp"
#else
#include "BootScreen.hpp"
#endif

BootScreen::BootScreen(const lv_image_dsc_t* bootLogo, const char* appTitle)
    : _bootLogo(bootLogo ? bootLogo : &cb_labs_boot_logo),
      _appTitle(appTitle) {}

BootScreen::~BootScreen() {
    if (_screen) {
        lv_obj_delete(_screen);
        _screen = nullptr;
    }
}

void BootScreen::init(const lv_image_dsc_t* bootLogo, const char* appTitle) {
    if (bootLogo) {
        _bootLogo = bootLogo;
    }
    if (appTitle) {
        _appTitle = appTitle;
    }
    if (_screen) {
        if (_appTitle && _titleLabel) {
            lv_label_set_text(_titleLabel, _appTitle);
        }
        return;
    }

    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_black(), LV_PART_MAIN);

    _logoImg = lv_image_create(_screen);
    if (_bootLogo) {
        lv_image_set_src(_logoImg, _bootLogo);
    }
    lv_obj_align(_logoImg, LV_ALIGN_CENTER, 0, -45);

    _titleLabel = lv_label_create(_screen);
    lv_label_set_text(_titleLabel, _appTitle ? _appTitle : "ESP32 App Foundation");
    lv_obj_set_style_text_color(_titleLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(_titleLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(_titleLabel, LV_ALIGN_CENTER, 0, -10);

    _statusLabel = lv_label_create(_screen);
    lv_label_set_text(_statusLabel, "Starting...");
    lv_obj_set_style_text_color(_statusLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(_statusLabel, LV_ALIGN_CENTER, 0, 18);

    _progressBar = lv_bar_create(_screen);
    lv_obj_set_size(_progressBar, 160, 8);
    lv_bar_set_range(_progressBar, 0, 100);
    lv_bar_set_value(_progressBar, 0, LV_ANIM_OFF);
    lv_obj_align(_progressBar, LV_ALIGN_CENTER, 0, 45);
}

void BootScreen::setStatus(const char* message, float progress) {
    if (!_screen) {
        init();
    }
    if (message) {
        lv_label_set_text(_statusLabel, message);
    }
    if (progress >= 0.0f) {
        int32_t val = (progress <= 1.0f) ? static_cast<int32_t>(progress * 100.0f)
                                         : static_cast<int32_t>(progress);
        if (val > 100) val = 100;
        lv_bar_set_value(_progressBar, val, LV_ANIM_OFF);
    }
}

void BootScreen::setBootLogo(const lv_image_dsc_t* bootLogo) {
    if (bootLogo) {
        _bootLogo = bootLogo;
        if (_logoImg) {
            lv_image_set_src(_logoImg, _bootLogo);
        }
    }
}

void BootScreen::setAppTitle(const char* appTitle) {
    _appTitle = appTitle;
    if (_titleLabel) {
        lv_label_set_text(_titleLabel, _appTitle ? _appTitle : "ESP32 App Foundation");
    }
}
