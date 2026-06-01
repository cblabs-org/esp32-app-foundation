/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/configManager/configManager.hpp
 * Description: Centralised configuration gateway that owns all config file I/O.
 *              Manages a fixed-size registry of iConfigurable modules, handles
 *              schema versioning via self-describing _v fields, orchestrates
 *              migration on boot, and provides bulk export/import for backup.
 *
 *              Config files are stored at /mod/<namespace>.json (one per module).
 *              Modules provide serialise/deserialise callbacks; they never touch
 *              LittleFS directly.
 *
 * Exported Functions/Classes:
 * - ConfigManager: Centralised configuration gateway
 *   - registerModule():      Register an iConfigurable module in the gateway
 *   - beginAll():            Load and apply config for all registered modules
 *   - saveModuleConfig():    Persist a single module's config to its namespace file
 *   - exportAllConfig():     Export all module configs into a master JSON document
 *   - importAllConfig():     Import configs from a master JSON document

 *   - registerRoutes():      Register REST API routes (IWebContributor)
 *   - getTabName():          Return the UI tab name ("Backup")
 *   - getParentTab():        Return parent tab ("System")
 *   - getTabOrder():         Return tab order (40)
 */

#pragma once

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <LittleFS.h>
#else
#include <string>
#include <stdint.h>
#endif

#include <ArduinoJson.h>
#include "../../include/interfaces/iConfigurable.hpp"
#include "../../include/interfaces/IWebContributor.hpp"

/**
 * @brief Centralised configuration gateway that owns all config file I/O.
 *
 * Manages a fixed-size array of iConfigurable module pointers. On boot,
 * beginAll() loads each module's config from /mod/<namespace>.json, checks
 * schema versions, runs migrations if needed, and calls applyConfig().
 * Individual modules call saveModuleConfig() when they need to persist changes.
 */
class ConfigManager : public IWebContributor {
public:
    ConfigManager() = default;

    // -------------------------------------------------------------------------
    // Module Registry
    // -------------------------------------------------------------------------

    static constexpr uint8_t MAX_CONFIGURABLES = 8; ///< Maximum number of registered iConfigurable modules

    /**
     * @brief Register an iConfigurable module with the gateway.
     *
     * Modules must be registered before beginAll() is called. Bounds-checked
     * against MAX_CONFIGURABLES.
     *
     * @param module Non-null pointer to the iConfigurable module.
     * @return true if registration succeeded; false if full or nullptr.
     */
    bool registerModule(iConfigurable* module);

    /**
     * @brief Load and apply configuration for all registered modules.
     *
     * Iterates the module registry and calls loadModuleConfig() on each.
     * This handles file reads, version checks, migrations, and applyConfig().
     */
    void beginAll();

    /**
     * @brief Persist a single module's current configuration to its namespace file.
     *
     * Creates a JsonDocument, calls module->serializeConfig(), sets the _v
     * field, and writes directly to /mod/<namespace>.json via file stream.
     *
     * @param module Pointer to the iConfigurable module to save.
     * @return true if successfully saved; false on error.
     */
    bool saveModuleConfig(iConfigurable* module);

    /**
     * @brief Export all registered module configs into a master JSON document.
     *
     * Each module's config is nested under its namespace key. Includes _v
     * for each section. Used for full-device backup.
     *
     * @param outDoc Reference to the output JsonDocument (caller-owned).
     */
    void exportAllConfig(JsonDocument& outDoc);

    /**
     * @brief Import module configs from a master JSON document.
     *
     * Extracts each registered module's namespace section, calls applyConfig(),
     * and persists via saveModuleConfig().
     *
     * @param inDoc Reference to the input JsonDocument containing config data.
     * @return true if all modules imported successfully.
     */
    bool importAllConfig(JsonDocument& inDoc);


    // -------------------------------------------------------------------------
    // IWebContributor Interface
    // -------------------------------------------------------------------------

    /**
     * @brief Register all REST API route handlers with the web server.
     * @param server Reference to the AsyncWebServer instance.
     */
    void registerRoutes(AsyncWebServer& server) override;

    /**
     * @brief Get the UI tab name for this contributor.
     * @return "Backup"
     */
    const char* getTabName() const override { return "Backup"; }

    /**
     * @brief Get the parent tab for navigation hierarchy.
     * @return "System" — ConfigManager appears as a sub-section of the System tab.
     */
    const char* getParentTab() const override { return "System"; }

    /**
     * @brief Get the tab display order within the System sub-navigation.
     * @return 40 — positioned after Keys (20) and MCP (30) in the pill bar.
     */
    int getTabOrder() const override { return 40; }

private:
    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Load a single module's config from its namespace file.
     *
     * Opens /mod/<namespace>.json, reads the stored _v version, compares
     * against the module's getSchemaVersion(), calls migrateConfig() if
     * needed, then calls applyConfig(). Rewrites the file if migration occurred.
     *
     * @param module Pointer to the iConfigurable module.
     */
    void loadModuleConfig(iConfigurable* module);

    /**
     * @brief Format the config file path for a given namespace.
     *
     * Writes "/mod/<ns>.json" (hardware) or "mod_<ns>.json" (NATIVE_BUILD)
     * into the provided buffer.
     *
     * @param ns   Namespace string (e.g. "wifi").
     * @param buf  Output buffer.
     * @param size Buffer size in bytes.
     */
    static void getConfigPath(const char* ns, char* buf, size_t size);

    // -------------------------------------------------------------------------
    // Member variables
    // -------------------------------------------------------------------------

    iConfigurable* _modules[MAX_CONFIGURABLES] = {}; ///< Fixed-size registry of iConfigurable modules
    uint8_t _moduleCount = 0;                        ///< Number of registered modules

};
