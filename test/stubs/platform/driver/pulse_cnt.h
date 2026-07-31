/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/stubs/platform/driver/pulse_cnt.h
 * Description: Native build stub header for ESP-IDF pulse counter (PCNT) peripheral driver.
 */

#pragma once

#include <cstdint>
#include <cstddef>

typedef void* pcnt_unit_handle_t;
typedef void* pcnt_channel_handle_t;
typedef int esp_err_t;

#ifndef ESP_OK
#define ESP_OK 0
#endif

typedef struct {
    int low_limit;
    int high_limit;
    struct {
        uint32_t accumulation: 1;
    } flags;
} pcnt_unit_config_t;

typedef struct {
    int edge_gpio_num;
    int level_gpio_num;
    struct {
        uint32_t invert_edge: 1;
        uint32_t invert_level: 1;
        uint32_t io_loop_back: 1;
    } flags;
} pcnt_chan_config_t;

typedef struct {
    uint32_t max_glitch_ns;
} pcnt_glitch_filter_config_t;

typedef enum {
    PCNT_CHANNEL_EDGE_ACTION_HOLD = 0,
    PCNT_CHANNEL_EDGE_ACTION_INCREASE,
    PCNT_CHANNEL_EDGE_ACTION_DECREASE,
} pcnt_channel_edge_action_t;

typedef enum {
    PCNT_CHANNEL_LEVEL_ACTION_KEEP = 0,
    PCNT_CHANNEL_LEVEL_ACTION_INVERSE,
    PCNT_CHANNEL_LEVEL_ACTION_HOLD,
} pcnt_channel_level_action_t;

// Test hook allowing unit tests to simulate PCNT count pulses in native builds
inline int g_stub_pcnt_count = 0;

inline esp_err_t pcnt_new_unit(const pcnt_unit_config_t *bus_config, pcnt_unit_handle_t *ret_unit) {
    (void)bus_config;
    if (ret_unit) *ret_unit = (pcnt_unit_handle_t)0x5678;
    return ESP_OK;
}

inline esp_err_t pcnt_new_channel(pcnt_unit_handle_t unit, const pcnt_chan_config_t *chan_config, pcnt_channel_handle_t *ret_chan) {
    (void)unit;
    (void)chan_config;
    if (ret_chan) *ret_chan = (pcnt_channel_handle_t)0x9ABC;
    return ESP_OK;
}

inline esp_err_t pcnt_channel_set_edge_action(pcnt_channel_handle_t chan, pcnt_channel_edge_action_t pos_act, pcnt_channel_edge_action_t neg_act) {
    (void)chan;
    (void)pos_act;
    (void)neg_act;
    return ESP_OK;
}

inline esp_err_t pcnt_channel_set_level_action(pcnt_channel_handle_t chan, pcnt_channel_level_action_t high_act, pcnt_channel_level_action_t low_act) {
    (void)chan;
    (void)high_act;
    (void)low_act;
    return ESP_OK;
}

inline esp_err_t pcnt_unit_set_glitch_filter(pcnt_unit_handle_t unit, const pcnt_glitch_filter_config_t *config) {
    (void)unit;
    (void)config;
    return ESP_OK;
}

inline esp_err_t pcnt_unit_enable(pcnt_unit_handle_t unit) {
    (void)unit;
    return ESP_OK;
}

inline esp_err_t pcnt_unit_start(pcnt_unit_handle_t unit) {
    (void)unit;
    return ESP_OK;
}

inline esp_err_t pcnt_unit_stop(pcnt_unit_handle_t unit) {
    (void)unit;
    return ESP_OK;
}

inline esp_err_t pcnt_unit_disable(pcnt_unit_handle_t unit) {
    (void)unit;
    return ESP_OK;
}

inline esp_err_t pcnt_del_channel(pcnt_channel_handle_t chan) {
    (void)chan;
    return ESP_OK;
}

inline esp_err_t pcnt_del_unit(pcnt_unit_handle_t unit) {
    (void)unit;
    return ESP_OK;
}

inline esp_err_t pcnt_unit_get_count(pcnt_unit_handle_t unit, int *value) {
    (void)unit;
    if (value) {
        *value = g_stub_pcnt_count;
    }
    return ESP_OK;
}

inline esp_err_t pcnt_unit_clear_count(pcnt_unit_handle_t unit) {
    (void)unit;
    g_stub_pcnt_count = 0;
    return ESP_OK;
}
