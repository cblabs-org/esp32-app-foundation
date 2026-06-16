# Configuration Persistence

The persistence layer guarantees that runtime configuration is safely stored to flash memory and can survive power loss or OTA updates.

## Triggering Saves: `saveModuleConfig()`

Modules should not attempt to write to flash directly. When a module's configuration changes internally, it must notify the manager. 

The `ConfigManager` exposes `saveModuleConfig(IConfigurable* module)`. When called:
1. The manager queries the module for its latest state via `serializeConfig(JsonObject& section)`.
2. The manager updates the in-memory representation.
3. The manager triggers a flash write operation.

*Note: Flash writes are slow and wear out the memory. Modules should batch configuration changes where possible and only call `saveModuleConfig()` when a user explicitly alters a setting.*

## Bulk Operations: Export & Import

The manager provides `exportAllConfig(JsonObject& doc)` and `importAllConfig(JsonObject& doc)` for bulk operations.

- **Export**: Used by the Web Portal to generate a downloadable backup of the device state. It iterates through all registered modules, calling their `serializeConfig()` methods to build a complete JSON tree.
- **Import**: Used to restore a device. It parses an incoming JSON tree, routes the appropriate objects to each module's `applyConfig()` method, and immediately flushes the new state to disk.

## The Three-Tier ConfigManifest

Flash corruption is a serious risk on ESP32 devices, especially if power is lost during a write operation. The framework mitigates this via a three-tier `ConfigManifest` backup system.

When the `ConfigManager` writes to disk, it does not simply overwrite `manifest.json`. It follows a safe rotation pattern:

1. **Active**: The current `manifest.json`. This is the file loaded during normal boot.
2. **Previous**: Before writing a new Active file, the current `manifest.json` is renamed to `manifest.prev.json`.
3. **Golden**: If the device successfully boots, connects to Wi-Fi, and reaches the `tick()` loop without panicking, the Active manifest is copied to `manifest.golden.json`. This represents a known-good configuration.

### Recovery Sequence

If the `ConfigManager` fails to load the Active manifest (e.g., due to JSON parse errors or a missing file):
1. It attempts to load `manifest.prev.json`.
2. If the previous file is also corrupted, it falls back to `manifest.golden.json`.
3. If all three fail, it alerts the user (via the display or serial) and proceeds with default configurations.

This robust recovery sequence ensures the device rarely requires a factory reset due to file system corruption.
