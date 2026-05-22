/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/platform/LogShim.h
 * Description: Unified logging shim that bridges ESP-IDF's esp_log.h API to
 *              the native GoogleTest build environment. All modules should
 *              include this header instead of esp_log.h directly.
 *
 *              On hardware: delegates to the real ESP-IDF logging system,
 *              inheriting compile-time stripping (CORE_DEBUG_LEVEL),
 *              runtime per-module control (esp_log_level_set), thread safety,
 *              and pluggable backends (esp_log_set_vprintf).
 *
 *              On native: thin printf shims that mirror the ESP-IDF output
 *              format for test readability.
 *
 * Usage:
 *   #include "platform/LogShim.h"
 *   static const char* TAG = "MyModule";
 *   ESP_LOGI(TAG, "Initialized with %d items", count);
 */

#pragma once

#ifndef NATIVE_BUILD
    // --- Hardware build: delegate to the real ESP-IDF logging system ---
    #include "esp_log.h"
#else
    // --- Native build: printf shims matching ESP-IDF output format ---
    #include <cstdio>

    // Log level constants (mirror esp_log.h enum values)
    #define ESP_LOG_NONE    0
    #define ESP_LOG_ERROR   1
    #define ESP_LOG_WARN    2
    #define ESP_LOG_INFO    3
    #define ESP_LOG_DEBUG   4
    #define ESP_LOG_VERBOSE 5

    #define ESP_LOGE(tag, fmt, ...) printf("E (%s): " fmt "\n", tag, ##__VA_ARGS__)
    #define ESP_LOGW(tag, fmt, ...) printf("W (%s): " fmt "\n", tag, ##__VA_ARGS__)
    #define ESP_LOGI(tag, fmt, ...) printf("I (%s): " fmt "\n", tag, ##__VA_ARGS__)
    #define ESP_LOGD(tag, fmt, ...) printf("D (%s): " fmt "\n", tag, ##__VA_ARGS__)
    #define ESP_LOGV(tag, fmt, ...) printf("V (%s): " fmt "\n", tag, ##__VA_ARGS__)

    // No-op for native builds — runtime level control is hardware-only
    static inline void esp_log_level_set(const char* tag, int level) { (void)tag; (void)level; }
#endif
