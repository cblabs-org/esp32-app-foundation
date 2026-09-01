/**
 * @file screen_system_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen_system_gen.h"
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

lv_obj_t * screen_system_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    lv_obj_t * the_root = NULL;

    #if UI_CHECK_COMPILE_TARGET(UI_TARGET_ALL)
    if (ui_check_target(UI_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "screen_system_#");
        lv_obj_set_width(lv_obj_0, lv_pct(100));
        lv_obj_set_height(lv_obj_0, lv_pct(100));
        lv_obj_set_style_bg_color(lv_obj_0, lv_color_hex(0x121220), 0);
        lv_obj_set_style_bg_opa(lv_obj_0, 255, 0);
        lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);

        lv_obj_t * system_error_group = lv_obj_create(lv_obj_0);
        lv_obj_set_name(system_error_group, "system_error_group");
        lv_obj_set_align(system_error_group, LV_ALIGN_CENTER);
        lv_obj_set_x(system_error_group, 0);
        lv_obj_set_y(system_error_group, 0);
        lv_obj_set_width(system_error_group, 160);
        lv_obj_set_height(system_error_group, 160);
        lv_obj_set_style_bg_opa(system_error_group, 0, 0);
        lv_obj_set_style_border_width(system_error_group, 0, 0);
        lv_obj_set_style_pad_all(system_error_group, 0, 0);
        lv_obj_set_flag(system_error_group, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_bind_flag_if_eq(system_error_group, &sys_error_visible, LV_OBJ_FLAG_HIDDEN, 0);
        lv_obj_t * error_icon = lv_label_create(system_error_group);
        lv_obj_set_name(error_icon, "error_icon");
        lv_label_set_text(error_icon, "");
        lv_obj_set_align(error_icon, LV_ALIGN_CENTER);
        lv_obj_set_x(error_icon, 0);
        lv_obj_set_y(error_icon, -60);
        lv_obj_set_width(error_icon, LV_SIZE_CONTENT);
        lv_obj_set_height(error_icon, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(error_icon, montserrat_22, 0);
        lv_obj_set_style_text_color(error_icon, lv_color_hex(0xE05040), 0);

        lv_obj_t * error_title = lv_label_create(system_error_group);
        lv_obj_set_name(error_title, "error_title");
        lv_label_bind_text(error_title, &sys_error_title, NULL);
        lv_label_set_text(error_title, "System Error");
        lv_obj_set_align(error_title, LV_ALIGN_CENTER);
        lv_obj_set_x(error_title, 0);
        lv_obj_set_y(error_title, -34);
        lv_obj_set_width(error_title, LV_SIZE_CONTENT);
        lv_obj_set_height(error_title, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(error_title, montserrat_16, 0);
        lv_obj_set_style_text_color(error_title, lv_color_hex(0xE05040), 0);

        lv_obj_t * error_message = lv_label_create(system_error_group);
        lv_obj_set_name(error_message, "error_message");
        lv_label_bind_text(error_message, &sys_error_message, NULL);
        lv_label_set_text(error_message, "WiFi lost. Retrying...  reboot or open the setup portal.");
        lv_obj_set_align(error_message, LV_ALIGN_CENTER);
        lv_obj_set_x(error_message, 0);
        lv_obj_set_y(error_message, 5);
        lv_obj_set_width(error_message, 157);
        lv_obj_set_height(error_message, 42);
        lv_label_set_long_mode(error_message, LV_LABEL_LONG_MODE_WRAP);
        lv_obj_set_style_text_font(error_message, montserrat_12, 0);
        lv_obj_set_style_text_color(error_message, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_align(error_message, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_t * system_dismiss_btn = lv_button_create(system_error_group);
        lv_obj_set_name(system_dismiss_btn, "system_dismiss_btn");
        lv_obj_set_align(system_dismiss_btn, LV_ALIGN_CENTER);
        lv_obj_set_x(system_dismiss_btn, 0);
        lv_obj_set_y(system_dismiss_btn, 58);
        lv_obj_set_width(system_dismiss_btn, 84);
        lv_obj_set_height(system_dismiss_btn, 34);
        lv_obj_t * dismiss_btn_label = lv_label_create(system_dismiss_btn);
        lv_obj_set_name(dismiss_btn_label, "dismiss_btn_label");
        lv_label_set_text(dismiss_btn_label, "Dismiss");
        lv_obj_set_align(dismiss_btn_label, LV_ALIGN_CENTER);
        lv_obj_set_x(dismiss_btn_label, 0);
        lv_obj_set_y(dismiss_btn_label, 0);
        lv_obj_set_width(dismiss_btn_label, LV_SIZE_CONTENT);
        lv_obj_set_height(dismiss_btn_label, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(dismiss_btn_label, montserrat_12, 0);
        lv_obj_set_style_text_color(dismiss_btn_label, lv_color_hex(0xFFFFFF), 0);

        lv_obj_add_event_cb(system_dismiss_btn, ui_system_dismiss_clicked, LV_EVENT_CLICKED, NULL);

        lv_obj_t * system_setup_group = lv_obj_create(lv_obj_0);
        lv_obj_set_name(system_setup_group, "system_setup_group");
        lv_obj_set_align(system_setup_group, LV_ALIGN_CENTER);
        lv_obj_set_x(system_setup_group, 0);
        lv_obj_set_y(system_setup_group, 0);
        lv_obj_set_width(system_setup_group, 160);
        lv_obj_set_height(system_setup_group, 160);
        lv_obj_set_style_bg_opa(system_setup_group, 0, 0);
        lv_obj_set_style_border_width(system_setup_group, 0, 0);
        lv_obj_set_style_pad_all(system_setup_group, 0, 0);
        lv_obj_set_flag(system_setup_group, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_bind_flag_if_eq(system_setup_group, &sys_setup_visible, LV_OBJ_FLAG_HIDDEN, 0);
        lv_obj_t * setup_icon = lv_label_create(system_setup_group);
        lv_obj_set_name(setup_icon, "setup_icon");
        lv_label_set_text(setup_icon, "");
        lv_obj_set_align(setup_icon, LV_ALIGN_CENTER);
        lv_obj_set_x(setup_icon, 0);
        lv_obj_set_y(setup_icon, -62);
        lv_obj_set_width(setup_icon, LV_SIZE_CONTENT);
        lv_obj_set_height(setup_icon, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(setup_icon, montserrat_20, 0);
        lv_obj_set_style_text_color(setup_icon, lv_color_hex(0x3FB950), 0);

        lv_obj_t * setup_title = lv_label_create(system_setup_group);
        lv_obj_set_name(setup_title, "setup_title");
        lv_label_set_text(setup_title, "WiFi Setup");
        lv_obj_set_align(setup_title, LV_ALIGN_CENTER);
        lv_obj_set_x(setup_title, 0);
        lv_obj_set_y(setup_title, -38);
        lv_obj_set_width(setup_title, LV_SIZE_CONTENT);
        lv_obj_set_height(setup_title, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(setup_title, montserrat_16, 0);
        lv_obj_set_style_text_color(setup_title, lv_color_hex(0xFFFFFF), 0);

        lv_obj_t * setup_ssid_caption = lv_label_create(system_setup_group);
        lv_obj_set_name(setup_ssid_caption, "setup_ssid_caption");
        lv_label_set_text(setup_ssid_caption, "SSID");
        lv_obj_set_align(setup_ssid_caption, LV_ALIGN_CENTER);
        lv_obj_set_x(setup_ssid_caption, 0);
        lv_obj_set_y(setup_ssid_caption, -12);
        lv_obj_set_width(setup_ssid_caption, LV_SIZE_CONTENT);
        lv_obj_set_height(setup_ssid_caption, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(setup_ssid_caption, montserrat_12, 0);
        lv_obj_set_style_text_color(setup_ssid_caption, lv_color_hex(0x8B929D), 0);

        lv_obj_t * setup_ssid = lv_label_create(system_setup_group);
        lv_obj_set_name(setup_ssid, "setup_ssid");
        lv_label_bind_text(setup_ssid, &sys_setup_ssid, NULL);
        lv_label_set_text(setup_ssid, "CBLabs-Setup");
        lv_obj_set_align(setup_ssid, LV_ALIGN_CENTER);
        lv_obj_set_x(setup_ssid, 0);
        lv_obj_set_y(setup_ssid, 6);
        lv_obj_set_width(setup_ssid, LV_SIZE_CONTENT);
        lv_obj_set_height(setup_ssid, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(setup_ssid, montserrat_14, 0);
        lv_obj_set_style_text_color(setup_ssid, lv_color_hex(0xFFFFFF), 0);

        lv_obj_t * setup_url_caption = lv_label_create(system_setup_group);
        lv_obj_set_name(setup_url_caption, "setup_url_caption");
        lv_label_set_text(setup_url_caption, "URL");
        lv_obj_set_align(setup_url_caption, LV_ALIGN_CENTER);
        lv_obj_set_x(setup_url_caption, 0);
        lv_obj_set_y(setup_url_caption, 30);
        lv_obj_set_width(setup_url_caption, LV_SIZE_CONTENT);
        lv_obj_set_height(setup_url_caption, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(setup_url_caption, montserrat_12, 0);
        lv_obj_set_style_text_color(setup_url_caption, lv_color_hex(0x8B929D), 0);

        lv_obj_t * setup_url = lv_label_create(system_setup_group);
        lv_obj_set_name(setup_url, "setup_url");
        lv_label_bind_text(setup_url, &sys_setup_url, NULL);
        lv_label_set_text(setup_url, "192.168.4.1");
        lv_obj_set_align(setup_url, LV_ALIGN_CENTER);
        lv_obj_set_x(setup_url, 0);
        lv_obj_set_y(setup_url, 48);
        lv_obj_set_width(setup_url, LV_SIZE_CONTENT);
        lv_obj_set_height(setup_url, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(setup_url, montserrat_14, 0);
        lv_obj_set_style_text_color(setup_url, lv_color_hex(0xFFFFFF), 0);

        lv_obj_t * system_ota_group = lv_obj_create(lv_obj_0);
        lv_obj_set_name(system_ota_group, "system_ota_group");
        lv_obj_set_align(system_ota_group, LV_ALIGN_CENTER);
        lv_obj_set_x(system_ota_group, 0);
        lv_obj_set_y(system_ota_group, 0);
        lv_obj_set_width(system_ota_group, 160);
        lv_obj_set_height(system_ota_group, 160);
        lv_obj_set_style_bg_opa(system_ota_group, 0, 0);
        lv_obj_set_style_border_width(system_ota_group, 0, 0);
        lv_obj_set_style_pad_all(system_ota_group, 0, 0);
        lv_obj_set_flag(system_ota_group, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_bind_flag_if_eq(system_ota_group, &sys_ota_visible, LV_OBJ_FLAG_HIDDEN, 0);
        lv_obj_t * ota_title = lv_label_create(system_ota_group);
        lv_obj_set_name(ota_title, "ota_title");
        lv_label_set_text(ota_title, "Firmware Update");
        lv_obj_set_align(ota_title, LV_ALIGN_CENTER);
        lv_obj_set_x(ota_title, 0);
        lv_obj_set_y(ota_title, -38);
        lv_obj_set_width(ota_title, LV_SIZE_CONTENT);
        lv_obj_set_height(ota_title, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(ota_title, montserrat_16, 0);
        lv_obj_set_style_text_color(ota_title, lv_color_hex(0x2196F3), 0);

        lv_obj_t * ota_status = lv_label_create(system_ota_group);
        lv_obj_set_name(ota_status, "ota_status");
        lv_label_bind_text(ota_status, &sys_ota_status, NULL);
        lv_label_set_text(ota_status, "Downloading...");
        lv_obj_set_align(ota_status, LV_ALIGN_CENTER);
        lv_obj_set_x(ota_status, 0);
        lv_obj_set_y(ota_status, -12);
        lv_obj_set_width(ota_status, LV_SIZE_CONTENT);
        lv_obj_set_height(ota_status, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(ota_status, montserrat_12, 0);
        lv_obj_set_style_text_color(ota_status, lv_color_hex(0xFFFFFF), 0);

        lv_obj_t * ota_bar = lv_bar_create(system_ota_group);
        lv_obj_set_name(ota_bar, "ota_bar");
        lv_bar_bind_value(ota_bar, &sys_ota_progress);
        lv_bar_set_min_value(ota_bar, 0);
        lv_bar_set_max_value(ota_bar, 100);
        lv_obj_set_align(ota_bar, LV_ALIGN_CENTER);
        lv_obj_set_x(ota_bar, 0);
        lv_obj_set_y(ota_bar, 14);
        lv_obj_set_width(ota_bar, 130);
        lv_obj_set_height(ota_bar, 10);
        lv_obj_set_style_bg_color(ota_bar, lv_color_hex(0x2196F3), LV_PART_INDICATOR);

        lv_obj_t * ota_percent = lv_label_create(system_ota_group);
        lv_obj_set_name(ota_percent, "ota_percent");
        lv_label_bind_text(ota_percent, &sys_ota_percent, NULL);
        lv_label_set_text(ota_percent, "0%");
        lv_obj_set_align(ota_percent, LV_ALIGN_CENTER);
        lv_obj_set_x(ota_percent, 0);
        lv_obj_set_y(ota_percent, 40);
        lv_obj_set_width(ota_percent, LV_SIZE_CONTENT);
        lv_obj_set_height(ota_percent, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(ota_percent, montserrat_16, 0);
        lv_obj_set_style_text_color(ota_percent, lv_color_hex(0xFFFFFF), 0);

        lv_obj_t * system_busy_group = lv_obj_create(lv_obj_0);
        lv_obj_set_name(system_busy_group, "system_busy_group");
        lv_obj_set_align(system_busy_group, LV_ALIGN_CENTER);
        lv_obj_set_x(system_busy_group, 0);
        lv_obj_set_y(system_busy_group, 0);
        lv_obj_set_width(system_busy_group, 160);
        lv_obj_set_height(system_busy_group, 160);
        lv_obj_set_style_bg_opa(system_busy_group, 0, 0);
        lv_obj_set_style_border_width(system_busy_group, 0, 0);
        lv_obj_set_style_pad_all(system_busy_group, 0, 0);
        lv_obj_set_flag(system_busy_group, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_bind_flag_if_eq(system_busy_group, &sys_busy_visible, LV_OBJ_FLAG_HIDDEN, 0);
        lv_obj_t * busy_spinner = lv_spinner_create(system_busy_group);
        lv_obj_set_name(busy_spinner, "busy_spinner");
        lv_obj_set_align(busy_spinner, LV_ALIGN_CENTER);
        lv_obj_set_x(busy_spinner, 0);
        lv_obj_set_y(busy_spinner, 0);
        lv_obj_set_height(busy_spinner, 48);
        lv_obj_set_style_arc_width(busy_spinner, 6, 0);
        lv_obj_set_style_arc_width(busy_spinner, 6, LV_PART_INDICATOR);
        lv_obj_set_width(busy_spinner, 48);

        lv_obj_t * busy_message = lv_label_create(system_busy_group);
        lv_obj_set_name(busy_message, "busy_message");
        lv_label_bind_text(busy_message, &sys_busy_message, NULL);
        lv_label_set_text(busy_message, "Please wait...");
        lv_obj_set_align(busy_message, LV_ALIGN_CENTER);
        lv_obj_set_x(busy_message, 0);
        lv_obj_set_y(busy_message, 46);
        lv_obj_set_width(busy_message, LV_SIZE_CONTENT);
        lv_obj_set_height(busy_message, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(busy_message, montserrat_12, 0);
        lv_obj_set_style_text_color(busy_message, lv_color_hex(0xFFFFFF), 0);

        lv_obj_t * system_message_group = lv_obj_create(lv_obj_0);
        lv_obj_set_name(system_message_group, "system_message_group");
        lv_obj_set_align(system_message_group, LV_ALIGN_CENTER);
        lv_obj_set_x(system_message_group, 0);
        lv_obj_set_y(system_message_group, 0);
        lv_obj_set_width(system_message_group, 160);
        lv_obj_set_height(system_message_group, 160);
        lv_obj_set_style_bg_opa(system_message_group, 0, 0);
        lv_obj_set_style_border_width(system_message_group, 0, 0);
        lv_obj_set_style_pad_all(system_message_group, 0, 0);
        lv_obj_set_flag(system_message_group, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_bind_flag_if_eq(system_message_group, &sys_msg_visible, LV_OBJ_FLAG_HIDDEN, 0);
        lv_obj_t * msg_icon = lv_label_create(system_message_group);
        lv_obj_set_name(msg_icon, "msg_icon");
        lv_label_set_text(msg_icon, "");
        lv_obj_set_align(msg_icon, LV_ALIGN_CENTER);
        lv_obj_set_x(msg_icon, 0);
        lv_obj_set_y(msg_icon, -34);
        lv_obj_set_width(msg_icon, LV_SIZE_CONTENT);
        lv_obj_set_height(msg_icon, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(msg_icon, montserrat_20, 0);
        lv_obj_set_style_text_color(msg_icon, lv_color_hex(0x6EA8FE), 0);

        lv_obj_t * msg_title = lv_label_create(system_message_group);
        lv_obj_set_name(msg_title, "msg_title");
        lv_label_bind_text(msg_title, &sys_msg_title, NULL);
        lv_label_set_text(msg_title, "Settings saved");
        lv_obj_set_align(msg_title, LV_ALIGN_CENTER);
        lv_obj_set_x(msg_title, 0);
        lv_obj_set_y(msg_title, -4);
        lv_obj_set_width(msg_title, LV_SIZE_CONTENT);
        lv_obj_set_height(msg_title, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(msg_title, montserrat_14, 0);
        lv_obj_set_style_text_color(msg_title, lv_color_hex(0xFFFFFF), 0);

        lv_obj_t * msg_body = lv_label_create(system_message_group);
        lv_obj_set_name(msg_body, "msg_body");
        lv_label_bind_text(msg_body, &sys_msg_body, NULL);
        lv_label_set_text(msg_body, "Applied from web portal");
        lv_obj_set_align(msg_body, LV_ALIGN_CENTER);
        lv_obj_set_x(msg_body, 0);
        lv_obj_set_y(msg_body, 44);
        lv_obj_set_width(msg_body, 140);
        lv_obj_set_height(msg_body, 50);
        lv_label_set_long_mode(msg_body, LV_LABEL_LONG_MODE_WRAP);
        lv_obj_set_style_text_font(msg_body, montserrat_12, 0);
        lv_obj_set_style_text_color(msg_body, lv_color_hex(0x8B929D), 0);
        lv_obj_set_style_text_align(msg_body, LV_TEXT_ALIGN_CENTER, 0);

        the_root = lv_obj_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

