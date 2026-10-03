/**
 * @file screen_boot_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen_boot_gen.h"
#include "../ui.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/

/***********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * screen_boot_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    lv_obj_t * the_root = NULL;

    #if UI_CHECK_COMPILE_TARGET(UI_TARGET_ALL)
    if (ui_check_target(UI_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "screen_boot_#");
        lv_obj_set_width(lv_obj_0, lv_pct(100));
        lv_obj_set_height(lv_obj_0, lv_pct(100));
        lv_obj_set_style_bg_color(lv_obj_0, lv_color_hex(0xF5F5F5), 0);
        lv_obj_set_style_bg_opa(lv_obj_0, 255, 0);
        lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);

        lv_obj_t * boot_logo = lv_image_create(lv_obj_0);
        lv_obj_set_name(boot_logo, "boot_logo");
        lv_image_set_src(boot_logo, img_cblabs_logo);
        lv_obj_set_align(boot_logo, LV_ALIGN_CENTER);
        lv_obj_set_x(boot_logo, 0);
        lv_obj_set_y(boot_logo, -40);
        lv_obj_set_width(boot_logo, 180);
        lv_obj_set_height(boot_logo, 58);

        lv_obj_t * boot_title = lv_label_create(lv_obj_0);
        lv_obj_set_name(boot_title, "boot_title");
        lv_label_bind_text(boot_title, &sys_app_title, NULL);
        lv_label_set_text(boot_title, "ESP32 App Foundation");
        lv_obj_set_align(boot_title, LV_ALIGN_CENTER);
        lv_obj_set_x(boot_title, 0);
        lv_obj_set_y(boot_title, 20);
        lv_obj_set_width(boot_title, 193);
        lv_obj_set_height(boot_title, 48);
        lv_label_set_long_mode(boot_title, LV_LABEL_LONG_MODE_WRAP);
        lv_obj_set_style_text_font(boot_title, montserrat_18, 0);
        lv_obj_set_style_text_color(boot_title, lv_color_hex(0x7B2C29), 0);
        lv_obj_set_style_text_align(boot_title, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_t * boot_progress_bar = lv_bar_create(lv_obj_0);
        lv_obj_set_name(boot_progress_bar, "boot_progress_bar");
        lv_bar_bind_value(boot_progress_bar, &sys_boot_progress);
        lv_bar_set_min_value(boot_progress_bar, 0);
        lv_bar_set_max_value(boot_progress_bar, 100);
        lv_obj_set_align(boot_progress_bar, LV_ALIGN_CENTER);
        lv_obj_set_x(boot_progress_bar, 0);
        lv_obj_set_y(boot_progress_bar, 52);
        lv_obj_set_width(boot_progress_bar, 140);
        lv_obj_set_height(boot_progress_bar, 8);
        lv_obj_set_style_bg_color(boot_progress_bar, lv_color_hex(0xDDD1D0), 0);
        lv_obj_set_style_bg_color(boot_progress_bar, lv_color_hex(0x7B2C29), LV_PART_INDICATOR);

        lv_obj_t * boot_status_label = lv_label_create(lv_obj_0);
        lv_obj_set_name(boot_status_label, "boot_status_label");
        lv_label_bind_text(boot_status_label, &sys_boot_status, NULL);
        lv_label_set_text(boot_status_label, "Initialising...");
        lv_obj_set_align(boot_status_label, LV_ALIGN_CENTER);
        lv_obj_set_x(boot_status_label, 0);
        lv_obj_set_y(boot_status_label, 74);
        lv_obj_set_width(boot_status_label, LV_SIZE_CONTENT);
        lv_obj_set_height(boot_status_label, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(boot_status_label, montserrat_12, 0);
        lv_obj_set_style_text_color(boot_status_label, lv_color_hex(0x7D7370), 0);

        lv_obj_t * boot_fade_overlay = lv_obj_create(lv_obj_0);
        lv_obj_set_name(boot_fade_overlay, "boot_fade_overlay");
        lv_obj_set_align(boot_fade_overlay, LV_ALIGN_CENTER);
        lv_obj_set_x(boot_fade_overlay, 0);
        lv_obj_set_y(boot_fade_overlay, 0);
        lv_obj_set_width(boot_fade_overlay, 240);
        lv_obj_set_height(boot_fade_overlay, 240);
        lv_obj_set_style_bg_opa(boot_fade_overlay, 0, 0);
        lv_obj_set_style_border_width(boot_fade_overlay, 0, 0);
        lv_obj_set_style_pad_all(boot_fade_overlay, 0, 0);
        lv_obj_set_flag(boot_fade_overlay, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_add_event_cb(boot_fade_overlay, ui_boot_fade_overlay_loaded, LV_EVENT_READY, NULL);

        the_root = lv_obj_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

