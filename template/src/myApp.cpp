/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: template/src/myApp.cpp
 * Description: Implementation of the MyApp class for the template skeleton.
 *
 * Exported Functions/Classes:
 * - MyApp: Main application logic container.
 *   - tick(): Periodic update hook.
 *   - registerRoutes(): Web route registration.
 *   - getTabName(): UI tab name provider.
 */

#include "myApp.hpp"
#include "MCPToolRegistry.hpp"

#ifndef NATIVE_BUILD
#include <PsychicHttp.h>
#endif

/**
 * @brief Periodic update hook called by the Application hub on every loop iteration.
 *
 * Implementations must be non-blocking and complete within 10ms.
 */
void MyApp::tick(uint32_t currentMillis) {
    // Example: perform an action every _refreshIntervalMs
    if (currentMillis - _lastActionMs >= (uint32_t)_refreshIntervalMs) {
        _lastActionMs = currentMillis;
        // Your periodic logic here
    }

    // Demonstrate gate pattern: open the gate after the first tick
    if (!_gateOpen) {
        _gateOpen = true;
    }
}

// -----------------------------------------------------------------------------
// IConfigurable Implementation
// -----------------------------------------------------------------------------

const char* MyApp::getConfigNamespace() const {
    return "myapp";  // Config stored at /mod/myapp.json
}

int MyApp::getSchemaVersion() const {
    return 1;
}

void MyApp::applyConfig(const JsonObject& config) {
    _refreshIntervalMs = config["refreshMs"] | 5000;
    _featureEnabled = config["enabled"] | true;
}

void MyApp::serializeConfig(JsonObject& config) const {
    config["refreshMs"] = _refreshIntervalMs;
    config["enabled"] = _featureEnabled;
}

void MyApp::resetConfig() {
    _refreshIntervalMs = 5000;
    _featureEnabled = true;
}

// -----------------------------------------------------------------------------
// IWebContributor Implementation
// -----------------------------------------------------------------------------

#ifndef NATIVE_BUILD
void MyApp::registerRoutes(PsychicHttpServer& server) {
    server.on("/api/myapp/status", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        doc["status"] = "ok";
        doc["refreshMs"] = _refreshIntervalMs;
        doc["enabled"] = _featureEnabled;
        doc["gateOpen"] = _gateOpen;

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

        return response->send(200, "application/json", "{\"status\":\"saved\"}");
    });
}
#else
void MyApp::registerRoutes(PsychicHttpServer& server) {
    // No-op for native build
    (void)server;
}
#endif

const char* MyApp::getTabName() const {
    return "MyApp";
}

int MyApp::getTabOrder() const {
    return 100; // Default tab order
}

// -----------------------------------------------------------------------------
// IMCPContributor Implementation
// -----------------------------------------------------------------------------

void MyApp::registerMCPTools(MCPToolRegistry& registry) {
    registry.addTool(
        "myapp_status",
        "Get status and configuration of MyApp",
        [this](JsonObject& params, JsonObject& result) {
            (void)params;
            result["refreshMs"] = _refreshIntervalMs;
            result["enabled"] = _featureEnabled;
            result["gateOpen"] = _gateOpen;
        },
        false // mutatesState = false
    );
}

void MyApp::registerMcpTools(MCPServer& mcp) {
    (void)mcp;
}

// -----------------------------------------------------------------------------
// IConfigGate Implementation
// -----------------------------------------------------------------------------

const char* MyApp::getGateName() const {
    return gateName();
}

bool MyApp::isSatisfied() const {
    return isOpen();
}

const char* MyApp::gateName() const {
    return "MyApp Ready";
}

bool MyApp::isOpen() const {
    return _gateOpen;
}
