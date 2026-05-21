/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/interfaces/iConfigurable.hpp
 * Description: Pure interface for modules that load and persist their
 *              configuration via the ConfigManager LittleFS JSON store.
 *              The module path convention is: /mod/<moduleName>.json
 *
 * Exported Classes:
 * - iConfigurable: Abstract base for config-aware modules
 *   - loadConfig(): Load config from LittleFS (called during begin())
 *   - saveConfig(): Persist current config to LittleFS
 *   - resetConfig(): Restore factory defaults and persist
 */

#pragma once

/**
 * @brief Contract for modules that manage persistent configuration via LittleFS.
 *
 * Modules implementing this interface store their JSON config under
 * `/mod/<moduleName>.json` by convention (see spec §6 LittleFS Directory Convention).
 */
class iConfigurable {
public:
    virtual ~iConfigurable() = default;

    /**
     * @brief Load module configuration from LittleFS.
     * @return true if config was successfully loaded; false if defaults were applied.
     */
    virtual bool loadConfig() = 0;

    /**
     * @brief Persist current module configuration to LittleFS.
     * @return true if the write succeeded.
     */
    virtual bool saveConfig() = 0;

    /**
     * @brief Reset module configuration to factory defaults and persist.
     */
    virtual void resetConfig() = 0;
};
