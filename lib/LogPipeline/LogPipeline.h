/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/LogPipeline/LogPipeline.h
 * Description: Global log interception pipeline. Installs an esp_log_set_vprintf()
 *              hook to intercept all ESP_LOG* output, redacts sensitive secrets
 *              via Redactor::instance(), and outputs sanitized logs to stdout.
 *
 * Exported Functions/Classes:
 * - LogPipeline: Pure static utility class managing global log interception and redaction.
 *   - begin(): Installs the esp_log_set_vprintf hook.
 *   - isInstalled(): Returns true if the hook has been installed.
 */

#pragma once

#include <cstddef>

/**
 * @brief Global log interception and secret redaction pipeline.
 *
 * LogPipeline installs a vprintf hook into the ESP-IDF logging subsystem.
 * Every ESP_LOG* macro routes through this hook, where secrets registered
 * with the Redactor singleton are redacted in-place before being emitted to stdout.
 */
class LogPipeline {
public:
    /**
     * @brief Installs the esp_log_set_vprintf hook.
     *
     * Idempotent: safe to call multiple times, will only install once.
     */
    static void begin();

    /**
     * @brief Checks if the vprintf hook is currently installed.
     * @return true if installed, false otherwise.
     */
    static bool isInstalled();

    // Delete constructors — pure static class
    LogPipeline() = delete;
};
