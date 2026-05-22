/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/mcpServer/mcpBuiltinTools.cpp
 * Description: Implementation of all foundation-level MCP tools that are always
 *              available in the framework. Registers 7 tools into the McpToolRegistry:
 *                - system_info:      Device heap, uptime, chip info, firmware version
 *                - get_config:       Read current SystemConfig fields
 *                - set_config:       Update SystemConfig fields and persist
 *                - reboot:           Restart the ESP32 device
 *                - get_logs:         Placeholder for future log ring-buffer
 *                - list_tools:       Enumerate all registered MCP tools
 *                - capture_display:  Capture the display framebuffer as base64-encoded XBM
 *
 * Exported Functions:
 * - mcpBuiltinTools::registerAll(): see mcpBuiltinTools.hpp
 */

#include "mcpBuiltinTools.hpp"
#include "mcpToolRegistry.hpp"
#include <configManager.hpp>
#include <ArduinoJson.h>

// ---------------------------------------------------------------------------
// Platform-conditional includes & logging shims
// Matches the Application.cpp pattern: ESP-IDF logs on hardware, printf on native.
// ---------------------------------------------------------------------------
#ifndef NATIVE_BUILD
    #include <Arduino.h>
    #include "esp_system.h"
    #include <base64.h>
#endif

#include "platform/LogShim.h"

static const char* TAG = "McpBuiltinTools";

// ---------------------------------------------------------------------------
// Base64 lookup table (hardware uses the ESP32 base64 library; native needs
// a minimal inline implementation to avoid external dependencies).
// ---------------------------------------------------------------------------
#ifdef NATIVE_BUILD
namespace {

// Standard Base64 alphabet
static const char kBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * @brief Encode a byte array to Base64 (native build only).
 *
 * On hardware the ESP32 base64 library is used instead. This minimal
 * implementation avoids pulling in any additional native dependencies.
 *
 * @param data  Pointer to the raw byte array.
 * @param len   Length of the byte array.
 * @return Base64-encoded std::string.
 */
std::string base64Encode(const uint8_t* data, size_t len) {
    std::string result;
    result.reserve(((len + 2) / 3) * 4); // Pre-allocate to avoid reallocation

    for (size_t i = 0; i < len; i += 3) {
        // --- Pack up to 3 input bytes into a 24-bit value ---
        uint32_t n = static_cast<uint32_t>(data[i]) << 16;
        if (i + 1 < len) n |= static_cast<uint32_t>(data[i + 1]) << 8;
        if (i + 2 < len) n |= static_cast<uint32_t>(data[i + 2]);

        // --- Extract 4 six-bit groups from the 24-bit value ---
        result += kBase64Chars[(n >> 18) & 0x3F];
        result += kBase64Chars[(n >> 12) & 0x3F];
        result += (i + 1 < len) ? kBase64Chars[(n >> 6) & 0x3F] : '=';
        result += (i + 2 < len) ? kBase64Chars[n & 0x3F] : '=';
    }
    return result;
}

} // anonymous namespace
#endif

// ---------------------------------------------------------------------------
// Built-in tool registration
// ---------------------------------------------------------------------------

void mcpBuiltinTools::registerAll(McpToolRegistry& registry,
                                  ConfigManager* configManager,
                                  const DisplayInfo* displayInfo) {

    // -----------------------------------------------------------------
    // Tool 1: system_info — Device system telemetry
    // -----------------------------------------------------------------
    registry.addTool(
        "system_info",
        "Get device system information including heap, uptime, chip info, and firmware version",
        [](JsonObject& params, JsonObject& result) {
            (void)params; // No input parameters required
            #ifndef NATIVE_BUILD
                result["heap_free"]      = ESP.getFreeHeap();
                result["heap_max_alloc"] = ESP.getMaxAllocHeap();
                result["uptime_seconds"] = millis() / 1000;
                result["chip_model"]     = ESP.getChipModel();
                result["chip_cores"]     = ESP.getChipCores();
                result["flash_size"]     = ESP.getFlashChipSize();
            #else
                // Native build stubs — no hardware telemetry available
                result["heap_free"]      = 0;
                result["heap_max_alloc"] = 0;
                result["uptime_seconds"] = 0;
                result["chip_model"]     = "native";
                result["chip_cores"]     = 0;
                result["flash_size"]     = 0;
            #endif
            result["firmware_version"] = APP_FOUNDATION_VERSION;
        }
    );

    // -----------------------------------------------------------------
    // Tool 2: get_config — Read current system configuration
    // -----------------------------------------------------------------
    registry.addTool(
        "get_config",
        "Get the current system configuration",
        [configManager](JsonObject& params, JsonObject& result) {
            (void)params; // No input parameters required
            if (configManager == nullptr) {
                result["error"] = "ConfigManager not available";
                result["isError"] = true;
                return;
            }

            const SystemConfig& cfg = configManager->getConfig();
            result["hostname"]                = cfg.hostname;
            result["timezone"]                = cfg.timezone;
            result["firmwareUpdatesEnabled"]  = cfg.firmwareUpdatesEnabled;
            result["otaQuietHour"]            = cfg.otaQuietHour;
            result["configVersion"]           = cfg.configVersion;
        }
    );

    // -----------------------------------------------------------------
    // Tool 3: set_config — Update system configuration fields
    // -----------------------------------------------------------------

    // Build the input schema with optional properties
    JsonDocument setConfigSchema;
    setConfigSchema["type"] = "object";
    JsonObject props = setConfigSchema["properties"].to<JsonObject>();

    JsonObject hostnameP = props["hostname"].to<JsonObject>();
    hostnameP["type"]        = "string";
    hostnameP["description"] = "Device hostname (max 32 chars)";

    JsonObject timezoneP = props["timezone"].to<JsonObject>();
    timezoneP["type"]        = "string";
    timezoneP["description"] = "POSIX timezone string (max 63 chars)";

    JsonObject fwUpdatesP = props["firmwareUpdatesEnabled"].to<JsonObject>();
    fwUpdatesP["type"]        = "boolean";
    fwUpdatesP["description"] = "Enable or disable firmware OTA updates";

    JsonObject otaQuietP = props["otaQuietHour"].to<JsonObject>();
    otaQuietP["type"]        = "integer";
    otaQuietP["description"] = "Hour (0-23) during which OTA checks are suppressed";

    registry.addTool(
        "set_config",
        "Update system configuration fields",
        setConfigSchema,
        [configManager](JsonObject& params, JsonObject& result) {
            if (configManager == nullptr) {
                result["error"] = "ConfigManager not available";
                result["isError"] = true;
                return;
            }

            SystemConfig& cfg = configManager->getMutableConfig();

            // --- Apply optional fields (ArduinoJson v7: use .is<T>() instead of containsKey) ---
            if (params["hostname"].is<const char*>()) {
                strncpy(cfg.hostname, params["hostname"].as<const char*>(), 32);
                cfg.hostname[32] = '\0'; // Null-terminate safety
            }
            if (params["timezone"].is<const char*>()) {
                strncpy(cfg.timezone, params["timezone"].as<const char*>(), 63);
                cfg.timezone[63] = '\0'; // Null-terminate safety
            }
            if (params["firmwareUpdatesEnabled"].is<bool>()) {
                cfg.firmwareUpdatesEnabled = params["firmwareUpdatesEnabled"].as<bool>();
            }
            if (params["otaQuietHour"].is<int>()) {
                cfg.otaQuietHour = params["otaQuietHour"].as<int>();
            }

            // --- Persist to flash ---
            configManager->saveConfig();
            ESP_LOGI(TAG, "Configuration updated and saved");

            // --- Return updated config in result ---
            result["hostname"]                = cfg.hostname;
            result["timezone"]                = cfg.timezone;
            result["firmwareUpdatesEnabled"]  = cfg.firmwareUpdatesEnabled;
            result["otaQuietHour"]            = cfg.otaQuietHour;
            result["configVersion"]           = cfg.configVersion;
            result["status"]                  = "updated";
        }
    );

    // -----------------------------------------------------------------
    // Tool 4: reboot — Restart the ESP32 device
    // -----------------------------------------------------------------
    registry.addTool(
        "reboot",
        "Restart the ESP32 device",
        [](JsonObject& params, JsonObject& result) {
            (void)params; // No input parameters required
            result["status"]  = "rebooting";
            result["message"] = "Device will restart in 1 second";

            #ifndef NATIVE_BUILD
                // Flush the JSON-RPC response before restarting.
                // The serial output is written synchronously, so by the
                // time we reach here the response has already been serialized.
                ESP_LOGI(TAG, "Reboot requested via MCP — restarting in 1s");
                delay(1000);
                esp_restart();
            #else
                ESP_LOGI(TAG, "Reboot requested via MCP (native build — no-op)");
            #endif
        }
    );

    // -----------------------------------------------------------------
    // Tool 5: get_logs — Placeholder for future log ring-buffer
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
        [](JsonObject& params, JsonObject& result) {
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
    // Tool 6: list_tools — Enumerate all registered MCP tools
    // -----------------------------------------------------------------
    // Capture a pointer to the registry so the lambda can read back all
    // tools at call time (after all contributors have been registered).
    McpToolRegistry* registryPtr = &registry;

    registry.addTool(
        "list_tools",
        "List all registered MCP tools with their descriptions and schemas",
        [registryPtr](JsonObject& params, JsonObject& result) {
            (void)params; // No input parameters required
            JsonArray toolsArray = result["tools"].to<JsonArray>();

            for (const auto& tool : registryPtr->getTools()) {
                JsonObject toolObj     = toolsArray.add<JsonObject>();
                toolObj["name"]        = tool.name;
                toolObj["description"] = tool.description;
            }

            result["count"] = registryPtr->size();
        }
    );

    // -----------------------------------------------------------------
    // Tool 7: capture_display — Capture the display framebuffer as base64
    // -----------------------------------------------------------------
    // Copy the display info into a local struct captured by value so the
    // lambda remains valid beyond this function's scope.
    DisplayInfo capturedDisplay;
    if (displayInfo != nullptr) {
        capturedDisplay = *displayInfo;
    }

    registry.addTool(
        "capture_display",
        "Capture the current display buffer as a base64-encoded image",
        [capturedDisplay](JsonObject& params, JsonObject& result) {
            (void)params; // No input parameters required

            // --- Guard: no display registered ---
            if (capturedDisplay.framebuffer == nullptr || capturedDisplay.framebufferSize == 0) {
                result["error"]   = "No display registered";
                result["isError"] = true;
                return;
            }

            // --- Encode framebuffer to base64 ---
            #ifndef NATIVE_BUILD
                // Hardware: use the ESP32 base64 library with chunked encoding
                // to avoid a single massive heap allocation. Process 48-byte
                // chunks (48 divides cleanly by 3 for base64 padding alignment).
                std::string b64;
                size_t offset = 0;
                size_t len = capturedDisplay.framebufferSize;
                const uint8_t* data = capturedDisplay.framebuffer;
                while (offset < len) {
                    size_t chunk = (len - offset > 48) ? 48 : (len - offset);
                    String b64Chunk = base64::encode(&data[offset], chunk);
                    b64 += b64Chunk.c_str();
                    offset += chunk;
                }
            #else
                // Native build: use the inline base64 encoder
                std::string b64 = base64Encode(capturedDisplay.framebuffer,
                                               capturedDisplay.framebufferSize);
            #endif

            result["format"] = "xbm";
            result["width"]  = capturedDisplay.width;
            result["height"] = capturedDisplay.height;
            result["data"]   = b64;
        }
    );

    ESP_LOGI(TAG, "Registered %zu built-in tools", registry.size());
}
