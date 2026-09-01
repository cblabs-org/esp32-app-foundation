/**
 * @file ui_gen.h
 * Native test stub for LVGL Pro generated header
 */

#ifndef LVGL_PRO_UI_GEN_H
#define LVGL_PRO_UI_GEN_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Permanent screens & Styles
extern lv_style_t transparent_container;

// Fonts
extern lv_font_t * montserrat_12;
extern lv_font_t * montserrat_14;
extern lv_font_t * montserrat_16;
extern lv_font_t * montserrat_18;
extern lv_font_t * montserrat_20;
extern lv_font_t * montserrat_22;
extern lv_font_t * montserrat_40;
extern lv_font_t * fontawesome_20;
extern lv_font_t * fontawesome_22;

// Images
extern const void * img_cblabs_logo;

// Subjects
extern lv_subject_t sys_boot_progress;
extern lv_subject_t sys_boot_status;
extern lv_subject_t sys_app_title;
extern lv_subject_t sys_error_visible;
extern lv_subject_t sys_setup_visible;
extern lv_subject_t sys_ota_visible;
extern lv_subject_t sys_busy_visible;
extern lv_subject_t sys_msg_visible;
extern lv_subject_t sys_error_title;
extern lv_subject_t sys_error_message;
extern lv_subject_t sys_setup_ssid;
extern lv_subject_t sys_setup_url;
extern lv_subject_t sys_ota_status;
extern lv_subject_t sys_ota_progress;
extern lv_subject_t sys_ota_percent;
extern lv_subject_t sys_busy_message;
extern lv_subject_t sys_msg_title;
extern lv_subject_t sys_msg_body;
extern lv_subject_t sys_diag_arc_value;
extern lv_subject_t sys_diag_category;
extern lv_subject_t sys_diag_value;
extern lv_subject_t sys_diag_counter;
extern lv_subject_t sys_diag_status;

// Screen constructors
lv_obj_t * screen_boot_create(void);
lv_obj_t * screen_system_create(void);
lv_obj_t * screen_diag_create(void);

// Target functions
void ui_set_target(uint32_t target);
uint32_t ui_get_target(void);
bool ui_check_target(uint32_t target);

// Event callbacks
void ui_boot_fade_overlay_loaded(lv_event_t * e);
void ui_system_dismiss_clicked(lv_event_t * e);

// Gen init
void ui_init_gen(const char * asset_path);

#ifdef __cplusplus
}
#endif

#endif // LVGL_PRO_UI_GEN_H
