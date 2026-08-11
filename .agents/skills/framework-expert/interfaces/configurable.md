# IConfigurable

Contract for modules that participate in the centralised configuration gateway managed by `ConfigManager`.

## Overview

Modules provide serialization/deserialization callbacks and a namespace. `ConfigManager` owns all LittleFS file I/O, persisting config at `/mod/<namespace>.json`.

```cpp
class IConfigurable {
public:
    virtual ~IConfigurable() = default;
    virtual const char* getConfigNamespace() const = 0;
    virtual int getSchemaVersion() const = 0;
    virtual void applyConfig(const JsonObject& section) = 0;
    virtual void serializeConfig(JsonObject& section) const = 0;
    virtual bool migrateConfig(JsonObject& section, int fromVersion) { return true; }
    virtual void resetConfig() = 0;
};
```

## Methods

- **`getConfigNamespace()`**: Returns a unique string identifier (e.g. `"wifi"`). Determines the filename. Must be a compile-time string literal.
- **`getSchemaVersion()`**: Returns the current schema version integer. Bumping this triggers a migration.
- **`applyConfig()`**: Reads values from the JSON object to internal state.
- **`serializeConfig()`**: Writes internal state to the JSON object.
- **`migrateConfig()`**: (Optional) In-place migration of an older schema to the current schema.
- **`resetConfig()`**: Restores factory defaults. `ConfigManager` will call `saveModuleConfig()` afterwards.

## Implementation Example

```cpp
void MyModule::applyConfig(const JsonObject& section) {
    // Use ArduinoJson v7 | operator for defaults
    _isEnabled = section["enabled"] | true;
    strlcpy(_apiKey, section["api_key"] | "", sizeof(_apiKey));
}

void MyModule::serializeConfig(JsonObject& section) const {
    section["enabled"] = _isEnabled;
    section["api_key"] = _apiKey;
}

void MyModule::resetConfig() {
    _isEnabled = true;
    _apiKey[0] = '\0';
}
```

## Schema Migrations

When `ConfigManager` detects that the stored `_v` field is less than `getSchemaVersion()`, it calls `migrateConfig()`.

```cpp
bool MyModule::migrateConfig(JsonObject& section, int fromVersion) {
    if (fromVersion < 2) {
        // v1 used "active", v2 uses "enabled"
        if (section.containsKey("active")) {
            section["enabled"] = section["active"];
            section.remove("active");
        }
    }
    return true; // Migration succeeded
}
```

> [!NOTE]
> When a module's state changes during runtime (e.g., a user toggles a setting via the web portal), the module MUST call `configManager.saveModuleConfig(this)` to persist the change. `ConfigManager` handles the disk write asynchronously.
