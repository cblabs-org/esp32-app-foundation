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

    // --- Hardware build: delegate to the real ESP-IDF logging system ---
    #include "esp_log.h"
