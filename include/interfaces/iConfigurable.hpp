/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/interfaces/iConfigurable.hpp
 * Description: Pure interface for modules that participate in the centralised
 *              configuration gateway managed by ConfigManager. Modules provide
 *              serialisation/deserialisation callbacks and a namespace identifier;
 *              ConfigManager owns all file I/O at /mod/<namespace>.json.
 *
 *              Each module declares a schema version via getSchemaVersion().
 *              On boot, ConfigManager compares the stored _v field against the
 *              declared version and calls migrateConfig() when a schema upgrade
 *              is required.
 *
 * Exported Classes:
 * - iConfigurable: Abstract base for config-aware modules
 *   - getConfigNamespace(): Return the module's unique config namespace identifier
 *   - getSchemaVersion():   Return the current schema version (integer)
 *   - applyConfig():        Apply configuration from a parsed JSON section
 *   - serializeConfig():    Serialize current configuration into a JSON section
 *   - migrateConfig():      Migrate config from an older schema version (optional)
 *   - resetConfig():        Restore factory defaults
 */

#pragma once

#include <ArduinoJson.h>

/**
 * @brief Contract for modules that participate in the ConfigManager gateway.
 *
 * Modules implementing this interface provide serialisation/deserialisation
 * callbacks. ConfigManager owns all file I/O — modules NEVER touch LittleFS
 * directly for configuration persistence. Config files are stored at
 * /mod/<namespace>.json with a self-describing _v schema version field.
 */
class iConfigurable {
public:
    virtual ~iConfigurable() = default;

    /**
     * @brief Return the module's unique configuration namespace identifier.
     *
     * The namespace determines the config file path: /mod/<namespace>.json.
     * Must be a compile-time string literal (no heap allocation).
     *
     * @return Null-terminated namespace string (e.g. "wifi", "display", "ota").
     */
    virtual const char* getConfigNamespace() const = 0;

    /**
     * @brief Return the current schema version for this module's config format.
     *
     * Schema version is an integer that is bumped when firmware changes require
     * a config migration. It is NOT a save counter.
     *
     * @return Integer schema version (starts at 1).
     */
    virtual int getSchemaVersion() const = 0;

    /**
     * @brief Apply configuration values from a parsed JSON object.
     *
     * Called by ConfigManager during beginAll() after loading and optionally
     * migrating the module's config file. The module reads its fields from
     * the provided section and applies them to internal state.
     *
     * @param section Read-only reference to the JSON object containing config keys.
     */
    virtual void applyConfig(const JsonObject& section) = 0;

    /**
     * @brief Serialize current module configuration into a JSON object.
     *
     * Called by ConfigManager during saveModuleConfig(). The module writes
     * its current settings into the provided section. ConfigManager appends
     * the _v field separately.
     *
     * @param section Mutable reference to the target JSON object.
     */
    virtual void serializeConfig(JsonObject& section) const = 0;

    /**
     * @brief Migrate configuration from an older schema version.
     *
     * Called by ConfigManager when the stored _v is less than the module's
     * declared getSchemaVersion(). The module may modify the JSON section
     * in-place (e.g. rename keys, add defaults for new fields).
     *
     * Default implementation returns true (no migration needed).
     *
     * @param section   Mutable reference to the JSON config section.
     * @param fromVersion The stored schema version being migrated from.
     * @return true if migration succeeded; false to fall back to defaults.
     */
    virtual bool migrateConfig(JsonObject& section, int fromVersion) {
        (void)section;
        (void)fromVersion;
        return true;
    }

    /**
     * @brief Reset module configuration to factory defaults.
     *
     * The module clears its internal state to safe defaults. ConfigManager
     * will call saveModuleConfig() afterwards to persist the reset state.
     */
    virtual void resetConfig() = 0;
};
