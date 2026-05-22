/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/configManager/configManager.cpp
 * Description: Implementation of the ConfigManager kernel service. Handles reading
 *              and writing the generic JSON configuration file for the framework.
 */

#include "configManager.hpp"
#ifndef NATIVE_BUILD
#include <ESPAsyncWebServer.h>
#endif
#include <string.h>
#include "platform/LogShim.h"

#ifndef NATIVE_BUILD
// (no additional hardware-only includes needed)
#else
#include <fstream>
#include <sstream>
#endif

static const char* TAG = "ConfigManager";

#ifdef NATIVE_BUILD
// A simple fallback for strlcpy in native testing environment if not available.
// macOS has strlcpy natively, but just to be safe if Linux is used:
#if !defined(__APPLE__) && !defined(__FreeBSD__)
static size_t strlcpy(char *dst, const char *src, size_t siz) {
    char *d = dst;
    const char *s = src;
    size_t n = siz;
    if (n != 0) {
        while (--n != 0) {
            if ((*d++ = *s++) == '\0') break;
        }
    }
    if (n == 0) {
        if (siz != 0) *d = '\0';
        while (*s++) ;
    }
    return (s - src - 1);
}
#endif
#endif

/**
 * @brief Loads the system configuration from LittleFS.
 * @return true if successfully loaded and parsed, false if defaults were applied.
 */
bool ConfigManager::loadConfig() {
    ESP_LOGI(TAG, "Loading system configuration from %s", SYSTEM_CONFIG_PATH);

#ifndef NATIVE_BUILD
    if (!LittleFS.exists(SYSTEM_CONFIG_PATH)) {
        ESP_LOGW(TAG, "Configuration file not found. Applying defaults.");
        saveConfig();
        return false;
    }

    File file = LittleFS.open(SYSTEM_CONFIG_PATH, "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open configuration file for reading.");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
#else
    std::ifstream file(SYSTEM_CONFIG_PATH);
    if (!file.is_open()) {
        ESP_LOGW(TAG, "Configuration file not found. Applying defaults.");
        saveConfig();
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonStr = buffer.str();
    file.close();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
#endif

    if (error) {
        ESP_LOGE(TAG, "Failed to parse configuration file: %s", error.c_str());
        ESP_LOGW(TAG, "Reverting to safe factory defaults.");
        resetConfig();
        return false;
    }

    // Process variables with safe defaults
    float loadedVersion = doc["configVersion"] | 1.0f;
    _config.configVersion = loadedVersion;

    if (doc["hostname"].is<const char*>()) {
        strlcpy(_config.hostname, doc["hostname"], sizeof(_config.hostname));
    }
    if (doc["timezone"].is<const char*>()) {
        strlcpy(_config.timezone, doc["timezone"], sizeof(_config.timezone));
    }
    
    _config.firmwareUpdatesEnabled = doc["firmwareUpdatesEnabled"] | true;
    _config.otaQuietHour = doc["otaQuietHour"] | 3;

    // Handle version migration if necessary
    if (loadedVersion < SYSTEM_CONFIG_VERSION) {
        ESP_LOGI(TAG, "Migrating configuration from v%.1f to v%.1f", loadedVersion, SYSTEM_CONFIG_VERSION);
        _config.configVersion = SYSTEM_CONFIG_VERSION;
        saveConfig();
    }

    ESP_LOGI(TAG, "Configuration successfully loaded.");
    return true;
}

/**
 * @brief Persists the current configuration to LittleFS.
 * @return true if successfully saved.
 */
bool ConfigManager::saveConfig() {
    ESP_LOGI(TAG, "Saving system configuration to %s", SYSTEM_CONFIG_PATH);

    JsonDocument doc;
    doc["configVersion"] = _config.configVersion;
    doc["hostname"] = _config.hostname;
    doc["timezone"] = _config.timezone;
    doc["firmwareUpdatesEnabled"] = _config.firmwareUpdatesEnabled;
    doc["otaQuietHour"] = _config.otaQuietHour;

#ifndef NATIVE_BUILD
    File file = LittleFS.open(SYSTEM_CONFIG_PATH, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open configuration file for writing.");
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        ESP_LOGE(TAG, "Failed to write to configuration file.");
        file.close();
        return false;
    }

    file.close();
#else
    std::ofstream file(SYSTEM_CONFIG_PATH);
    if (!file.is_open()) {
        ESP_LOGE(TAG, "Failed to open configuration file for writing.");
        return false;
    }
    std::string jsonStr;
    serializeJson(doc, jsonStr);
    file << jsonStr;
    file.close();
#endif

    ESP_LOGI(TAG, "Configuration saved successfully.");
    return true;
}

/**
 * @brief Reverts configuration to factory defaults and persists them.
 */
void ConfigManager::resetConfig() {
    ESP_LOGW(TAG, "Resetting system configuration to factory defaults.");
    _config = SystemConfig(); // Reset struct to defaults
    saveConfig();
}

/**
 * @brief Examines the pending reload flag and resets it if necessary.
 * @return true if a reload was pending and is now cleared, false otherwise.
 */
bool ConfigManager::checkAndClearReload() {
    bool wasPending = _reloadPending;
    _reloadPending = false;
    return wasPending;
}

void ConfigManager::registerRoutes(AsyncWebServer& server) {
#ifndef NATIVE_BUILD
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["hostname"] = _config.hostname;
        doc["timezone"] = _config.timezone;
        doc["firmwareUpdatesEnabled"] = _config.firmwareUpdatesEnabled;
        doc["otaQuietHour"] = _config.otaQuietHour;
        
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    server.on("/api/config", HTTP_POST, 
        [](AsyncWebServerRequest *request) {},
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            if (!error) {
                if (doc["hostname"].is<const char*>()) strlcpy(_config.hostname, doc["hostname"], sizeof(_config.hostname));
                if (doc["timezone"].is<const char*>()) strlcpy(_config.timezone, doc["timezone"], sizeof(_config.timezone));
                if (doc["firmwareUpdatesEnabled"].is<bool>()) _config.firmwareUpdatesEnabled = doc["firmwareUpdatesEnabled"];
                if (doc["otaQuietHour"].is<int>()) _config.otaQuietHour = doc["otaQuietHour"];
                
                this->saveConfig();
                this->requestReload();
                
                request->send(200, "application/json", "{\"status\":\"ok\"}");
                return;
            }
            request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid JSON\"}");
        }
    );
#endif
}
