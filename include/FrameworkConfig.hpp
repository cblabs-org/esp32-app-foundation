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
 *   - otaPublicKey:  PEM-encoded RSA public key for OTA signature verification
 *   - otaGithubRepo: GitHub repository in "owner/repo" format for OTA releases
 *   - otaGithubToken: Optional GitHub access token for private repos
 *
 * Note: The OTA enable/disable toggle (`updatesEnabled`) is now a persisted
 *       OTAUpdateManager module config field, not a boot-time framework field.
 */

#pragma once

/**
 * @brief Boot-time framework configuration.
 *
 * Currently holds OTA-specific secrets (public key, GitHub repo, token).
 * Named "FrameworkConfig" rather than "OTAConfig" because it is the
 * framework's boot-time configuration entry point — additional non-OTA
 * framework settings may be added here in future (e.g., device name,
 * logging level, feature flags).
 *
 * Provides initial/default values for core services before ConfigManager
 * loads persisted state. Once ConfigManager::beginAll() runs, persisted
 * values (if any) override these defaults via applyConfig().
 *
 * In a downstream app, main.cpp populates this struct and passes it to
 * Application::begin().
 *
 * All fields have safe defaults so the framework can boot without a config file.
 *
 * OTA secrets live here (read-only at boot, never persisted).
 * The OTA enable/disable toggle is owned by OTAUpdateManager (persisted via
 * IConfigurable as `updatesEnabled` in /mod/ota.json).
 *
 * Note: _config is stored as a non-const member of Application because
 * Application::begin() assigns it post-construction. The begin() parameter
 * is already const FrameworkConfig&, and getConfig() returns const&, so
 * downstream code cannot mutate it after boot.
 */
struct FrameworkConfig {
    const char* otaPublicKey  = nullptr;           ///< PEM-encoded RSA public key for OTA signature verification (nullptr = skip)
    const char* otaGithubRepo = nullptr;           ///< GitHub repository in "owner/repo" format for OTA releases
    const char* otaGithubToken = nullptr;          ///< Optional GitHub access token for private repos
};
