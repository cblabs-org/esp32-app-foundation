/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Header: test/stubs/platform/iot_button.h
 * Description: Native build stub for Espressif iot_button component.
 */

#pragma once

#include <cstdint>
#include <cstddef>

typedef void* button_handle_t;
typedef int esp_err_t;

#ifndef ESP_OK
#define ESP_OK 0
#endif

typedef enum {
    BUTTON_TYPE_GPIO,
    BUTTON_TYPE_ADC,
    BUTTON_TYPE_MATRIX,
} button_type_t;

typedef enum {
    BUTTON_SINGLE_CLICK = 0,
    BUTTON_DOUBLE_CLICK,
    BUTTON_LONG_PRESS_START,
    BUTTON_PRESS_DOWN,
    BUTTON_EVENT_MAX,
} button_event_t;

typedef void (*button_cb_t)(void *button_handle, void *usr_data);

typedef struct {
    int32_t gpio_num;
    uint8_t active_level;
    bool enable_power_save;
} button_gpio_config_t;

typedef struct {
    int32_t adc_channel;
    uint16_t min_voltage;
    uint16_t max_voltage;
} button_adc_config_t;

typedef struct {
    uint8_t type;
    union {
        button_gpio_config_t gpio_button_config;
        button_adc_config_t adc_button_config;
    };
} button_config_t;

inline button_handle_t iot_button_create(const button_config_t *config) {
    return (button_handle_t)0x1234;
}

inline esp_err_t iot_button_new_gpio_device(const button_config_t *config, const button_gpio_config_t *gpio_config, button_handle_t *ret_button) {
    if (ret_button) *ret_button = (button_handle_t)0x1234;
    return ESP_OK;
}

inline esp_err_t iot_button_register_cb(button_handle_t btn_handle, button_event_t event, void* event_args, button_cb_t cb, void *usr_data) {
    return ESP_OK;
}

inline esp_err_t iot_button_unregister_cb(button_handle_t btn_handle, button_event_t event) {
    return ESP_OK;
}

inline esp_err_t iot_button_delete(button_handle_t btn_handle) {
    return ESP_OK;
}
