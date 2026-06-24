/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: template-app/src/myApp.hpp
 * Description: Main application module for the sample skeleton. Implements
 *              ITickable, IConfigurable, IWebContributor, IMCPContributor,
 *              and IConfigGate to demonstrate all primary extension points.
 *
 * Exported Classes:
 * - MyApp: Main application logic container.
 *   - tick():              Periodic update hook called by the Application hub.
 *   - applyConfig():       Loads persisted settings from ConfigManager.
 *   - registerRoutes():    Registers custom REST endpoints with the web server.
 *   - getTabName():        Retrieves the UI tab name for this module.
 *   - registerMCPTools():  Exposes tools to the MCP server.
 *   - getGateName():       Returns the human-readable gate name.
 */

#pragma once

#include "AppFoundation.h"

class PsychicHttpServer;
class MCPServer;
class MCPToolRegistry;
class ConfigManager;

/**
 * @brief Main application class implementing specific business logic.
 *
 * Participates in the Application tick loop, participates in the ConfigManager,
 * contributes Web UI tabs and routes, contributes MCP tools, and exposes a config gate.
 */
class MyApp : public ITickable,
              public IConfigurable,
              public IWebContributor,
              public IMCPContributor,
              public IConfigGate {
public:
    MyApp() = default;
    ~MyApp() override = default;

    // ITickable
    void tick(uint32_t currentMillis) override;

    // IConfigurable
    const char* getConfigNamespace() const override;
    int getSchemaVersion() const override;
    void applyConfig(const JsonObject& config) override;
    void serializeConfig(JsonObject& config) const override;
    void resetConfig() override;

    // IWebContributor
    void registerRoutes(PsychicHttpServer& server) override;
    const char* getTabName() const override;
    int getTabOrder() const override;

    // IMCPContributor
    void registerMCPTools(MCPToolRegistry& registry) override;
    void registerMcpTools(MCPServer& mcp);

    // IConfigGate
    const char* getGateName() const override;
    bool isSatisfied() const override;

    /// @brief Wire the ConfigManager pointer for save operations.
    void setConfigManager(ConfigManager* cm) { _configManager = cm; }

private:
    // Configuration state (persisted via IConfigurable)
    int _refreshIntervalMs = 5000;     ///< Example configurable parameter
    bool _featureEnabled = true;       ///< Example boolean toggle

    // Config gate state
    bool _gateOpen = false;

    // Tick timing
    uint32_t _lastActionMs = 0;

    // Dependencies
    ConfigManager* _configManager = nullptr; ///< Pointer to ConfigManager for save operations
};
