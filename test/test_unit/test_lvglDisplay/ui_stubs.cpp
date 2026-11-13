/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_lvglDisplay/ui_stubs.cpp
 * Description: Weak-symbol stubs for LVGL Pro UI globals, subjects, and screen constructors
 *              for native unit tests.
 */

#include "lvgl.h"

#if defined(__GNUC__) || defined(__clang__)
#define WEAK_SYMBOL __attribute__((weak))
#else
#define WEAK_SYMBOL
#endif

extern "C" {

// Subjects
WEAK_SYMBOL lv_subject_t sys_boot_progress;
WEAK_SYMBOL lv_subject_t sys_boot_status;
WEAK_SYMBOL lv_subject_t sys_app_title;
WEAK_SYMBOL lv_subject_t sys_error_visible;
WEAK_SYMBOL lv_subject_t sys_setup_visible;
WEAK_SYMBOL lv_subject_t sys_ota_visible;
WEAK_SYMBOL lv_subject_t sys_busy_visible;
WEAK_SYMBOL lv_subject_t sys_msg_visible;
WEAK_SYMBOL lv_subject_t sys_error_title;
WEAK_SYMBOL lv_subject_t sys_error_message;
WEAK_SYMBOL lv_subject_t sys_setup_ssid;
WEAK_SYMBOL lv_subject_t sys_setup_url;
WEAK_SYMBOL lv_subject_t sys_ota_status;
WEAK_SYMBOL lv_subject_t sys_ota_progress;
WEAK_SYMBOL lv_subject_t sys_ota_percent;
WEAK_SYMBOL lv_subject_t sys_busy_message;
WEAK_SYMBOL lv_subject_t sys_msg_title;
WEAK_SYMBOL lv_subject_t sys_msg_body;
WEAK_SYMBOL lv_subject_t sys_diag_arc_value;
WEAK_SYMBOL lv_subject_t sys_diag_category;
WEAK_SYMBOL lv_subject_t sys_diag_value;
WEAK_SYMBOL lv_subject_t sys_diag_counter;
WEAK_SYMBOL lv_subject_t sys_diag_status;

// Fonts & Assets
WEAK_SYMBOL const void * img_cblabs_logo = NULL;
WEAK_SYMBOL lv_font_t * montserrat_12 = NULL;
WEAK_SYMBOL lv_font_t * montserrat_14 = NULL;
WEAK_SYMBOL lv_font_t * montserrat_16 = NULL;
WEAK_SYMBOL lv_font_t * montserrat_18 = NULL;
WEAK_SYMBOL lv_font_t * montserrat_20 = NULL;
WEAK_SYMBOL lv_font_t * montserrat_22 = NULL;
WEAK_SYMBOL lv_font_t * montserrat_40 = NULL;
WEAK_SYMBOL lv_font_t * fontawesome_20 = NULL;
WEAK_SYMBOL lv_font_t * fontawesome_22 = NULL;
WEAK_SYMBOL lv_style_t transparent_container;

// Screen constructors
WEAK_SYMBOL lv_obj_t * screen_boot_create(void) { return lv_obj_create(nullptr); }
WEAK_SYMBOL lv_obj_t * screen_system_create(void) { return lv_obj_create(nullptr); }
WEAK_SYMBOL lv_obj_t * screen_diag_create(void) { return lv_obj_create(nullptr); }

// UI Init / callbacks
WEAK_SYMBOL void ui_init(const char * asset_path) { (void)asset_path; }
WEAK_SYMBOL void ui_init_gen(const char * asset_path) { (void)asset_path; }
WEAK_SYMBOL void ui_set_target(uint32_t target) { (void)target; }
WEAK_SYMBOL uint32_t ui_get_target(void) { return 0; }
WEAK_SYMBOL bool ui_check_target(uint32_t target) { (void)target; return true; }
WEAK_SYMBOL void ui_boot_fade_overlay_loaded(lv_event_t * e) { (void)e; }
WEAK_SYMBOL void ui_system_dismiss_clicked(lv_event_t * e) { (void)e; }

}  // extern "C"
