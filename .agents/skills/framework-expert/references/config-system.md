# Configuration System Reference

## Config File Ownership & Paths

The framework centralizes configuration management in `ConfigManager`. Individual modules never open files or read/write flash directly.
*   **Path Pattern:** Stored JSON files live at `/mod/<namespace>.json` where `<namespace>` is the string returned by `getConfigNamespace()`.
*   **Metadata Versioning:** `ConfigManager` inserts and manages a `_v` integer key at the root of the JSON file to track schema versions.

## The IConfigurable Interface Contract

To participate in configuration persistence, implement these five methods:

### 1. `getConfigNamespace`
*   **Signature:** `virtual const char* getConfigNamespace() const = 0;`
*   **Purpose:** Returns the unique namespace for the config file name (e.g., `"wifi"` maps to `/mod/wifi.json`). Must be a compile-time string literal.

### 2. `getSchemaVersion`
*   **Signature:** `virtual int getSchemaVersion() const = 0;`
*   **Purpose:** Returns the current config version of the running code.

### 3. `applyConfig`
*   **Signature:** `virtual void applyConfig(const JsonObject& config) = 0;`
*   **Purpose:** Invoked at startup. Parsed config options are passed in. Read values into class members.

### 4. `serializeConfig`
*   **Signature:** `virtual void serializeConfig(JsonObject& config) const = 0;`
*   **Purpose:** Invoked when saving. Serialize current member values into the mutable `JsonObject`. Do not insert the `_v` metadata key.

### 5. `resetConfig`
*   **Signature:** `virtual void resetConfig() = 0;`
*   **Purpose:** Invoked on factory reset or when loading a corrupted config file. Resets settings to defaults.

## Schema Versioning & Migrations

If the stored config file's version (`_v`) is lower than the value returned by the module's `getSchemaVersion()`, `ConfigManager` executes the migration callback:

*   **Signature:** `virtual bool migrateConfig(JsonObject& section, int fromVersion);`
*   **Implementation Rule:** Modify `section` in-place. Return `true` if successful. Return `false` to abort the migration and force a factory reset (calls `resetConfig()`).

```cpp
bool MyApp::migrateConfig(JsonObject& section, int fromVersion) {
    if (fromVersion == 1) {
        // Example: rename key
        if (section.containsKey("oldKey")) {
            section["newKey"] = section["oldKey"];
            section.remove("oldKey");
        }
        return true;
    }
    return false;
}
```

## ArduinoJson v7 Best Practices
Use the default operator `|` when reading fields to provide safe fallback values if a key is missing:
```cpp
// Correct: Safe fallback
_interval = config["interval"] | 5000;

// Incorrect: Causes crashes if key is missing or null
_interval = config["interval"].as<int>();
```

## Runtime Config Updates
To save updated configuration values to flash during runtime, invoke `saveModuleConfig` on the `ConfigManager` instance:
```cpp
void MyApp::updateInterval(int newInterval) {
    _interval = newInterval;
    app.getConfigManager().saveModuleConfig(this); // Serializes and writes to /mod/myapp.json
}
```

## Compile-Time Secrets: FrameworkConfig
*   **`FrameworkConfig`:** A stack-allocated struct passed to `app.begin(config)`. It holds **only** compile-time secrets (e.g. GitHub private tokens, OTA public keys) that must **never** be saved to LittleFS.
*   **`IConfigurable`:** Dedicated runtime configuration. All other parameters (WiFi, offsets, custom module parameters) must use the `IConfigurable` interface.
