/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: template-app/src/MyApp.hpp
 * Description: Main application module for the template skeleton. Implements
 *              ITickable, IConfigurable, IWebContributor, IMCPContributor,
 *              and IPreconditionContributor to demonstrate all five primary extension points.
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
 *   - registerTabs():        Register navigation tabs via ITabRegistry.
 *   - getApiPrefix():        API path prefix for body-size middleware [NEW]
 *   - getMaxPostBody():      Max POST/PUT body size (default: 4096) [NEW]
 *
 *   IMCPContributor:
 *   - registerMCPTools():    Exposes tools to the MCP server.
 *
 *   IPreconditionContributor:
 *   - registerPreconditions(): Contributes "MyApp Ready" operational precondition.
 */

#pragma once

#include "AppFoundation.h"

class PsychicHttpServer;
class MCPToolRegistry;
class ConfigManager;

// TODO: Rename this class to match your application
/**
 * @brief Main application class implementing specific business logic.
 *
 * Participates in the Application tick loop, participates in the ConfigManager,
 * contributes Web UI tabs and routes, contributes MCP tools, and exposes preconditions.
 */
class MyApp : public ITickable,
              public IConfigurable,
              public IWebContributor,
              public IMCPContributor,
              public IPreconditionContributor {
public:
    /** @brief Default constructor. */
    MyApp() = default;

    /** @brief Virtual destructor. */
    ~MyApp() override = default;

    // --- ITickable ---

    /** @brief Periodic update hook called by the Application hub on every loop iteration.
     *  @param currentMillis Current system time in milliseconds.
     */
    void tick(uint32_t currentMillis) override;

    // --- IConfigurable ---

    /** @brief Returns the config namespace identifier.
     *  @return Namespace string used to locate the persisted config file.
     */
    const char* getConfigNamespace() const override;

    /** @brief Returns the current schema version for config migration.
     *  @return Integer schema version.
     */
    int getSchemaVersion() const override;

    /** @brief Loads persisted settings from ConfigManager.
     *  @param config JSON object containing the persisted key-value pairs.
     */
    void applyConfig(const JsonObject& config) override;

    /** @brief Serializes current configuration to a JSON object for persistence.
     *  @param config JSON object to populate with current settings.
     */
    void serializeConfig(JsonObject& config) const override;

    /** @brief Restores all configuration values to factory defaults. */
    void resetConfig() override;

    // --- IWebContributor ---

    /** @brief Registers custom REST endpoints with the web server.
     *  @param server Reference to the PsychicHttpServer instance.
     */
    void registerRoutes(PsychicHttpServer& server) override;

    /** @brief Register the MyApp tab as a top-level navigation tab.
     *  @param registry  The ITabRegistry to register tabs with. */
    void registerTabs(ITabRegistry& registry) override;

    // Optional: nest under System instead of top-level:
    // void registerTabs(ITabRegistry& registry) override {
    //     registry.addTab("MyApp", "System", 100);
    // }

    /** @brief Returns the API path prefix for body-size middleware matching.
     *  @return API prefix string.
     */
    const char* getApiPrefix() const override { return "/api/myapp"; }

    /** @brief Returns the maximum POST/PUT body size for this module's routes.
     *  @return Maximum body size in bytes (ADR-15 default: 4096).
     */
    size_t getMaxPostBody() const override { return 4096; }

    // --- IMCPContributor ---

    /** @brief Register m c p tools. */
    void registerMCPTools(MCPToolRegistry& registry) override;

    // --- IPreconditionContributor ---
    /** @brief Register preconditions. */
    void registerPreconditions(IPreconditionRegistry& reg) override;

    // --- Own API ---

    /** @brief Wire the ConfigManager pointer for save operations.
     *  @param cm Pointer to the framework ConfigManager instance.
     */
    void setConfigManager(ConfigManager* cm) { _configManager = cm; }

private:
    // Configuration state (persisted via IConfigurable)
    int _refreshIntervalMs = 5000;     ///< Example configurable parameter
    bool _featureEnabled = true;       ///< Example boolean toggle

    // Precondition readiness state
    bool _ready = false;

    // Tick timing
    uint32_t _lastActionMs = 0;

    // Dependencies
    ConfigManager* _configManager = nullptr; ///< Pointer to ConfigManager for save operations
};
