/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/FrameworkConfig.hpp
 * Description: POD struct holding framework-level configuration values loaded
 *              from LittleFS /etc/config.json by the ConfigManager. An app
 *              populates this struct during BOOTING and passes it to
 *              Application::begin(). The framework does not own config
 *              persistence — that is the ConfigManager's responsibility.
 *
 * Exported Classes:
 * - FrameworkConfig: Plain-old-data configuration struct
 *   - firmwareUpdatesEnabled: Master kill-switch for OTA update checks
 *   - otaQuietHour:           Hour (0–23) during which OTA checks are suppressed
 *   - otaPublicKey:           PEM-encoded RSA public key for OTA signature verification
 *   - otaGithubRepo:          GitHub repository in "owner/repo" format for OTA releases
 *   - otaGithubToken:         Optional GitHub access token for private repos
 */

#pragma once
#include <stdint.h>

/**
 * @brief Boot-time framework configuration.
 *
 * Provides initial/default values for core services before ConfigManager
 * loads persisted state. Once ConfigManager::beginAll() runs, persisted
 * values (if any) override these defaults via applyConfig().
 *
 * In a downstream app, main.cpp populates this struct and passes it to
 * Application::begin().
 *
 * All fields have safe defaults so the framework can boot without a config file.
 */
struct FrameworkConfig {
    bool        firmwareUpdatesEnabled = true;             ///< OTA update kill-switch
    uint8_t     otaQuietHour          = 3;                 ///< Hour to suppress OTA (0-23)
    const char* otaPublicKey          = nullptr;           ///< PEM-encoded RSA public key for OTA signature verification (nullptr = skip)
    const char* otaGithubRepo         = nullptr;           ///< GitHub repository in "owner/repo" format for OTA releases
    const char* otaGithubToken        = nullptr;           ///< Optional GitHub access token for private repos
};
