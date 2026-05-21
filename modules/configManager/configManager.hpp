/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/configManager/configManager.hpp
 * Description: Manages the loading, saving, and version migration of the 
 *              global framework configuration file (/etc/config.json) on LittleFS.
 *              Provides a safe interface for requesting configuration reloads.
 *
 * Exported Functions/Classes:
 * - SystemConfig: Struct containing global framework settings.
 * - ConfigManager: Class for handling configuration persistence and access.
 *   - loadConfig(): Loads the system config from LittleFS or creates a default.
 *   - saveConfig(): Persists the current configuration to LittleFS.
 *   - resetConfig(): Reverts configuration to factory defaults.
 *   - getConfig(): Retrieves read-only reference to the configuration.
 *   - getMutableConfig(): Retrieves mutable reference to the configuration.
 *   - getFrameworkConfig(): Returns a FrameworkConfig struct populated from the current config.
 *   - requestReload(): Sets the flag to indicate a configuration reload is requested.
 *   - checkAndClearReload(): Returns and clears the reload pending flag.
 */

#pragma once

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <LittleFS.h>
#else
#include <string>
#include <stdint.h>
#endif

#include <ArduinoJson.h>
#include "FrameworkConfig.hpp"
#include "../../include/interfaces/IWebContributor.hpp"

#define SYSTEM_CONFIG_VERSION 1.0f // Current configuration schema version

#ifndef NATIVE_BUILD
#define SYSTEM_CONFIG_PATH "/etc/config.json"
#else
#define SYSTEM_CONFIG_PATH "etc_config.json" // Local path for unit testing
#endif

/**
 * @brief Global framework settings stored in /etc/config.json.
 */
struct SystemConfig {
    float configVersion = SYSTEM_CONFIG_VERSION;
    char hostname[33] = "esp32-device";
    char timezone[64] = "GMT0";
    bool firmwareUpdatesEnabled = true;
    uint8_t otaQuietHour = 3;
};

/**
 * @brief Kernel service that manages global framework configuration on LittleFS.
 */
class ConfigManager : public IWebContributor {
public:
    ConfigManager() = default;

    /**
     * @brief Loads the system configuration from LittleFS.
     * @return true if successfully loaded and parsed, false if defaults were applied.
     */
    bool loadConfig();

    /**
     * @brief Persists the current configuration to LittleFS.
     * @return true if successfully saved.
     */
    bool saveConfig();

    /**
     * @brief Reverts configuration to factory defaults and persists them.
     */
    void resetConfig();

    /**
     * @brief Access the current configuration (Read-only).
     * @return Const reference to the internal SystemConfig struct.
     */
    const SystemConfig& getConfig() const { return _config; }

    /**
     * @brief Access the current configuration (Mutable).
     * @return Reference to the internal SystemConfig struct.
     */
    SystemConfig& getMutableConfig() { return _config; }

    /**
     * @brief Extracts a FrameworkConfig adapter for Application::begin().
     * @return FrameworkConfig struct with pointers to internal config values.
     */
    FrameworkConfig getFrameworkConfig() const {
        FrameworkConfig fc;
        fc.hostname = _config.hostname;
        fc.timezone = _config.timezone;
        fc.firmwareUpdatesEnabled = _config.firmwareUpdatesEnabled;
        fc.otaQuietHour = _config.otaQuietHour;
        return fc;
    }

    /**
     * @brief Requests a pending reconfiguration by setting the internal flag to true.
     */
    void requestReload() { _reloadPending = true; }

    /**
     * @brief Examines the pending reload flag and resets it if necessary.
     * @return true if a reload was pending and is now cleared, false otherwise.
     */
    bool checkAndClearReload();

    // --- IWebContributor Interface ---

    /**
     * @brief Register all REST API route handlers with the web server.
     * @param server Reference to the AsyncWebServer instance.
     */
    void registerRoutes(AsyncWebServer& server) override;

    /**
     * @brief Get the UI tab name for this contributor.
     * @return String name of the tab, or nullptr if no UI tab.
     */
    const char* getTabName() const override { return "Config"; }

private:
    SystemConfig _config;        // Master configuration state container
    bool _reloadPending = false; // Flag to safely trigger reconfiguration defensively
};
