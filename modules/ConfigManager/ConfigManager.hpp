/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/ConfigManager/ConfigManager.hpp
 * Description: Centralised configuration gateway that owns all config file I/O.
 *              Manages a dynamic locked registry of IConfigurable modules,
 *              handles schema versioning via self-describing _v fields,
 *              orchestrates migration on boot, and provides bulk export/import.
 *
 *              Config files are stored at /mod/<namespace>.json (one per module).
 *              Modules provide serialise/deserialise callbacks; they never touch
 *              LittleFS directly.
 *
 *              Backup output includes a _meta envelope (backup_version,
 *              firmware_version, device_id, device_type, datetime, timezone,
 *              module_count) prepended as the first key in the JSON document.
 *
 * Exported Functions/Classes:
 * - ConfigManager: Centralised configuration gateway
 *
 *   Module Registry:
 *   - registerModule():      Register an IConfigurable module in the gateway
 *   - beginAll():            Load and apply config for all registered modules
 *   - saveModuleConfig():    Persist a single module's config to its namespace file
 *   - exportAllConfig():     Export all module configs into a master JSON document
 *                            (includes _meta as first key)
 *   - importAllConfig():     Import configs from a master JSON document
 *                            (_meta and other unknown keys are skipped silently)
 *   - setMetaTimezone():     Register the timezone string for the _meta envelope
 *
 *   IWebContributor (tab: "Backup", parent: "System", order: 40):
 *   - registerRoutes():      Register REST API routes
 *   - getTabName():          Return "Backup" for portal tab
 *   - getParentTab():        Return "System" for navigation hierarchy
 *   - getTabOrder():         Return 40 for pill ordering
 *
 *   IMCPContributor:
 *   - registerMCPTools():    Register config_get_schema_version and
 *                            config_factory_reset MCP tools
 */

#pragma once

#include <vector>

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>    // WiFi.macAddress() for _meta envelope
#include <time.h>    // time(), gmtime_r(), strftime() for _meta datetime
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#include <ArduinoJson.h>
#include "../../include/interfaces/IConfigurable.hpp"
#include "../../include/interfaces/IWebContributor.hpp"
#include "../../include/interfaces/IMCPContributor.hpp"
#include "../../lib/ConfigManifest/ConfigManifest.hpp"

/**
 * @brief Centralised configuration gateway that owns all config file I/O.
 *
 * Manages a dynamic registry of IConfigurable module pointers. Modules are
 * registered before beginAll() is called; the registry is locked afterwards
 * (matching the Application two-tier model). On boot, beginAll() loads each
 * module's config from /mod/<namespace>.json, checks schema versions, runs
 * migrations if needed, and calls applyConfig(). Individual modules call
 * saveModuleConfig() when they need to persist changes.
 */
class ConfigManager : public IWebContributor, public IMCPContributor {
public:
    ConfigManager() = default;

    // -------------------------------------------------------------------------
    // Module Registry
    // -------------------------------------------------------------------------

    /**
     * @brief Register an IConfigurable module with the gateway.
     *
     * Modules must be registered before beginAll() is called. Post-lock
     * registration attempts are rejected and logged as errors.
     *
     * @param module Non-null pointer to the IConfigurable module.
     * @return true if registration succeeded; false if locked or nullptr.
     */
    bool registerModule(IConfigurable* module);

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
     * @param module Pointer to the IConfigurable module to save.
     * @return true if successfully saved; false on error.
     */
    bool saveModuleConfig(IConfigurable* module);

    /**
     * @brief Export all registered module configs into a master JSON document.
     *
     * Writes a _meta envelope as the first key, then nests each module's config
     * under its namespace key. Each section includes a _v schema version field.
     * Used for full-device backup. The _meta key is silently skipped on import.
     *
     * @param outDoc Reference to the output JsonDocument (caller-owned).
     */
    void exportAllConfig(JsonDocument& outDoc);

    /**
     * @brief Import module configs from a master JSON document.
     *
     * Extracts each registered module's namespace section, calls applyConfig(),
     * and persists via saveModuleConfig(). Unknown top-level keys (e.g. "_meta")
     * that do not match any registered module namespace are silently skipped;
     * a DEBUG-level log is emitted for each unmatched key.
     *
     * @param inDoc Reference to the input JsonDocument containing config data.
     * @return true if all modules imported successfully.
     */
    bool importAllConfig(JsonDocument& inDoc);

    /**
     * @brief Export a single module's configuration by namespace.
     *
     * Iterates the internal module list and calls serializeConfig() on the
     * matching module. Callers do not need access to the module list.
     *
     * @param ns        Namespace string to match (e.g. "wifi", "pm_channels").
     * @param outSection Mutable JSON object to serialize into.
     * @return true if namespace was found, false otherwise.
     */
    bool exportNamespaceConfig(const char* ns, JsonObject& outSection) const;

    /**
     * @brief Register the current timezone string for inclusion in backup _meta.
     *
     * Intended to be called from TimeManager::applyConfig() after the timezone
     * is resolved, so that the backup envelope reflects the configured TZ.
     * If not called, the _meta timezone field defaults to "UTC".
     *
     * @param tz Null-terminated POSIX timezone string (e.g. "Europe/London").
     */
    void setMetaTimezone(const char* tz);

    /**
     * @brief Access the Three-Tier config manifest.
     *
     * Provides direct access to the ConfigManifest for snapshot operations
     * by OTAUpdateManager (Active→Previous before OTA, Active→Golden after
     * firmware blessing).
     *
     * @return Mutable reference to the internal ConfigManifest.
     */
    ConfigManifest& getManifest() { return _manifest; }


    // -------------------------------------------------------------------------
    // IWebContributor Interface
    // -------------------------------------------------------------------------

    /**
     * @brief Register the ConfigManager's own API endpoints.
     * @param server Reference to the PsychicHttpServer instance.
     */
    void registerRoutes(PsychicHttpServer& server) override;

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

    // -------------------------------------------------------------------------
    // IMCPContributor Interface
    // -------------------------------------------------------------------------

    /**
     * @brief Register ConfigManager-specific MCP tools.
     *
     * Registers:
     *   - config_get_schema_version (read-only): Returns the system-level config
     *     schema version used in backup/restore envelopes.
     *   - config_factory_reset (mutating): Wipes all persisted module config
     *     files from LittleFS and schedules a device reboot.
     *
     * @param registry The MCPToolRegistry to register tools with.
     */
    void registerMCPTools(MCPToolRegistry& registry) override;

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
     * @param module Pointer to the IConfigurable module.
     */
    void loadModuleConfig(IConfigurable* module);

    /**
     * @brief Populate the _meta envelope for a backup document.
     *
     * Writes backup_version, firmware_version, device_id, device_type,
     * datetime, timezone, and module_count into the provided JsonObject.
     * Hardware values (MAC, chip model, clock) are only available outside
     * native builds; safe defaults are used in the native test environment.
     *
     * @param meta Mutable reference to the target JsonObject.
     */
    void buildBackupMeta(JsonObject& meta);

    /**
     * @brief Format the config file path for a given namespace.
     *
     * Writes "/mod/<ns>.json" (hardware) or "mod_<ns>.json" (native builds)
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

    std::vector<IConfigurable*> _modules;  ///< Dynamic registry of IConfigurable modules (locked after beginAll())
    bool _locked = false;                  ///< True after beginAll() — post-lock registration is rejected
    char _metaTimezone[64] = {};           ///< Timezone string for backup _meta envelope (default: "UTC")
    ConfigManifest _manifest;              ///< Three-Tier config lifecycle manifest (Active/Previous/Golden)

};
