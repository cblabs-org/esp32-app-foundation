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

/**
 * @brief Register the timezone string for the backup _meta envelope.
 *
 * Called by TimeManager::applyConfig() to propagate the configured timezone
 * into the backup envelope without creating a direct ConfigManager→TimeManager
 * coupling. Stores up to 63 characters of the TZ string.
 *
 * @param tz Null-terminated POSIX timezone string (e.g. "Europe/London").
 */
void ConfigManager::setMetaTimezone(const char* tz) {
    if (tz != nullptr) {
        strlcpy(_metaTimezone, tz, sizeof(_metaTimezone));
        ESP_LOGD(TAG, "Meta timezone set to '%s'", _metaTimezone);
    }
}

// =============================================================================
// Module Registry
// =============================================================================

/**
 * @brief Register an IConfigurable module with the gateway.
 *
 * Rejected if called after beginAll() has locked the registry, or if the
 * pointer is null. Matching Application's two-tier locked-vector model.
 *
 * @param module Non-null pointer to the IConfigurable module.
 * @return true if registration succeeded; false if locked or nullptr.
 */
bool ConfigManager::registerModule(IConfigurable* module) {
    if (module == nullptr) {
        ESP_LOGE(TAG, "registerModule() rejected — null pointer");
        return false;
    }
    if (_locked) {
        ESP_LOGE(TAG, "registerModule() rejected — registry is locked after beginAll()");
        return false;
    }
    _modules.push_back(module);
    ESP_LOGI(TAG, "Registered module '%s' (slot %d)",
             module->getConfigNamespace(), (int)_modules.size());
    return true;
}

// =============================================================================
// Boot Sequence
// =============================================================================

/**
 * @brief Load and apply configuration for all registered modules.
 *
 * Locks the registry (matching Application's compact-and-lock model), then
 * iterates the module list and calls loadModuleConfig() on each.
 */
void ConfigManager::beginAll() {
    // Lock the registry — post-beginAll() registration attempts will be rejected
    _locked = true;
    _modules.shrink_to_fit();

    ESP_LOGI(TAG, "beginAll() — loading config for %d module(s)", (int)_modules.size());

    for (IConfigurable* mod : _modules) {
        loadModuleConfig(mod);
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
 * @brief Populate the _meta envelope for a backup document.
 *
 * Writes standard metadata fields into the provided JsonObject:
 *   - backup_version: always 1
 *   - firmware_version: from FIRMWARE_VERSION build define (or "unknown")
 *   - device_id: chip MAC address (or "00:00:00:00:00:00" on NATIVE_BUILD)
 *   - device_type: chip model string (or "native" on NATIVE_BUILD)
 *   - datetime: ISO-8601 UTC string if system clock is set, else "unknown"
 *   - timezone: TZ string registered via setMetaTimezone(), else "UTC"
 *   - module_count: number of registered IConfigurable modules
 *
 * @param meta Mutable reference to the JsonObject to populate.
 */
void ConfigManager::buildBackupMeta(JsonObject& meta) {
    meta["backup_version"] = 1;

#ifdef APP_FOUNDATION_VERSION
    meta["firmware_version"] = APP_FOUNDATION_VERSION;
#else
    meta["firmware_version"] = "unknown";
#endif

#ifndef NATIVE_BUILD
    meta["device_id"]   = WiFi.macAddress();
    meta["device_type"] = ESP.getChipModel();

    // Populate datetime from the system clock if it has been synchronised
    // (epoch > 1 000 000 000 means the RTC has been set via NTP or RTC module)
    time_t now = time(nullptr);
    if (now > 1000000000) {
        char buf[25];
        struct tm tm_info;
        gmtime_r(&now, &tm_info);
        // Format as ISO-8601: "2024-01-15T12:34:56Z"
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_info);
        meta["datetime"] = buf;
    } else {
        meta["datetime"] = "unknown";
    }
#else
    meta["device_id"]   = "00:00:00:00:00:00";
    meta["device_type"] = "native";
    meta["datetime"]    = "unknown";
#endif

    // TODO: call setMetaTimezone() from TimeManager::applyConfig() to propagate
    //       the configured timezone into the backup envelope.
    meta["timezone"]     = _metaTimezone[0] != '\0' ? _metaTimezone : "UTC";
    meta["module_count"] = (int)_modules.size();
}

/**
 * @brief Export all registered module configs into a master JSON document.
 *
 * The output document structure:
 * {
 *   "_meta": { backup_version, firmware_version, device_id, device_type,
 *              datetime, timezone, module_count },
 *   "<namespace>": { ... module fields ..., "_v": <schema_version> },
 *   ...
 * }
 *
 * The _meta key is always written first. Unknown keys (e.g. _meta) are
 * skipped silently during importAllConfig().
 *
 * @param outDoc Reference to the output JsonDocument (caller-owned).
 */
void ConfigManager::exportAllConfig(JsonDocument& outDoc) {
    ESP_LOGI(TAG, "Exporting config for %d module(s)", (int)_modules.size());

    // Write _meta first so it appears as the leading key in the output
    JsonObject meta = outDoc["_meta"].to<JsonObject>();
    buildBackupMeta(meta);

    for (IConfigurable* mod : _modules) {
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
 * Keys present in the import document that do not match any registered module
 * namespace (e.g. "_meta") are silently skipped. A DEBUG-level log is emitted
 * for each unmatched key to aid troubleshooting without filling the serial log.
 *
 * @param inDoc Reference to the input JsonDocument.
 * @return true if all found modules imported successfully.
 */
bool ConfigManager::importAllConfig(JsonDocument& inDoc) {
    ESP_LOGI(TAG, "Importing config for %d module(s)", (int)_modules.size());
    bool allOk = true;

    // Iterate over every top-level key in the import document and emit a debug
    // log for keys that are not matched by any registered module (e.g. "_meta")
    for (JsonPair kv : inDoc.as<JsonObject>()) {
        const char* ns = kv.key().c_str();
        bool matched = false;
        for (IConfigurable* mod : _modules) {
            if (strcmp(mod->getConfigNamespace(), ns) == 0) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            ESP_LOGD(TAG, "Skipping unknown key '%s' during import", ns);
        }
    }

    for (IConfigurable* mod : _modules) {
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
 * Routes are registered with specific sub-paths BEFORE the generic parent
 * to prevent ESPAsyncWebServer prefix matching from swallowing sub-route requests:
 *   GET  /api/config/backup  — exports all module configs as a downloadable JSON file.
 *   POST /api/config/restore — upload JSON config, validate, import, and restart.
 *                              Body limit: 8KB to prevent heap exhaustion.
 *   GET  /api/config         — returns a JSON object with registered namespace identifiers.
 *
 * @param server Reference to the AsyncWebServer instance.
 */
void ConfigManager::registerRoutes(AsyncWebServer& server) {
#ifndef NATIVE_BUILD
    // --- GET /api/config/backup — export all config as downloadable JSON ---
    // IMPORTANT: registered BEFORE /api/config to prevent prefix-match swallowing.
    server.on("/api/config/backup", HTTP_GET, [this](AsyncWebServerRequest* request) {
        JsonDocument doc;
        exportAllConfig(doc);

        // Use AsyncResponseStream to serialize directly to the response without
        // an intermediate String buffer — critical for embedded memory constraints.
        AsyncResponseStream* stream = request->beginResponseStream("application/json");
        stream->addHeader("Content-Disposition",
            "attachment; filename=\"config-backup.json\"");
        serializeJson(doc, *stream);
        request->send(stream);

        ESP_LOGI(TAG, "Config backup exported via stream");
    });

    // --- POST /api/config/restore — upload JSON, validate, import, restart ---
    // IMPORTANT: registered BEFORE /api/config to prevent prefix-match swallowing.
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

    // --- GET /api/config — list registered namespaces ---
    // IMPORTANT: registered AFTER specific sub-paths to prevent prefix matching
    // from swallowing /api/config/backup and /api/config/restore requests.
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        JsonDocument doc;
        JsonArray namespaces = doc["namespaces"].to<JsonArray>();
        for (IConfigurable* mod : _modules) {
            namespaces.add(mod->getConfigNamespace());
        }
        doc["count"] = (int)_modules.size();

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });
#endif
}

