/**
 * @file screen_diag_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen_diag_gen.h"
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

lv_obj_t * screen_diag_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    lv_obj_t * the_root = NULL;

    #if UI_CHECK_COMPILE_TARGET(UI_TARGET_ALL)
    if (ui_check_target(UI_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "screen_diag_#");
        lv_obj_set_width(lv_obj_0, lv_pct(100));
        lv_obj_set_height(lv_obj_0, lv_pct(100));
        lv_obj_set_style_bg_color(lv_obj_0, lv_color_hex(0x0D1117), 0);
        lv_obj_set_style_bg_opa(lv_obj_0, 255, 0);
        lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);

        lv_obj_t * diag_arc = lv_arc_create(lv_obj_0);
        lv_obj_set_name(diag_arc, "diag_arc");
        lv_arc_bind_value(diag_arc, &sys_diag_arc_value);
        lv_arc_set_value(diag_arc, 0);
        lv_arc_set_min_value(diag_arc, 0);
        lv_arc_set_max_value(diag_arc, 100);
        lv_arc_set_bg_start_angle(diag_arc, 135);
        lv_arc_set_bg_end_angle(diag_arc, 45);
        lv_arc_set_mode(diag_arc, LV_ARC_MODE_NORMAL);
        lv_obj_set_align(diag_arc, LV_ALIGN_CENTER);
        lv_obj_set_x(diag_arc, 0);
        lv_obj_set_y(diag_arc, 0);
        lv_obj_set_width(diag_arc, 216);
        lv_obj_set_height(diag_arc, 216);
        lv_obj_set_style_arc_color(diag_arc, lv_color_hex(0xF0A020), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(diag_arc, 0, LV_PART_KNOB);
        lv_obj_set_style_border_width(diag_arc, 0, LV_PART_KNOB);
        lv_obj_set_flag(diag_arc, LV_OBJ_FLAG_SCROLLABLE, false);

        lv_obj_t * diag_category = lv_label_create(lv_obj_0);
        lv_obj_set_name(diag_category, "diag_category");
        lv_label_bind_text(diag_category, &sys_diag_category, NULL);
        lv_label_set_text(diag_category, "TEMPERATURE");
        lv_obj_set_align(diag_category, LV_ALIGN_CENTER);
        lv_obj_set_x(diag_category, 0);
        lv_obj_set_y(diag_category, -58);
        lv_obj_set_width(diag_category, LV_SIZE_CONTENT);
        lv_obj_set_height(diag_category, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(diag_category, montserrat_12, 0);
        lv_obj_set_style_text_color(diag_category, lv_color_hex(0x8B929D), 0);

        lv_obj_t * diag_counter = lv_label_create(lv_obj_0);
        lv_obj_set_name(diag_counter, "diag_counter");
        lv_label_bind_text(diag_counter, &sys_diag_counter, NULL);
        lv_label_set_text(diag_counter, "3 / 6");
        lv_obj_set_align(diag_counter, LV_ALIGN_CENTER);
        lv_obj_set_x(diag_counter, 0);
        lv_obj_set_y(diag_counter, -36);
        lv_obj_set_width(diag_counter, LV_SIZE_CONTENT);
        lv_obj_set_height(diag_counter, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(diag_counter, montserrat_12, 0);
        lv_obj_set_style_text_color(diag_counter, lv_color_hex(0x5B6270), 0);

        lv_obj_t * diag_value = lv_label_create(lv_obj_0);
        lv_obj_set_name(diag_value, "diag_value");
        lv_label_bind_text(diag_value, &sys_diag_value, NULL);
        lv_label_set_text(diag_value, "78°C");
        lv_obj_set_align(diag_value, LV_ALIGN_CENTER);
        lv_obj_set_x(diag_value, 0);
        lv_obj_set_y(diag_value, -2);
        lv_obj_set_width(diag_value, LV_SIZE_CONTENT);
        lv_obj_set_height(diag_value, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(diag_value, montserrat_40, 0);
        lv_obj_set_style_text_color(diag_value, lv_color_hex(0xFFFFFF), 0);

        lv_obj_t * diag_status = lv_label_create(lv_obj_0);
        lv_obj_set_name(diag_status, "diag_status");
        lv_label_bind_text(diag_status, &sys_diag_status, NULL);
        lv_label_set_text(diag_status, "HIGH");
        lv_obj_set_align(diag_status, LV_ALIGN_CENTER);
        lv_obj_set_x(diag_status, 0);
        lv_obj_set_y(diag_status, 36);
        lv_obj_set_width(diag_status, LV_SIZE_CONTENT);
        lv_obj_set_height(diag_status, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(diag_status, montserrat_14, 0);
        lv_obj_set_style_text_color(diag_status, lv_color_hex(0xF0A020), 0);

        the_root = lv_obj_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

