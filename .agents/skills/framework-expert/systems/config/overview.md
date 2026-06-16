# Configuration Subsystem Overview

The Config Subsystem is responsible for managing, persisting, and distributing runtime configuration state across the framework and all registered modules.

## The ConfigManager Gateway Pattern

The `ConfigManager` acts as the single gateway for all configuration read/write operations. Individual modules do not interact with the LittleFS file system directly for their configuration. Instead, they register with the `ConfigManager` and provide it with a data structure. 

This pattern provides several critical benefits:
1. **Centralized Serialization**: Modules define simple `struct`s. The framework handles the complex serialization/deserialization to JSON via ArduinoJson.
2. **Atomicity**: The manager ensures that configuration reads and writes are safe and atomic, protecting against flash corruption during unexpected power loss.
3. **Unified API**: Exposing configuration to the Web Portal or MCP Server requires only querying the central manager, rather than polling individual modules.

## File Ownership

The `ConfigManager` has exclusive ownership of the `/mod/` directory on the LittleFS partition. No other module should attempt to write to this directory. 

Each module's configuration is stored in a separate file at `/mod/<namespace>.json` (e.g. `/mod/wifi.json`, `/mod/display.json`).

## Registry Lifecycle

Configuration management follows a strict lifecycle tied to the `Application` boot sequence:

1. **Instantiation**: The `ConfigManager` is created.
2. **Registration**: During `Application::begin()`, all modules call `ConfigManager::registerConfigurable()`, passing a pointer to themselves (as an [`IConfigurable`](../../interfaces/configurable.md)). The manager builds a registry of all known configurable modules.
3. **Loading**: The `ConfigManager` reads each module's configuration file from disk, parses the JSON, and passes the relevant JSON object to the module's `applyConfig()` method.
4. **Boot Continuation**: Modules proceed with their initialization using the newly loaded configuration.
5. **Runtime Updates**: When a module's configuration is modified (via the Web Portal, MCP, or internally), the module calls `saveModuleConfig()`. The manager updates the central registry and flushes the entire state back to disk.

This lifecycle ensures that no module attempts to act on empty or default configuration state during boot.
