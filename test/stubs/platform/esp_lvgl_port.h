/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/stubs/platform/esp_lvgl_port.h
 * Description: Native build stub for esp_lvgl_port helper library.
 */

#ifndef TEST_STUBS_PLATFORM_ESP_LVGL_PORT_H_
#define TEST_STUBS_PLATFORM_ESP_LVGL_PORT_H_

#include "esp_lcd.h"
#include "lvgl.h"

typedef struct {
    int task_priority;
    int task_stack;
    int task_affinity;
    int timer_period_ms;
} lvgl_port_cfg_t;

typedef struct {
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_handle_t panel_handle;
    uint32_t buffer_size;
    bool double_buffer;
    int hres;
    int vres;
    bool monochrome;
    lv_display_rotation_t rotation;
} lvgl_port_disp_config_t;

typedef lv_display_t* lvgl_port_disp_handle_t;

inline esp_err_t lvgl_port_init(const lvgl_port_cfg_t* cfg) {
    (void)cfg;
    return ESP_OK;
}

inline lv_display_t* lvgl_port_add_disp(const lvgl_port_disp_config_t* disp_cfg) {
    return lv_display_create(disp_cfg ? disp_cfg->hres : 240, disp_cfg ? disp_cfg->vres : 240);
}

#endif  // TEST_STUBS_PLATFORM_ESP_LVGL_PORT_H_
