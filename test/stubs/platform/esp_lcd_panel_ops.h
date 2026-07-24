/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: test/stubs/platform/esp_lcd_panel_ops.h
 * Description: Native stub for esp_lcd panel operations.
 */

#ifndef TEST_STUBS_PLATFORM_ESP_LCD_PANEL_OPS_H_
#define TEST_STUBS_PLATFORM_ESP_LCD_PANEL_OPS_H_

#include "esp_lcd.h"

inline esp_err_t esp_lcd_panel_reset(esp_lcd_panel_handle_t panel) { (void)panel; return ESP_OK; }
inline esp_err_t esp_lcd_panel_init(esp_lcd_panel_handle_t panel) { (void)panel; return ESP_OK; }
inline esp_err_t esp_lcd_panel_invert_color(esp_lcd_panel_handle_t panel, bool invert_color_data) { (void)panel; (void)invert_color_data; return ESP_OK; }
inline esp_err_t esp_lcd_panel_disp_on_off(esp_lcd_panel_handle_t panel, bool on_off) { (void)panel; (void)on_off; return ESP_OK; }

#endif  // TEST_STUBS_PLATFORM_ESP_LCD_PANEL_OPS_H_
