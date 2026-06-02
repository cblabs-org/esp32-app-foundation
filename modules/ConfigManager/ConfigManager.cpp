/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/ConfigManager/configManager.cpp
 * Description: Implementation of the ConfigManager centralised configuration gateway.
 *              Owns all config file I/O for registered IConfigurable modules,
 *              handles schema versioning and migration, and provides bulk
 *              export/import for backup/restore.
 *
 * Exported Functions/Classes:
 * - ConfigManager: see ConfigManager.hpp
 */

#include "ConfigManager.hpp"
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
#include <cstdio>
#endif

static const char* TAG = "ConfigManager";

// =============================================================================
// Path Helper
// =============================================================================

/**
 * @brief Format the config file path for a given namespace.
 *
 * On hardware: "/mod/<ns>.json" (LittleFS path convention)
 * On native:   "mod_<ns>.json" (flat file in working directory for tests)
 *
 * @param ns   Namespace string (e.g. "wifi").
 * @param buf  Output buffer.
 * @param size Buffer size in bytes.
 */
void ConfigManager::getConfigPath(const char* ns, char* buf, size_t size) {
#ifndef NATIVE_BUILD
    snprintf(buf, size, "/mod/%s.json", ns);
#else
    snprintf(buf, size, "mod_%s.json", ns);
#endif
}

// =============================================================================
// Module Registry
// =============================================================================

/**
 * @brief Register an IConfigurable module with the gateway.
 *
 * Performs bounds checking against MAX_CONFIGURABLES and null-pointer guard.
 *
 * @param module Non-null pointer to the IConfigurable module.
 * @return true if registration succeeded; false if full or nullptr.
 */
bool ConfigManager::registerModule(IConfigurable* module) {
    if (module == nullptr) {
        ESP_LOGE(TAG, "registerModule() rejected — null pointer");
        return false;
    }
    if (_moduleCount >= MAX_CONFIGURABLES) {
        ESP_LOGE(TAG, "registerModule() rejected — registry full (%u/%u)",
                 _moduleCount, MAX_CONFIGURABLES);
        return false;
    }
    _modules[_moduleCount++] = module;
    ESP_LOGI(TAG, "Registered module '%s' (slot %u/%u)",
             module->getConfigNamespace(), _moduleCount, MAX_CONFIGURABLES);
    return true;
}

// =============================================================================
// Boot Sequence
// =============================================================================

/**
 * @brief Load and apply configuration for all registered modules.
 *
 * Iterates the fixed-size module registry and calls loadModuleConfig()
 * on each registered module. Handles file reads, version checks,
 * migrations, and applyConfig() dispatch.
 */
void ConfigManager::beginAll() {
    ESP_LOGI(TAG, "beginAll() — loading config for %u module(s)", _moduleCount);

    for (uint8_t i = 0; i < _moduleCount; i++) {
        loadModuleConfig(_modules[i]);
    }

    ESP_LOGI(TAG, "beginAll() complete");
}

/**
 * @brief Load a single module's config from its namespace file.
 *
 * Opens /mod/<namespace>.json, deserialises it, reads the stored _v version,
 * compares against the module's getSchemaVersion(), calls migrateConfig()
 * if needed, then calls applyConfig(). Rewrites the file if migration occurred.
 *
 * @param module Pointer to the IConfigurable module.
 */
void ConfigManager::loadModuleConfig(IConfigurable* module) {
    const char* ns = module->getConfigNamespace();
    char path[48]; // "/mod/" (5) + namespace (max ~32) + ".json" (5) + null
    getConfigPath(ns, path, sizeof(path));

    ESP_LOGI(TAG, "Loading config for '%s' from %s", ns, path);

    // --- Read the config file ---
    JsonDocument doc;
    bool fileExists = false;

#ifndef NATIVE_BUILD
    if (LittleFS.exists(path)) {
        File file = LittleFS.open(path, "r");
        if (file) {
            DeserializationError err = deserializeJson(doc, file);
            file.close();
            if (err) {
                ESP_LOGE(TAG, "Failed to parse %s: %s — applying defaults", path, err.c_str());
                module->resetConfig();
                saveModuleConfig(module);
                return;
            }
            fileExists = true;
        } else {
            ESP_LOGW(TAG, "Failed to open %s for reading", path);
        }
    }
#else
    std::ifstream file(path);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonStr = buffer.str();
        file.close();

        DeserializationError err = deserializeJson(doc, jsonStr);
        if (err) {
            ESP_LOGE(TAG, "Failed to parse %s: %s — applying defaults", path, err.c_str());
            module->resetConfig();
            saveModuleConfig(module);
            return;
        }
        fileExists = true;
    }
#endif

    if (!fileExists) {
        // No config file — apply defaults and create the file
        ESP_LOGW(TAG, "Config file %s not found — applying defaults for '%s'", path, ns);
        module->resetConfig();
        saveModuleConfig(module);
        return;
    }

    // --- Version check and migration ---
    int storedVersion = doc["_v"] | 0;
    int declaredVersion = module->getSchemaVersion();
    bool migrated = false;

    if (storedVersion < declaredVersion) {
        ESP_LOGI(TAG, "Schema migration needed for '%s': v%d → v%d", ns, storedVersion, declaredVersion);
        JsonObject root = doc.as<JsonObject>();
        if (module->migrateConfig(root, storedVersion)) {
            ESP_LOGI(TAG, "Migration succeeded for '%s'", ns);
            migrated = true;
        } else {
            ESP_LOGW(TAG, "Migration failed for '%s' — resetting to defaults", ns);
            module->resetConfig();
            saveModuleConfig(module);
            return;
        }
    }

    // --- Apply the configuration ---
    JsonObject root = doc.as<JsonObject>();
    module->applyConfig(root);

    // --- Rewrite file if migration occurred ---
    if (migrated) {
        saveModuleConfig(module);
    }
}

// =============================================================================
// Save
// =============================================================================

/**
 * @brief Persist a single module's current configuration to its namespace file.
 *
 * Creates a JsonDocument (~256 bytes), calls serializeConfig() to populate it,
 * sets the _v field, and writes directly to the file stream (no intermediate
 * String buffer — critical for embedded memory constraints).
 *
 * @param module Pointer to the IConfigurable module to save.
 * @return true if successfully saved; false on error.
 */
bool ConfigManager::saveModuleConfig(IConfigurable* module) {
    if (module == nullptr) {
        ESP_LOGE(TAG, "saveModuleConfig() — null module pointer");
        return false;
    }

    const char* ns = module->getConfigNamespace();
    char path[48];
    getConfigPath(ns, path, sizeof(path));

    ESP_LOGI(TAG, "Saving config for '%s' to %s", ns, path);

    // --- Build the JSON document ---
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    module->serializeConfig(root);
    doc["_v"] = module->getSchemaVersion();

    // --- Write to file ---
#ifndef NATIVE_BUILD
    // Ensure the /mod/ directory exists
    if (!LittleFS.exists("/mod")) {
        LittleFS.mkdir("/mod");
    }

    File file = LittleFS.open(path, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open %s for writing", path);
        return false;
    }

    // CRITICAL: serialize directly to File stream — no intermediate String
    if (serializeJson(doc, file) == 0) {
        ESP_LOGE(TAG, "Failed to write config to %s", path);
        file.close();
        return false;
    }
    file.close();
#else
    std::ofstream file(path);
    if (!file.is_open()) {
        ESP_LOGE(TAG, "Failed to open %s for writing", path);
        return false;
    }
    std::string jsonStr;
    serializeJson(doc, jsonStr);
    file << jsonStr;
    file.close();
#endif

    ESP_LOGI(TAG, "Config saved for '%s'", ns);
    return true;
}

// =============================================================================
// Bulk Export / Import
// =============================================================================

/**
 * @brief Export all registered module configs into a master JSON document.
 *
 * For each registered module, creates a nested object keyed by namespace,
 * calls serializeConfig(), and adds the _v field. The master document is
 * suitable for full-device backup (~2-3KB heap).
 *
 * @param outDoc Reference to the output JsonDocument (caller-owned).
 */
void ConfigManager::exportAllConfig(JsonDocument& outDoc) {
    ESP_LOGI(TAG, "Exporting config for %u module(s)", _moduleCount);

    for (uint8_t i = 0; i < _moduleCount; i++) {
        IConfigurable* mod = _modules[i];
        const char* ns = mod->getConfigNamespace();

        JsonObject section = outDoc[ns].to<JsonObject>();
        mod->serializeConfig(section);
        section["_v"] = mod->getSchemaVersion();
    }
}

/**
 * @brief Import module configs from a master JSON document.
 *
 * For each registered module, extracts its namespace section from the input
 * document, calls applyConfig(), and persists via saveModuleConfig().
 * Modules whose namespace is not found in the input are skipped.
 *
 * @param inDoc Reference to the input JsonDocument.
 * @return true if all found modules imported successfully.
 */
bool ConfigManager::importAllConfig(JsonDocument& inDoc) {
    ESP_LOGI(TAG, "Importing config for %u module(s)", _moduleCount);
    bool allOk = true;

    for (uint8_t i = 0; i < _moduleCount; i++) {
        IConfigurable* mod = _modules[i];
        const char* ns = mod->getConfigNamespace();

        if (!inDoc[ns].is<JsonObject>()) {
            ESP_LOGW(TAG, "No config section found for '%s' in import — skipping", ns);
            continue;
        }

        JsonObject section = inDoc[ns].as<JsonObject>();
        mod->applyConfig(section);

        if (!saveModuleConfig(mod)) {
            ESP_LOGE(TAG, "Failed to save imported config for '%s'", ns);
            allOk = false;
        }
    }

    return allOk;
}


// =============================================================================
// IWebContributor — REST API Routes
// =============================================================================

/**
 * @brief Register REST API routes for the config gateway.
 *
 * Routes:
 *   GET  /api/config         — returns a JSON array of registered namespace identifiers.
 *   GET  /api/config/backup  — exports all module configs as a downloadable JSON file.
 *   POST /api/config/restore — upload JSON config, validate, import, and restart.
 *                              Body limit: 8KB to prevent heap exhaustion.
 *
 * @param server Reference to the AsyncWebServer instance.
 */
void ConfigManager::registerRoutes(AsyncWebServer& server) {
#ifndef NATIVE_BUILD
    // --- GET /api/config — list registered namespaces ---
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        JsonDocument doc;
        JsonArray namespaces = doc["namespaces"].to<JsonArray>();
        for (uint8_t i = 0; i < _moduleCount; i++) {
            namespaces.add(_modules[i]->getConfigNamespace());
        }
        doc["count"] = _moduleCount;

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // --- GET /api/config/backup — export all config as downloadable JSON ---
    server.on("/api/config/backup", HTTP_GET, [this](AsyncWebServerRequest* request) {
        JsonDocument doc;
        exportAllConfig(doc);

        String output;
        serializeJson(doc, output);

        // Set Content-Disposition to trigger browser download
        AsyncWebServerResponse* response = request->beginResponse(
            200, "application/json", output);
        response->addHeader("Content-Disposition",
            "attachment; filename=\"config-backup.json\"");
        request->send(response);

        ESP_LOGI("ConfigManager", "Config backup exported (%u bytes)", output.length());
    });

    // --- POST /api/config/restore — upload JSON, validate, import, restart ---
    server.on("/api/config/restore", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
               size_t index, size_t total) {

            // --- Body size guard (8KB max) ---
            constexpr size_t MAX_RESTORE_BODY = 8192;
            if (total > MAX_RESTORE_BODY) {
                request->send(413, "application/json",
                    "{\"error\":\"Payload too large\",\"max\":8192}");
                return;
            }

            // Only process when we have the complete body (index + len == total)
            // For chunked uploads, accumulate in the final chunk
            if (index + len != total) {
                return; // Wait for remaining chunks
            }

            // --- Parse the uploaded JSON ---
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, data, len);
            if (err) {
                ESP_LOGE("ConfigManager", "Restore JSON parse failed: %s", err.c_str());
                String errMsg = "{\"error\":\"Invalid JSON: ";
                errMsg += err.c_str();
                errMsg += "\"}";
                request->send(400, "application/json", errMsg);
                return;
            }

            // --- Validate: must be a JSON object with at least one key ---
            if (!doc.is<JsonObject>() || doc.as<JsonObject>().size() == 0) {
                request->send(400, "application/json",
                    "{\"error\":\"Config must be a non-empty JSON object\"}");
                return;
            }

            // --- Import the config ---
            bool success = importAllConfig(doc);
            if (!success) {
                ESP_LOGW("ConfigManager", "Some modules failed during config import");
            }

            ESP_LOGI("ConfigManager", "Config restore complete — scheduling restart");

            // Send response before restart
            request->send(200, "application/json",
                "{\"status\":\"ok\",\"msg\":\"Config restored. Restarting...\"}");

            // --- Schedule restart (500ms delay for response to flush) ---
            // Use a FreeRTOS software timer to avoid blocking the async handler
            TimerHandle_t restartTimer = xTimerCreate(
                "cfg_restart", pdMS_TO_TICKS(500), pdFALSE, nullptr,
                [](TimerHandle_t t) {
                    ESP.restart();
                }
            );
            if (restartTimer != nullptr) {
                xTimerStart(restartTimer, 0);
            }
        }
    );
#endif
}

