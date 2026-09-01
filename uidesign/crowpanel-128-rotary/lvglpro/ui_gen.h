/**
 * @file ui_gen.h
 */

#ifndef LVGL_PRO_UI_GEN_H
#define LVGL_PRO_UI_GEN_H

#ifndef UI_SUBJECT_STRING_LENGTH
#define UI_SUBJECT_STRING_LENGTH 256
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
    #include "lvgl_private.h"
#else
    #include "lvgl/lvgl.h"
    #include "lvgl/lvgl_private.h"
#endif

#if defined(LV_USE_XML) && LV_USE_XML
    #include "lv_xml/lv_xml.h"
#endif



/* Prototypes for target functions, needed by responsive const definitions */

void ui_set_target(uint32_t target);
uint32_t ui_get_target(void);
bool ui_check_target(uint32_t target);

/*********************
 *      DEFINES
 *********************/

#define UI_TARGET_UNDEFINED  (0 << 1)
#define UI_TARGET_DEFAULT    (1 << 1)
#define UI_TARGET_ALL        0x0FFFFFFF

/* By default compile for all targets, allowing to switch to any targets at runtime */
#ifndef UI_COMPILE_TARGET
#define UI_COMPILE_TARGET UI_TARGET_ALL
#endif

#define UI_CHECK_COMPILE_TARGET(target) (UI_COMPILE_TARGET & (target) ? 1 : 0)

#define BOOT_BG lv_color_hex(0xF5F5F5)
#define BOOT_MAROON lv_color_hex(0x7B2C29)
#define BOOT_BAR_TRACK lv_color_hex(0xDDD1D0)
#define BOOT_STATUS_TEXT lv_color_hex(0x7D7370)
#define SYSTEM_BG lv_color_hex(0x121220)
#define ERROR_RED lv_color_hex(0xE05040)
#define SETUP_GREEN lv_color_hex(0x3FB950)
#define OTA_BLUE lv_color_hex(0x2196F3)
#define MSG_ICON_BLUE lv_color_hex(0x6EA8FE)
#define CAPTION_GREY lv_color_hex(0x8B929D)
#define WHITE lv_color_hex(0xFFFFFF)
#define DIAG_BG lv_color_hex(0x0D1117)
#define DIAG_AMBER lv_color_hex(0xF0A020)
#define COUNTER_GREY lv_color_hex(0x5B6270)


#ifndef LV_XML_EVAL_STRING_BUF_SIZE
    #define LV_XML_EVAL_STRING_BUF_SIZE 256
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL VARIABLES
 **********************/

/*-------------------
 * Permanent screens
 *------------------*/

/*----------------
 * Global styles
 *----------------*/

extern lv_style_t transparent_container;

/*----------------
 * Fonts
 *----------------*/

/* Targets: any */
extern lv_font_t * montserrat_12;
extern lv_font_t * montserrat_14;
extern lv_font_t * montserrat_16;
extern lv_font_t * montserrat_18;
extern lv_font_t * montserrat_20;
extern lv_font_t * montserrat_22;
extern lv_font_t * montserrat_40;
extern lv_font_t * fontawesome_20;
extern lv_font_t * fontawesome_22;


/*----------------
 * Images
 *----------------*/

/* Targets: any */
extern const void * img_cblabs_logo;

/*----------------
 * Subjects
 *----------------*/

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

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/*----------------
 * Event Callbacks
 *----------------*/

void ui_boot_fade_overlay_loaded(lv_event_t * e);
void ui_system_dismiss_clicked(lv_event_t * e);

/**
 * Initialize the component library
 */

void ui_init_gen(const char * asset_path);

/**********************
 *      MACROS
 **********************/

/**********************
 *   POST INCLUDES
 **********************/

/*Include all the widgets, components and screens of this library*/
#include "screens/screen_boot_gen.h"
#include "screens/screen_diag_gen.h"
#include "screens/screen_system_gen.h"

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LVGL_PRO_UI_GEN_H*/