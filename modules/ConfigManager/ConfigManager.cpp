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
 *              Thread safety: a FreeRTOS mutex protects the
 *              serialize→applyConfig window against concurrent access
 *              from the HTTP task (MCP / REST) and the main loop.
 *
 *              JSON Patch (RFC 6902 subset): patchNamespaceConfig() provides
 *              element-level config addressing for the MCP config_patch tool.
 *
 * Exported Functions/Classes:
 * - ConfigManager: see ConfigManager.hpp
 */

#include "ConfigManager.hpp"
#include <PsychicHttp.h>
#include <string.h>
#include "platform/LogShim.h"
#include "MCPToolRegistry.hpp"

static constexpr const char* TAG = "ConfigManager";

// =============================================================================
// Constructor
// =============================================================================

/**
 * @brief Construct ConfigManager and create the config mutex.
 *
 * The mutex protects serialize/applyConfig pairs against concurrent access
 * from the HTTP task (MCP / REST handlers) and the main loop.
 */
ConfigManager::ConfigManager()
    : _configMutex(xSemaphoreCreateMutex()),
      _persistMutex(xSemaphoreCreateMutex())
{
}

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
    snprintf(buf, size, "/mod/%s.json", ns);
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

    // Load the Three-Tier config manifest (no-op on first boot)
    _manifest.load();

    for (IConfigurable* mod : _modules) {
        loadModuleConfig(mod);
        // Update the Active slot after each module's config is successfully applied
        _manifest.updateActive(mod->getConfigNamespace(), mod->getSchemaVersion());
    }

    // Persist the updated manifest with current Active versions
    _manifest.save();

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
// Serialize Helper
// =============================================================================

/**
 * @brief Serialize a module's current config into a JsonDocument.
 *
 * Extracted from saveModuleConfig() so the serialize step can run
 * under mutex while the persist step runs outside it.
 *
 * @param module Pointer to the IConfigurable module.
 * @param outDoc Output JsonDocument to serialize into (cleared first via to<>).
 * @return true on success.
 */
bool ConfigManager::serializeModuleToDoc(IConfigurable* module, JsonDocument& outDoc) {
    JsonObject root = outDoc.to<JsonObject>();
    module->serializeConfig(root);
    outDoc["_v"] = module->getSchemaVersion();
    return true;
}

// =============================================================================
// Save
// =============================================================================

/**
 * @brief Persist a single module's current configuration to its namespace file.
 *
 * Serializes under the config mutex (if available), then persists to flash
 * outside the lock. No intermediate String buffer — writes directly to the
 * file stream (critical for embedded memory constraints).
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

    // Serialize under mutex (if available), persist outside
    JsonDocument doc;
    if (_configMutex != nullptr) {
        if (xSemaphoreTake(_configMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
            ESP_LOGW(TAG, "saveModuleConfig() — mutex timeout for '%s'", ns);
            return false;
        }
    }
    serializeModuleToDoc(module, doc);
    if (_configMutex != nullptr) {
        xSemaphoreGive(_configMutex);
    }

    // Persist outside lock — guarded by _persistMutex for write ordering
    if (_persistMutex != nullptr) {
        if (xSemaphoreTake(_persistMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
            ESP_LOGW(TAG, "saveModuleConfig() — persist mutex timeout for '%s'", ns);
            return false;
        }
    }
    if (!LittleFS.exists("/mod")) { LittleFS.mkdir("/mod"); }
    File file = LittleFS.open(path, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open %s for writing", path);
        if (_persistMutex != nullptr) xSemaphoreGive(_persistMutex);
        return false;
    }
    if (serializeJson(doc, file) == 0) {
        ESP_LOGE(TAG, "Failed to write config to %s", path);
        file.close();
        if (_persistMutex != nullptr) xSemaphoreGive(_persistMutex);
        return false;
    }
    file.close();
    if (_persistMutex != nullptr) xSemaphoreGive(_persistMutex);

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
 *   - device_id: chip MAC address (or "00:00:00:00:00:00" in native builds)
 *   - device_type: chip model string (or "native" in native builds)
 *   - datetime: ISO-8601 UTC string if system clock is set, else "unknown"
 *   - timezone: TZ string registered via setMetaTimezone(), else "UTC"
 *   - module_count: number of registered IConfigurable modules
 *
 * @param meta Mutable reference to the JsonObject to populate.
 */
void ConfigManager::buildBackupMeta(JsonObject& meta) {
    meta["backup_version"] = 1;

#ifdef BUILD_VERSION
    meta["firmware_version"] = BUILD_VERSION;
#else
    meta["firmware_version"] = "unknown";
#endif

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

        // Deep-merge incoming partial section over current state so that
        // applyConfig() always receives a COMPLETE section — preventing
        // modules that reset-then-load from losing omitted keys.
        // NOTE: Array values are replaced wholesale (ArduinoJson semantics)
        // — element-level merge requires JSON Patch (patchNamespaceConfig).
        if (_configMutex != nullptr) {
            if (xSemaphoreTake(_configMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
                ESP_LOGW(TAG, "importAllConfig() — mutex timeout for '%s'", ns);
                allOk = false;
                continue;
            }
        }

        JsonDocument cur;
        JsonObject curSec = cur.to<JsonObject>();
        mod->serializeConfig(curSec);        // snapshot current state

        // Merge: incoming keys overwrite current; current keys not in
        // incoming are preserved.
        for (JsonPair kv : section) {
            curSec[kv.key()] = kv.value();
        }

        mod->applyConfig(curSec);            // module sees complete section

        if (_configMutex != nullptr) {
            xSemaphoreGive(_configMutex);
        }

        // saveModuleConfig persists outside lock via its own refactored path
        if (!saveModuleConfig(mod)) {
            ESP_LOGE(TAG, "Failed to save imported config for '%s'", ns);
            allOk = false;
        }
    }

    return allOk;
}

bool ConfigManager::exportNamespaceConfig(const char* ns, JsonObject& outSection) const {
    for (const auto* mod : _modules) {
        if (strcmp(mod->getConfigNamespace(), ns) == 0) {
            mod->serializeConfig(outSection);
            return true;
        }
    }
    return false;
}

// =============================================================================
// JSON Patch — Element-Level Config Addressing
// =============================================================================

/**
 * @brief Apply RFC 6902 JSON Patch ops to a single namespace.
 *
 * Atomic: either ALL ops succeed and are persisted, or NONE are.
 * The config mutex is held during the snapshot→apply→serialize
 * window; flash I/O happens outside the lock.
 *
 * Sequence: snapshot → apply ops → applyConfig() → serializeModuleToDoc()
 * (all under mutex) → persist outside lock.
 *
 * @param ns   Namespace to patch (e.g. "pm_channels").
 * @param ops  Array of RFC 6902 op objects.
 * @param result Output: error details and stored values.
 * @param storedValues Optional: if non-null, receives the post-clamp
 *                     values for each patched path.
 * @return true if all ops applied successfully.
 */
bool ConfigManager::patchNamespaceConfig(const char* ns, JsonArrayConst ops,
                                         JsonPatch::PatchResult& result,
                                         JsonDocument* storedValues) {
    // Find module by namespace
    IConfigurable* mod = nullptr;
    for (auto* m : _modules) {
        if (strcmp(m->getConfigNamespace(), ns) == 0) { mod = m; break; }
    }
    if (!mod) {
        result.error = JsonPatch::Error::PATH_NOT_FOUND;
        result.applied = false;
        snprintf(result.path, sizeof(result.path), "(namespace '%s' not found)", ns);
        return false;
    }

    if (static_cast<int>(ops.size()) > JsonPatch::MAX_PATCH_OPS) {
        result.error = JsonPatch::Error::TOO_MANY_OPS;
        result.applied = false;
        return false;
    }

    // Acquire mutex
    if (_configMutex != nullptr) {
        if (xSemaphoreTake(_configMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
            result.error = JsonPatch::Error::MUTEX_TIMEOUT;
            result.applied = false;
            return false;
        }
    }

    // 1. Snapshot current state
    JsonDocument cur;
    JsonObject sec = cur.to<JsonObject>();
    mod->serializeConfig(sec);

    // 2. Apply every op to the scratch copy
    int i = 0;
    for (JsonObjectConst op : ops) {
        JsonPatch::Error err = JsonPatch::applyOne(sec, op, result.path, sizeof(result.path));
        if (err != JsonPatch::Error::OK) {
            result.error = err;
            result.opIndex = i;
            result.applied = false;
            if (_configMutex != nullptr) xSemaphoreGive(_configMutex);
            return false;
        }
        ++i;
    }

    // 3. Commit — module ingests, clamps, normalises
    mod->applyConfig(sec);

    // 4. Serialize post-apply state (for response + persist) — still under lock
    JsonDocument postDoc;
    serializeModuleToDoc(mod, postDoc);

    if (_configMutex != nullptr) xSemaphoreGive(_configMutex);

    // 5. Extract stored values for patched paths (outside lock)
    if (storedValues != nullptr) {
        JsonVariantConst postRoot = postDoc.as<JsonObjectConst>();
        JsonObject stored = storedValues->to<JsonObject>();
        for (JsonObjectConst op : ops) {
            const char* opPath = op["path"] | "";
            const char* opType = op["op"] | "";
            // Only extract stored values for mutating ops (not test)
            if (strcmp(opType, "test") == 0) continue;
            JsonVariantConst val;
            if (JsonPatch::resolveValue(postRoot, opPath, val)) {
                stored[opPath] = val;
            }
        }
    }

    // 6. Persist outside lock — guarded by _persistMutex for write ordering
    const char* modNs = mod->getConfigNamespace();
    char filePath[48];
    getConfigPath(modNs, filePath, sizeof(filePath));
    if (_persistMutex != nullptr) {
        if (xSemaphoreTake(_persistMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
            ESP_LOGW(TAG, "patchNamespaceConfig — persist mutex timeout for '%s'", ns);
            // Applied in RAM but can't persist — will revert on reboot
            result.error = JsonPatch::Error::PERSIST_FAILED;
            result.applied = true;
            return false;
        }
    }
    if (!LittleFS.exists("/mod")) { LittleFS.mkdir("/mod"); }
    File file = LittleFS.open(filePath, "w");
    if (!file) {
        ESP_LOGE(TAG, "patchNamespaceConfig — failed to open %s for writing", filePath);
        if (_persistMutex != nullptr) xSemaphoreGive(_persistMutex);
        // Applied in RAM but flash write failed — will revert on reboot
        result.error = JsonPatch::Error::PERSIST_FAILED;
        result.applied = true;
        return false;
    }
    if (serializeJson(postDoc, file) == 0) {
        ESP_LOGE(TAG, "patchNamespaceConfig — failed to write %s", filePath);
        file.close();
        if (_persistMutex != nullptr) xSemaphoreGive(_persistMutex);
        // Applied in RAM but flash write failed — will revert on reboot
        result.error = JsonPatch::Error::PERSIST_FAILED;
        result.applied = true;
        return false;
    }
    file.close();
    if (_persistMutex != nullptr) xSemaphoreGive(_persistMutex);

    result.applied = true;
    result.opsApplied = i;
    ESP_LOGI(TAG, "Patched '%s': %d ops applied", ns, i);
    return true;
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
void ConfigManager::registerRoutes(PsychicHttpServer& server) {
    // --- GET /api/config/backup — export all config as downloadable JSON ---
    // IMPORTANT: registered BEFORE /api/config to prevent prefix-match swallowing.
    server.on("/api/config/backup", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        exportAllConfig(doc);

        std::string output;
        serializeJson(doc, output);
        response->addHeader("Content-Disposition", "attachment; filename=\"config-backup.json\"");
        response->send(200, "application/json", output.c_str());

        ESP_LOGI(TAG, "Config backup exported");
        return ESP_OK;
    });

    // --- POST /api/config/restore — upload JSON, validate, import, restart ---
    // IMPORTANT: registered BEFORE /api/config to prevent prefix-match swallowing.
    server.on("/api/config/restore", HTTP_POST,
        [this](PsychicRequest *request, PsychicResponse* response) {

            // --- Body size guard (8KB max) ---
            constexpr size_t MAX_RESTORE_BODY = 8192;
            if (request->contentLength() > MAX_RESTORE_BODY) {
                response->send(413, "application/json",
                    "{\"error\":\"Payload too large\",\"max\":8192}");
                return ESP_FAIL;
            }

            const char* body = request->bodyCStr();
            if (!body || strlen(body) == 0) {
                response->send(400, "application/json", "{\"error\":\"body_required\"}");
                return ESP_FAIL;
            }

            // --- Parse the uploaded JSON ---
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, body);
            if (err) {
                ESP_LOGE("ConfigManager", "Restore JSON parse failed: %s", err.c_str());
                std::string errMsg = "{\"error\":\"Invalid JSON: ";
                errMsg += err.c_str();
                errMsg += "\"}";
                response->send(400, "application/json", errMsg.c_str());
                return ESP_FAIL;
            }

            // --- Validate: must be a JSON object with at least one key ---
            if (!doc.is<JsonObject>() || doc.as<JsonObject>().size() == 0) {
                response->send(400, "application/json",
                    "{\"error\":\"Config must be a non-empty JSON object\"}");
                return ESP_FAIL;
            }

            // --- Import the config ---
            bool success = importAllConfig(doc);
            if (!success) {
                ESP_LOGW("ConfigManager", "Some modules failed during config import");
            }

            ESP_LOGI("ConfigManager", "Config restore complete — scheduling restart");

            // Send response before restart
            response->send(200, "application/json",
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
            return ESP_OK;
        }
    );

    // --- GET /api/config — list registered namespaces ---
    // IMPORTANT: registered AFTER specific sub-paths to prevent prefix matching
    // from swallowing /api/config/backup and /api/config/restore requests.
    server.on("/api/config", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        JsonArray namespaces = doc["namespaces"].to<JsonArray>();
        for (IConfigurable* mod : _modules) {
            namespaces.add(mod->getConfigNamespace());
        }
        doc["count"] = (int)_modules.size();

        std::string output;
        serializeJson(doc, output);
        response->send(200, "application/json", output.c_str());
        return ESP_OK;
    });
}


// =============================================================================
// IMCPContributor — MCP Tool Registration
// =============================================================================

/**
 * @brief Register ConfigManager-specific MCP tools.
 *
 * Two complementary tools are registered:
 *   - config_get_schema_version (read-only): Returns the system-level config
 *     backup schema version. This is the envelope version used by
 *     exportAllConfig() / importAllConfig(), NOT individual module _v values.
 *   - config_factory_reset (mutating): Removes all persisted module config
 *     files from the /mod/ directory and schedules a device reboot. On restart,
 *     loadModuleConfig() will find no files and call resetConfig() on each
 *     module, restoring factory defaults.
 *
 * Both tools are using native test stubs — no hardware APIs are
 * available in the native test environment.
 *
 * @param registry The MCPToolRegistry to register tools with.
 */
void ConfigManager::registerMCPTools(MCPToolRegistry& registry) {
    // --- config_get_schema_version: read-only system config schema version ---
    registry.addTool("config_get_schema_version",
        "Get the system-level config backup schema version",
        [](void* /*context*/, JsonObject& args, JsonObject& result) {
            (void)args; // No input parameters
            // This is the envelope version used by exportAllConfig()/importAllConfig(),
            // matching the backup_version field in the _meta envelope.
            result["version"] = 1;
        }, nullptr, false);

    // --- config_factory_reset: wipe all config and reboot ---
    registry.addTool("config_factory_reset",
        "Wipe all persisted module configuration and reboot to factory defaults. "
        "WARNING: This is destructive and cannot be undone.",
        [](void* context, JsonObject& args, JsonObject& result) {
            (void)args; // No input parameters
            ConfigManager* self = static_cast<ConfigManager*>(context);

            ESP_LOGW(TAG, "Factory reset requested via MCP — wiping all config files");

            // Delete every registered module's config file from /mod/
            for (IConfigurable* mod : self->_modules) {
                char path[48];
                getConfigPath(mod->getConfigNamespace(), path, sizeof(path));
                if (LittleFS.exists(path)) {
                    LittleFS.remove(path);
                    ESP_LOGI(TAG, "Removed config file: %s", path);
                }
            }

            // Also remove the config manifest file
            if (LittleFS.exists("/config_manifest.json")) {
                LittleFS.remove("/config_manifest.json");
                ESP_LOGI(TAG, "Removed config manifest file");
            }

            result["success"] = true;
            result["message"] = "Config wiped, restarting";

            // Schedule restart (500ms delay for response to flush)
            TimerHandle_t restartTimer = xTimerCreate(
                "cfg_factory_rst", pdMS_TO_TICKS(500), pdFALSE, nullptr,
                [](TimerHandle_t t) {
                    ESP.restart();
                }
            );
            if (restartTimer != nullptr) {
                xTimerStart(restartTimer, 0);
            }
        }, this, true);
}
