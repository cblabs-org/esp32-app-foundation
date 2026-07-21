# ConfigManager Subsystem

The `ConfigManager` acts as the single gateway for all configuration read/write operations, persistence to LittleFS, element-level JSON patching, and bulk import/export operations.

## Architecture & Lifecycle

Individual modules implement [`IConfigurable`](../../interfaces/configurable.md) and register with `ConfigManager` during boot.

1. **Registration**: Modules call `ConfigManager::registerModule()` before `beginAll()`. Post-lock registrations are rejected.
2. **Boot Loading**: `ConfigManager::beginAll()` loads `/mod/<namespace>.json` for each module, performs schema migration if necessary, and calls `applyConfig()`.
3. **Runtime Updates**: Modules call `saveModuleConfig()` when their configuration changes.

## Triggering Saves: `saveModuleConfig()`

Modules should not attempt to write to flash directly. When a module's configuration changes internally, it notifies the manager.

The `ConfigManager` exposes `saveModuleConfig(IConfigurable* module)`:
1. The manager queries the module for its latest state via `serializeConfig(JsonObject& section)`.
2. The manager updates the in-memory representation.
3. The manager triggers a flash write operation.

*Note: Flash writes are slow and wear out memory. Modules should batch configuration changes where possible and only call `saveModuleConfig()` when a setting changes.*

## Bulk Operations: Export & Import

The manager provides `exportAllConfig(JsonObject& doc)` and `importAllConfig(JsonObject& doc)` for bulk operations.

- **Export**: Used by the Web Portal and backup tools to generate a full backup. It iterates registered modules, calling `serializeConfig()` to build a master JSON tree with a `_meta` envelope.
- **Import**: Restores a device from backup. It routes each namespace object to the respective module's `applyConfig()` method and flushes the state to disk.

## Element-Level Writes (JSON Patch)

For fine-grained updates without transmitting the entire namespace configuration, `ConfigManager` provides element-level patching using an RFC 6902 JSON Patch subset.

### `patchNamespaceConfig()` Method

```cpp
bool patchNamespaceConfig(const char* ns, JsonArrayConst ops,
                          JsonPatch::PatchResult& result,
                          JsonDocument* storedValues = nullptr);
```

- **Atomic Semantics**: All-or-nothing execution. Either **all** operations in the `ops` array succeed and are persisted to flash, or **none** are applied (rolling back to the pre-patch state snapshot).
- **Supported Operations**: `replace`, `add`, `test`, and `remove` (on JSON object keys only). `move` and `copy` are unsupported.

### The `patch_config` MCP Tool

The built-in `patch_config` MCP tool exposes element-level patching to AI agents and external controllers.

**Wire Format (Request):**
```json
{
  "name": "patch_config",
  "arguments": {
    "namespace": "pm_channels",
    "ops": [
      { "op": "test", "path": "/channels/0/enabled", "value": true },
      { "op": "replace", "path": "/channels/0/label", "value": "Solar Inverter" }
    ]
  }
}
```

**Response Shape (Success):**
```json
{
  "status": "patched",
  "namespace": "pm_channels",
  "ops_applied": 2,
  "results": {
    "/channels/0/label": "Solar Inverter"
  }
}
```

**Response Shape (Failure):**
```json
{
  "error": "Path not found",
  "failed_op": 1,
  "path": "/channels/99/label",
  "applied": false,
  "isError": true
}
```

**Error Codes:**
- `INVALID_POINTER`: Malformed JSON Pointer string (RFC 6901 violation).
- `PATH_NOT_FOUND`: Target pointer path does not exist (for replace, test, remove).
- `INDEX_OUT_OF_RANGE`: Array index out of range.
- `ARRAY_REMOVE_BLOCKED`: `remove` operation on array indices is blocked in v1 (use `replace` with sentinel instead).
- `TEST_FAILED`: `test` op failed due to value mismatch (aborts the entire batch).
- `UNKNOWN_OP`: Unrecognised operation string.
- `TOO_MANY_OPS`: Operation count exceeds `MAX_PATCH_OPS` (32).
- `MUTEX_TIMEOUT`: Config mutex acquisition timed out.

### Bandwidth & Payload Comparison

Patching individual fields avoids sending large multi-kilobyte documents over bandwidth-constrained links (such as BLE, low-power Wi-Fi, or cellular IoT).

Example: Renaming a single channel label sends ~70 bytes for `patch_config` versus ~8 KB for `set_config`.

**Sizing Table (Issue #28 §10 Benchmark):**

| Scenario | `set_config` (Full Namespace) | `patch_config` (RFC 6902) | Bandwidth Reduction |
| :--- | :--- | :--- | :--- |
| Rename 1 channel | ~8.0 KB | ~70 B | ~99.1% |
| Edit 8 channels | ~8.0 KB | ~1.1 KB | ~86.2% |
| Relabel all 64 channels | ~8.0 KB | ~4.5 KB | ~43.8% |

### Engine Limitations

- **Array Index Removal**: `remove` on array indices is blocked in v1 to avoid implicit index shifting.
- **Maximum Operations**: `MAX_PATCH_OPS = 32` ops per patch request.
- **Pointer Depth**: JSON Pointer path depth `MAX_DEPTH ≤ 6` (e.g. `/a/b/c/d/e/f`).
- **Token Length**: Individual pointer token length `MAX_TOKEN_LEN ≤ 32` characters.

## Config Mutex & Thread Safety

The `ConfigManager` employs a FreeRTOS mutex (`_configMutex`) to protect configuration read-modify-write sequences against concurrency issues.

- **Lock Window**: `_configMutex` protects the snapshot → applyConfig → serialize window.
- **Timed Acquisition**: Uses a 50ms acquisition timeout (`pdMS_TO_TICKS(50)`). If the mutex cannot be acquired within 50ms, the operation aborts and returns a `MUTEX_TIMEOUT` error.
- **Flash I/O Outside Lock**: In-memory serialization occurs under lock, but disk persistence (LittleFS flash write) executes **outside** the lock.
- **Thread Safety Rationale**: Prevents race conditions between asynchronous HTTP server tasks (PsychicHttp / MCP worker threads) and the main FreeRTOS loop, while ensuring slow flash erase/write cycles do not stall other tasks waiting on configuration reads.

## The Three-Tier ConfigManifest

Flash corruption is mitigated via a three-tier `ConfigManifest` backup system:
1. **Active** (`manifest.json`): Loaded during normal boot.
2. **Previous** (`manifest.prev.json`): Rotated before writing new Active file.
3. **Golden** (`manifest.golden.json`): Known-good configuration saved after stable boot.
