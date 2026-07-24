/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: test/stubs/platform/esp_lcd_panel_vendor.h
 * Description: Native stub for esp_lcd vendor drivers (e.g. GC9A01).
 */

#ifndef TEST_STUBS_PLATFORM_ESP_LCD_PANEL_VENDOR_H_
#define TEST_STUBS_PLATFORM_ESP_LCD_PANEL_VENDOR_H_

#include "esp_lcd.h"

inline esp_err_t esp_lcd_new_panel_gc9a01(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t* panel_dev_config, esp_lcd_panel_handle_t* ret_panel) {
    (void)io; (void)panel_dev_config;
    if (ret_panel) *ret_panel = reinterpret_cast<esp_lcd_panel_handle_t>(1);
    return ESP_OK;
}

#endif  // TEST_STUBS_PLATFORM_ESP_LCD_PANEL_VENDOR_H_
