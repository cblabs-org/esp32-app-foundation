/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/MCPServer/MCPBuiltinTools.cpp
 * Description: Implementation of all foundation-level MCP tools that are always
 *              available in the framework. Registers 7 tools into the MCPToolRegistry:
 *                - system_info:      Device heap, uptime, chip info, firmware version
 *                - get_config:       Read all module configs via gateway export
 *                - set_config:       Update module configs via gateway import
 *                - patch_config:     Apply RFC 6902 JSON Patch ops to a single config namespace
 *                - reboot:           Restart the ESP32 device
 *                - get_logs:         Placeholder for future log ring-buffer
 *                - list_tools:       Enumerate all registered MCP tools
 *
 * Exported Functions:
 * - mcpBuiltinTools::registerAll(): see MCPBuiltinTools.hpp
 */

#include "MCPBuiltinTools.hpp"
#include "MCPToolRegistry.hpp"
#include "ConfigManager.hpp"
#include "JsonPatch.hpp"
#include "TimeManager.hpp"
#include "DataManager.hpp"
#include "events/DataSourceHealthPayload.hpp"
#include <ArduinoJson.h>

// ---------------------------------------------------------------------------
// Platform-conditional includes & logging shims
// Matches the Application.cpp pattern: ESP-IDF logs on hardware, printf on native.
// ---------------------------------------------------------------------------
    #include <Arduino.h>
    #include "esp_system.h"
    #include "esp_heap_caps.h"
    #include "esp_chip_info.h"
    #include "esp_partition.h"
    #include "esp_ota_ops.h"
    #include <LittleFS.h>
    #include <WiFi.h>
    #include <base64.h>

#include "platform/LogShim.h"

static constexpr const char* TAG = "MCPBuiltinTools";

// ---------------------------------------------------------------------------
// Base64 lookup table (hardware uses the ESP32 base64 library; native needs
// a minimal inline implementation to avoid external dependencies).
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
// File-scope context for built-in tools
// ---------------------------------------------------------------------------
// Multiple built-in tools need access to ConfigManager, TimeManager, and/or
// the MCPToolRegistry. Rather than using capturing lambdas (which require
// std::function and heap allocation), we store these pointers in a file-scope
// struct and pass its address as the void* context to each handler.

/** @brief Bundles all dependency pointers used by built-in tool handlers. */
struct BuiltinToolsContext {
    ConfigManager*  configManager = nullptr;
    TimeManager*    timeManager   = nullptr;
    DataManager*    dataManager   = nullptr;
    MCPToolRegistry* registry    = nullptr;
};

static BuiltinToolsContext s_builtinCtx;

// ---------------------------------------------------------------------------
// Built-in tool registration
// ---------------------------------------------------------------------------

void mcpBuiltinTools::registerAll(MCPToolRegistry& registry,
                                  ConfigManager* configManager,
                                  TimeManager* timeManager,
                                  DataManager* dataManager) {

    // Populate the file-scope context struct
    s_builtinCtx.configManager = configManager;
    s_builtinCtx.timeManager   = timeManager;
    s_builtinCtx.dataManager   = dataManager;
    s_builtinCtx.registry      = &registry;

    // -----------------------------------------------------------------
    // Tool 1: system_info — Device system telemetry
    // -----------------------------------------------------------------
    registry.addTool(
        "system_info",
        "Get device system information including heap, storage, WiFi, time, uptime, chip info, and firmware version",
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* c = static_cast<BuiltinToolsContext*>(ctx);
            (void)params; // No input parameters required
                // --- Internal DRAM metrics (always present on all ESP32 variants) ---
                result["heap_free"]          = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
                result["heap_total"]         = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
                result["heap_min_free"]      = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
                result["heap_max_alloc"]     = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);

                // --- PSRAM metrics (returns 0 on boards without PSRAM) ---
                result["psram_free"]         = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                result["psram_total"]        = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);

                // --- Combined totals (DEFAULT = internal + PSRAM if available) ---
                result["heap_default_free"]  = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
                result["heap_default_total"] = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);

                // --- Storage metrics (LittleFS) ---
                result["storage_used"]       = LittleFS.usedBytes();
                result["storage_total"]      = LittleFS.totalBytes();

                // --- Temperature (internal sensor) ---
                float chipTemp = temperatureRead();
                result["chip_temp"]          = chipTemp;

                // --- Uptime ---
                result["uptime_seconds"]     = millis() / 1000;

                // --- Firmware / Build info ---
                result["build_serial"]       = __DATE__ " " __TIME__;
                result["sdk_version"]        = ESP.getSdkVersion();

                // --- Chip info ---
                esp_chip_info_t chipInfo;
                esp_chip_info(&chipInfo);
                result["chip_model"]         = ESP.getChipModel();
                result["chip_revision"]      = chipInfo.revision;
                result["chip_cores"]         = chipInfo.cores;
                result["flash_size"]         = ESP.getFlashChipSize();

                // --- WiFi info ---
                if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
                    result["ap_mode"]   = true;
                    result["wifi_ssid"] = WiFi.softAPSSID();
                    result["wifi_ip"]   = WiFi.softAPIP().toString();
                    result["wifi_mac"]  = WiFi.softAPmacAddress();
                    result["wifi_rssi"] = 0;
                } else {
                    result["ap_mode"]   = false;
                    result["wifi_ssid"] = WiFi.SSID();
                    result["wifi_ip"]   = WiFi.localIP().toString();
                    result["wifi_mac"]  = WiFi.macAddress();
                    result["wifi_rssi"] = WiFi.RSSI();
                }

                // --- Active partition ---
                const esp_partition_t* running = esp_ota_get_running_partition();
                if (running != nullptr) {
                    result["partition"] = running->label;
                }

                // --- Time info (NTP sync status, timezone, local time) ---
                if (c->timeManager != nullptr) {
                    result["time_configured"] = c->timeManager->isConfigured();
                    result["time_synced"]     = c->timeManager->isTimeSet();
                    result["time_timezone"]   = c->timeManager->getTimezone();
                    result["time_posix"]      = c->timeManager->getPosixTz();

                    const char* syncStr = "UNSYNCED";
                    if (c->timeManager->getSyncStatus() == TimeSyncStatus::SYNCED) syncStr = "SYNCED";
                    else if (c->timeManager->getSyncStatus() == TimeSyncStatus::IN_PROGRESS) syncStr = "IN_PROGRESS";
                    result["time_sync_status"] = syncStr;

                    if (c->timeManager->isTimeSet()) {
                        struct tm timeinfo;
                        if (c->timeManager->getLocalTime(&timeinfo)) {
                            char buf[32];
                            strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
                            result["time_local"] = buf;
                        }
                    }
                }
            result["firmware_version"] = BUILD_VERSION;
        },
        &s_builtinCtx  // context: BuiltinToolsContext*
    );

    // -----------------------------------------------------------------
    // Tool 2: get_config — Read system configuration (all or single ns)
    // -----------------------------------------------------------------

    // Build the input schema — optional 'namespace' filter
    JsonDocument getConfigSchema;
    getConfigSchema["type"] = "object";
    JsonObject getProps = getConfigSchema["properties"].to<JsonObject>();

    JsonObject nsParam = getProps["namespace"].to<JsonObject>();
    nsParam["type"]        = "string";
    nsParam["description"] = "Optional namespace to query a single module (e.g. \"wifi\"). Omit to return all namespaces.";

    registry.addTool(
        "get_config",
        "Get system configuration. Optionally pass 'namespace' to read a single module's config instead of all modules.",
        getConfigSchema,
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* c = static_cast<BuiltinToolsContext*>(ctx);
            if (c->configManager == nullptr) {
                result["error"] = "ConfigManager not available";
                result["isError"] = true;
                return;
            }

            // Namespace-scoped query
            if (params["namespace"].is<const char*>()) {
                const char* ns = params["namespace"].as<const char*>();
                JsonObject nsSec = result[ns].to<JsonObject>();
                if (!c->configManager->exportNamespaceConfig(ns, nsSec)) {
                    result.remove(ns);
                    result["error"]   = String("Unknown namespace: ") + ns;
                    result["isError"] = true;
                }
                return;
            }

            // All-namespace export (existing behaviour)
            JsonDocument exportDoc;
            c->configManager->exportAllConfig(exportDoc);

            // Copy each namespace section into the result
            for (JsonPair kv : exportDoc.as<JsonObject>()) {
                result[kv.key()] = kv.value();
            }
        },
        &s_builtinCtx  // context: BuiltinToolsContext*
    );

    // -----------------------------------------------------------------
    // Tool 3: set_config — Update system configuration fields
    // -----------------------------------------------------------------

    // Build the input schema — accepts namespace-keyed config sections
    JsonDocument setConfigSchema;
    setConfigSchema["type"] = "object";
    JsonObject props = setConfigSchema["properties"].to<JsonObject>();

    JsonObject configP = props["config"].to<JsonObject>();
    configP["type"]        = "object";
    configP["description"] = "Configuration object with namespace-keyed sections (e.g. {\"wifi\":{\"ssid\":\"MyNetwork\"}})";

    registry.addTool(
        "set_config",
        "Update system configuration fields by namespace. Returns {status, updated_namespaces} — does NOT echo the full config.",
        setConfigSchema,
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* c = static_cast<BuiltinToolsContext*>(ctx);
            if (c->configManager == nullptr) {
                result["error"] = "ConfigManager not available";
                result["isError"] = true;
                return;
            }

            if (!params["config"].is<JsonObject>()) {
                result["error"] = "Missing 'config' object parameter";
                result["isError"] = true;
                return;
            }

            // Import the provided config sections via the gateway
            JsonDocument importDoc;
            JsonObject configObj = params["config"].as<JsonObject>();
            for (JsonPair kv : configObj) {
                importDoc[kv.key()] = kv.value();
            }

            bool success = c->configManager->importAllConfig(importDoc);
            ESP_LOGI(TAG, "Configuration updated via MCP");

            result["status"] = success ? "updated" : "partial_failure";

            // Report which namespaces were updated, not the entire config
            JsonObject updated = result["updated_namespaces"].to<JsonObject>();
            for (JsonPair kv : configObj) {
                updated[kv.key()] = true;
            }
        },
        &s_builtinCtx,  // context: BuiltinToolsContext*
        true  // mutatesState — writes config via ConfigManager
    );

    // -----------------------------------------------------------------
    // Tool 4: patch_config — Apply RFC 6902 JSON Patch to a namespace
    // -----------------------------------------------------------------

    // Build the input schema — namespace string + ops array
    JsonDocument patchConfigSchema;
    patchConfigSchema["type"] = "object";
    JsonObject patchProps = patchConfigSchema["properties"].to<JsonObject>();

    JsonObject patchNsParam = patchProps["namespace"].to<JsonObject>();
    patchNsParam["type"]        = "string";
    patchNsParam["description"] = "Config namespace to patch (e.g. \"wifi\", \"pm_channels\")";

    JsonObject patchOpsParam = patchProps["ops"].to<JsonObject>();
    patchOpsParam["type"]        = "array";
    patchOpsParam["description"] = "Array of RFC 6902 JSON Patch ops. Each op: {\"op\": \"replace\"|\"add\"|\"test\"|\"remove\", \"path\": \"/json/pointer\", \"value\": ...}. Max 32 ops. Atomic: all succeed or none are applied.";
    JsonObject patchOpsItems = patchOpsParam["items"].to<JsonObject>();
    patchOpsItems["type"] = "object";

    JsonArray patchRequired = patchConfigSchema["required"].to<JsonArray>();
    patchRequired.add("namespace");
    patchRequired.add("ops");

    registry.addTool(
        "patch_config",
        "Apply RFC 6902 JSON Patch ops to a single config namespace. Atomic: all ops succeed or none are applied. Returns stored (post-validation) values for patched paths.",
        patchConfigSchema,
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* c = static_cast<BuiltinToolsContext*>(ctx);
            if (c->configManager == nullptr) {
                result["error"] = "ConfigManager not available";
                result["isError"] = true;
                return;
            }

            // Validate namespace parameter
            if (!params["namespace"].is<const char*>()) {
                result["error"] = "Missing or invalid 'namespace' string parameter";
                result["isError"] = true;
                return;
            }

            // Validate ops parameter
            if (!params["ops"].is<JsonArray>()) {
                result["error"] = "Missing or invalid 'ops' array parameter";
                result["isError"] = true;
                return;
            }

            const char* ns = params["namespace"].as<const char*>();
            JsonArrayConst ops = params["ops"].as<JsonArrayConst>();

            JsonPatch::PatchResult patchResult;
            JsonDocument storedDoc;

            bool ok = c->configManager->patchNamespaceConfig(ns, ops, patchResult, &storedDoc);

            if (ok) {
                result["status"]      = "patched";
                result["namespace"]   = ns;
                result["ops_applied"] = patchResult.opsApplied;

                // Copy stored (post-validation) values into result
                JsonObject storedObj = result["results"].to<JsonObject>();
                for (JsonPair kv : storedDoc.as<JsonObject>()) {
                    storedObj[kv.key()] = kv.value();
                }
            } else {
                // Map error enum to human-readable message
                const char* errMsg = "Unknown error";
                switch (patchResult.error) {
                    case JsonPatch::Error::INVALID_POINTER:
                        errMsg = "Invalid JSON Pointer"; break;
                    case JsonPatch::Error::PATH_NOT_FOUND:
                        errMsg = "Path not found"; break;
                    case JsonPatch::Error::INDEX_OUT_OF_RANGE:
                        errMsg = "Array index out of range"; break;
                    case JsonPatch::Error::ARRAY_REMOVE_BLOCKED:
                        errMsg = "Remove on array index not supported (use replace with sentinel)"; break;
                    case JsonPatch::Error::ARRAY_INSERT_BLOCKED:
                        errMsg = "Indexed array insert not supported (use '-' to append)"; break;
                    case JsonPatch::Error::TEST_FAILED:
                        errMsg = "Test op failed — value mismatch (batch aborted)"; break;
                    case JsonPatch::Error::UNKNOWN_OP:
                        errMsg = "Unknown op (supported: replace, add, test, remove)"; break;
                    case JsonPatch::Error::TOO_MANY_OPS:
                        errMsg = "Too many ops (max 32)"; break;
                    case JsonPatch::Error::MUTEX_TIMEOUT:
                        errMsg = "Config mutex busy — try again"; break;
                    case JsonPatch::Error::PERSIST_FAILED:
                        errMsg = "Applied in memory but flash write failed — will revert on reboot"; break;
                    case JsonPatch::Error::MISSING_VALUE:
                        errMsg = "Missing 'value' field (required for replace, add, test)"; break;
                    default:
                        break;
                }
                result["error"]     = errMsg;
                result["failed_op"] = patchResult.opIndex;
                result["path"]      = patchResult.path;
                result["applied"]   = patchResult.applied;  // true for PERSIST_FAILED (applied in RAM)
                result["isError"]   = true;
            }
        },
        &s_builtinCtx,  // context: BuiltinToolsContext*
        true  // mutatesState — writes config via ConfigManager
    );

    // -----------------------------------------------------------------
    // Tool 5: reboot — Restart the ESP32 device
    // -----------------------------------------------------------------
    registry.addTool(
        "reboot",
        "Restart the ESP32 device",
        [](void* /*ctx*/, JsonObject& params, JsonObject& result) {
            (void)params; // No input parameters required
            result["status"]  = "rebooting";
            result["message"] = "Device will restart in 1 second";

                // Flush the JSON-RPC response before restarting.
                // The serial output is written synchronously, so by the
                // time we reach here the response has already been serialized.
                ESP_LOGI(TAG, "Reboot requested via MCP — restarting in 1s");
                delay(1000);
                esp_restart();
        },
        nullptr,  // context: not needed
        true  // mutatesState — restarts the device
    );

    // -----------------------------------------------------------------
    // Tool 6: get_logs — Placeholder for future log ring-buffer
    // -----------------------------------------------------------------

    // Build input schema with optional 'count' parameter
    JsonDocument getLogsSchema;
    getLogsSchema["type"] = "object";
    JsonObject logsProps = getLogsSchema["properties"].to<JsonObject>();

    JsonObject countP = logsProps["count"].to<JsonObject>();
    countP["type"]        = "integer";
    countP["description"] = "Number of recent log lines to retrieve (default 20)";

    registry.addTool(
        "get_logs",
        "Get recent log entries from the device log buffer",
        getLogsSchema,
        [](void* /*ctx*/, JsonObject& params, JsonObject& result) {
            // v1 stub — the full ring-buffer logger is a future enhancement.
            // When implemented, this will read the last N entries from a
            // circular log buffer and return them as a JSON array.
            int count = params["count"] | 20; // Default to 20 lines
            (void)count; // Suppress unused-variable warning until buffer is implemented
            result["logs"] = "Log buffer not yet implemented (PR-05b placeholder)";
            result["note"] = "A ring-buffer logger will be added in a future PR";
        }
    );

    // -----------------------------------------------------------------
    // Tool 7: list_tools — Enumerate all registered MCP tools
    // -----------------------------------------------------------------
    registry.addTool(
        "list_tools",
        "List all registered MCP tools with their descriptions and schemas",
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* c = static_cast<BuiltinToolsContext*>(ctx);
            (void)params; // No input parameters required
            JsonArray toolsArray = result["tools"].to<JsonArray>();

            for (const auto& tool : c->registry->getTools()) {
                JsonObject toolObj     = toolsArray.add<JsonObject>();
                toolObj["name"]        = tool.name;
                toolObj["description"] = tool.description;
            }

            result["count"] = c->registry->size();
        },
        &s_builtinCtx  // context: BuiltinToolsContext*
    );

    // -----------------------------------------------------------------
    // Tool 8: get_data_sources — Retrieve DataManager health states
    // -----------------------------------------------------------------
    registry.addTool(
        "get_data_sources",
        "Returns current state, failure counts, backoffs for all data sources",
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* c = static_cast<BuiltinToolsContext*>(ctx);
            (void)params; // No input parameters required

            if (c->dataManager == nullptr) {
                result["error"] = "DataManager not available";
                result["isError"] = true;
                return;
            }

            JsonArray sourcesArray = result["sources"].to<JsonArray>();
            size_t count = c->dataManager->getSourceCount();
            for (size_t i = 0; i < count; ++i) {
                SourceHealth health;
                if (c->dataManager->getSourceHealthAt(i, health)) {
                    JsonObject src = sourcesArray.add<JsonObject>();
                    src["name"] = health.name;
                    
                    src["state"] = sourceStateToString(health.state);

                    // Last fetch result and lifetime success flag (#170)
                    static const char* statusNames[] = {
                        "Success", "NoChange", "RateLimited",
                        "FetchError", "ParseError", "AuthError", "Skipped"
                    };
                    uint8_t statusIdx = static_cast<uint8_t>(health.lastStatus);
                    src["last_status"] = (statusIdx < 7) ? statusNames[statusIdx] : "Unknown";
                    src["ever_succeeded"] = health.everSucceeded;
                    
                    src["consecutive_failures"] = health.consecutiveFailures;
                    src["ms_since_last_fetch"] = health.lastAttemptMs == 0 ? 0 : millis() - health.lastAttemptMs;
                    src["current_backoff_ms"] = health.currentBackoffMs;
                }
            }
        },
        &s_builtinCtx
    );

    ESP_LOGI(TAG, "Registered %zu built-in tools", registry.size());
}
