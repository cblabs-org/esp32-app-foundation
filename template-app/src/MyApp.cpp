/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: template-app/src/MyApp.cpp
 * Description: Implementation of the MyApp class for the template skeleton.
 *              Demonstrates all 5 primary extension points: ITickable,
 *              IConfigurable, IWebContributor, IMCPContributor, and IPreconditionContributor.
 *
 * Exported Classes:
 * - MyApp: Template application module demonstrating 5 primary framework interfaces.
 *
 *   Own API:
 *   - setConfigManager():    Wire the ConfigManager pointer for save operations.
 *
 *   ITickable:
 *   - tick():                Periodic update hook called by the Application hub.
 *
 *   IConfigurable ("myapp"):
 *   - getConfigNamespace():  Returns the config namespace identifier.
 *   - getSchemaVersion():    Returns the current schema version.
 *   - applyConfig():         Loads persisted settings from ConfigManager.
 *   - serializeConfig():     Serializes current configuration to JSON.
 *   - resetConfig():         Restores factory defaults.
 *
 *   IWebContributor:
 *   - registerRoutes():      Registers custom REST endpoints with the web server.
 *   - registerTabs():        Register navigation tabs via TabRegistry.
 *   - getApiPrefix():        API path prefix for body-size middleware.
 *   - getMaxPostBody():      Max POST/PUT body size (default: 4096).
 *
 *   IMCPContributor:
 *   - registerMCPTools():    Exposes tools to the MCP server.
 *
 *   IPreconditionContributor:
 *   - registerPreconditions(): Registers "MyApp Ready" operational precondition.
 */

#include "MyApp.hpp"
#include "MCPToolRegistry.hpp"
#include "ConfigManager.hpp"

#include <PsychicHttp.h>

// =============================================================================
// ITickable Implementation
// =============================================================================

/**
 * @brief Periodic update hook called by the Application hub on every loop iteration.
 *
 * Performs interval-based periodic logic and sets readiness on the first
 * tick to demonstrate the IPreconditionContributor pattern.
 *
 * @param currentMillis Current system time in milliseconds.
 */
void MyApp::tick(uint32_t currentMillis) {
    // Example: perform an action every _refreshIntervalMs
    if (currentMillis - _lastActionMs >= static_cast<uint32_t>(_refreshIntervalMs)) {
        _lastActionMs = currentMillis;
        // Your periodic logic here
    }

    // Demonstrate precondition pattern: mark ready after the first tick
    if (!_ready) {
        _ready = true;
    }
}

// =============================================================================
// IConfigurable Implementation
// =============================================================================

/**
 * @brief Returns the config namespace identifier.
 *
 * Config is stored at /mod/myapp.json on the LittleFS filesystem.
 *
 * @return Namespace string used to locate the persisted config file.
 */
const char* MyApp::getConfigNamespace() const {
    return "myapp";  // TODO: Change "myapp" to your app's config namespace
}

/**
 * @brief Returns the current schema version for config migration.
 * @return Integer schema version.
 */
int MyApp::getSchemaVersion() const {
    return 1;
}

/**
 * @brief Loads persisted settings from ConfigManager.
 *
 * Reads key-value pairs from the persisted JSON config and applies them
 * to the module's internal state. Uses ArduinoJson's | operator for
 * safe defaults when keys are absent.
 *
 * @param config JSON object containing the persisted key-value pairs.
 */
void MyApp::applyConfig(const JsonObject& config) {
    _refreshIntervalMs = config["refreshMs"] | 5000;
    _featureEnabled = config["enabled"] | true;
}

/**
 * @brief Serializes current configuration to a JSON object for persistence.
 * @param config JSON object to populate with current settings.
 */
void MyApp::serializeConfig(JsonObject& config) const {
    config["refreshMs"] = _refreshIntervalMs;
    config["enabled"] = _featureEnabled;
}

/**
 * @brief Restores all configuration values to factory defaults.
 */
void MyApp::resetConfig() {
    _refreshIntervalMs = 5000;
    _featureEnabled = true;
}

// =============================================================================
// IWebContributor Implementation (including getApiPrefix & getMaxPostBody)
// =============================================================================

/**
 * @brief Registers custom REST endpoints with the web server.
 *
 * Provides a GET endpoint for reading current status and a POST endpoint
 * for updating configuration at runtime with automatic persistence.
 *
 * @param server Reference to the PsychicHttpServer instance.
 */
void MyApp::registerRoutes(PsychicHttpServer& server) {
    // TODO: Change "/api/myapp/..." to your API paths
    server.on("/api/myapp/status", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        doc["status"] = "ok";
        doc["refreshMs"] = _refreshIntervalMs;
        doc["enabled"] = _featureEnabled;
        doc["ready"] = _ready;

        String output;
        serializeJson(doc, output);
        return response->send(200, "application/json", output.c_str());
    });

    server.on("/api/myapp/config", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) {
            return response->send(400, "application/json", "{\"error\":\"body_required\"}");
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            return response->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        }

        if (doc["refreshMs"].is<int>()) {
            _refreshIntervalMs = doc["refreshMs"].as<int>();
        }
        if (doc["enabled"].is<bool>()) {
            _featureEnabled = doc["enabled"].as<bool>();
        }

        // Persist the updated config to LittleFS
        if (_configManager) {
            _configManager->saveModuleConfig(this);
        }

        return response->send(200, "application/json", "{\"status\":\"saved\"}");
    });
}

/**
 * @brief Register the MyApp tab as a top-level navigation tab.
 * @param registry  The TabRegistry to register tabs with.
 */
void MyApp::registerTabs(TabRegistry& registry) {
    registry.addTab("MyApp", nullptr, 100);
    // TODO: Change "MyApp" and 100 to your tab name and desired order
}

// =============================================================================
// IMCPContributor Implementation
// =============================================================================

/**
 * @brief Exposes tools to the MCP server for AI agent interaction.
 *
 * Registers a read-only status tool that returns the module's current
 * configuration and gate state.
 *
 * @param registry Reference to the MCPToolRegistry for tool registration.
 */
void MyApp::registerMCPTools(MCPToolRegistry& registry) {
    // TODO: Rename "myapp_status" to your tool name
    registry.addTool(
        "myapp_status",
        "Get status and configuration of MyApp",
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* self = static_cast<MyApp*>(ctx);
            (void)params;
            result["refreshMs"] = self->_refreshIntervalMs;
            result["enabled"] = self->_featureEnabled;
            result["ready"] = self->_ready;
        },
        this,  // context: MyApp*
        false // mutatesState = false
    );
}

// =============================================================================
// IPreconditionContributor Implementation
// =============================================================================

void MyApp::registerPreconditions(PreconditionRegistry& reg) {
    reg.add("MyApp Ready", PreconditionRole::Operational,
        [this]() -> PreconditionStatus {
            return _ready
                ? PreconditionStatus{true, Cause::Met}
                : PreconditionStatus{false, Cause::Connecting};
        });
}
