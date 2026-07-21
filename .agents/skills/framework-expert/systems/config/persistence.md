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
