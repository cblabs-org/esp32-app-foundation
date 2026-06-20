# ADR: ConfigManager IMCPContributor Design

## Decision 1: Config Schema Version Semantics

**Context:** The WO specified `config_get_schema_version` should return `{"version": int}`. ConfigManager is not itself an IConfigurable (it has no `getSchemaVersion()`), but it does maintain a system-level `backup_version` constant used in the `_meta` envelope of backup/restore operations.

**Decision:** Return the system-level `backup_version` (currently `1`), NOT individual per-module `_v` values. The per-module schema versions are already accessible via the existing `get_config` built-in tool (which includes `_v` in each namespace section).

**Rationale:** This makes `config_get_schema_version` complementary to `get_config` rather than duplicating it. The backup_version represents the config *system's* own schema — the format of the backup envelope itself.

## Decision 2: Factory Reset via LittleFS File Deletion (Not NVS)

**Context:** The WO said "wipe the NVS partition", but ConfigManager does NOT use NVS. All config persistence is via LittleFS files at `/mod/<namespace>.json`. NVS is used by other subsystems (WiFi credentials via ESP-IDF internals, DeviceCrypto key material) but not by ConfigManager directly.

**Decision:** Factory reset deletes all registered module config files from `/mod/` plus the config manifest file (`/config_manifest.json`), then schedules a device reboot. On restart, `loadModuleConfig()` finds no files and calls `resetConfig()` on each module, naturally restoring factory defaults.

**Rationale:** This is the correct and safe approach — it resets exactly what ConfigManager owns. A full `nvs_flash_erase()` would be far more destructive (wiping WiFi calibration data, crypto keys, etc.) and crosses module boundaries. The per-module file deletion approach is precise and follows the existing config lifecycle model.

## Decision 3: Delayed Reboot Pattern

**Context:** After wiping config files, the device needs to reboot so modules re-initialize with factory defaults.

**Decision:** Used `xTimerCreate("cfg_factory_rst", pdMS_TO_TICKS(500), ...)` with `ESP.restart()` — identical to the pattern used in the config restore POST endpoint and `wifi_forget` MCP tool.

**Rationale:** The 500ms delay ensures the JSON-RPC response is flushed to the client before the device restarts. Using a FreeRTOS software timer avoids blocking the HTTP handler thread.

## Decision 4: No Input Schema Needed

**Context:** Both tools take no parameters.

**Decision:** Used the simpler `addTool()` overload without a `JsonDocument inputSchema` parameter. The registry creates a default empty schema: `{ "type": "object", "properties": {} }`.

## Decision 5: `config_get_schema_version` Uses `nullptr` Context

**Context:** The schema version tool returns a compile-time constant and needs no access to ConfigManager state.

**Decision:** Passed `nullptr` as the context pointer and named the parameter `/*context*/` to indicate it's intentionally unused. This avoids the (tiny) overhead of a static_cast that would never be used.
