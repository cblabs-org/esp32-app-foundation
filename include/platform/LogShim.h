/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
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
 *
 *              On native: resolved through test stubs at test/stubs/platform/esp_log.h (see platformio.ini [env:native] -I flags)
 *
 * Usage:
 *   #include "platform/LogShim.h"
 *   static const char* TAG = "MyModule";
 *   ESP_LOGI(TAG, "Initialized with %d items", count);
 */

#pragma once

// --- Hardware build: delegate to the real ESP-IDF logging system ---
#include "esp_log.h"
