/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/stubs/platform/esp_lcd.h
 * Description: Type-complete, behaviorally inert native build stub for ESP-IDF esp_lcd.
 */

#ifndef TEST_STUBS_PLATFORM_ESP_LCD_H_
#define TEST_STUBS_PLATFORM_ESP_LCD_H_

#include <cstdint>
#include <cstddef>
#include "esp_system.h"

#ifndef ESP_OK
#define ESP_OK 0
#endif
#ifndef ESP_FAIL
#define ESP_FAIL -1
#endif
typedef int esp_err_t;

typedef struct esp_lcd_panel_io_t* esp_lcd_panel_io_handle_t;
typedef struct esp_lcd_panel_t* esp_lcd_panel_handle_t;

typedef struct {
    int cs_gpio_num;
    int dc_gpio_num;
    int spi_mode;
    unsigned int pclk_hz;
    size_t trans_queue_depth;
    void* on_color_trans_done;
    void* user_ctx;
    int lcd_cmd_bits;
    int lcd_param_bits;
} esp_lcd_panel_io_spi_config_t;

typedef struct {
    int reset_gpio_num;
    int rgb_endian;
    int bits_per_pixel;
    struct {
        unsigned int reset_active_high : 1;
    } flags;
} esp_lcd_panel_dev_config_t;

inline esp_err_t esp_lcd_new_panel_io_spi(void* bus, const esp_lcd_panel_io_spi_config_t* io_config, esp_lcd_panel_io_handle_t* ret_io) {
    (void)bus; (void)io_config;
    if (ret_io) *ret_io = reinterpret_cast<esp_lcd_panel_io_handle_t>(1);
    return ESP_OK;
}

#endif  // TEST_STUBS_PLATFORM_ESP_LCD_H_
