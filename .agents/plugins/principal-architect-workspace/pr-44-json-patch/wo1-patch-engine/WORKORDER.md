---
name: pr-44-wo1
description: Handoff instructions for Work Order 1 — JSON Pointer resolver, patch engine, ConfigManager patchNamespaceConfig(), mutex, saveModuleConfig() refactor
complexity: architect
---

# WO1 — JSON Patch Engine + ConfigManager Core

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Implement the RFC 6902 JSON Patch subset engine and integrate it into ConfigManager. This adds element-level config addressing, a config mutex for thread safety, and refactors `saveModuleConfig()` to split serialize from persist.

## Strict Boundary Box

- **DO NOT** modify any files outside the ones listed in the checklist
- **DO NOT** modify MCP tool registration (`MCPBuiltinTools.cpp`) — that is WO2
- **DO NOT** write tests — that is WO3
- **DO NOT** modify documentation/skill files — that is WO4
- **DO NOT** modify template-app or sample-app
- Surface any out-of-scope discoveries to me (the PA) in your completion report rather than filing issues

## Tooling Constraints

- Use the **PlatformIO MCP server** tools for compilation verification (build for `native` environment only — no flash)
- Follow the house style: ESP32 App Foundation header block, Doxygen method docs, `constexpr` TAG, no `String` class in new code, fixed `char` buffers

## Key Design Decisions (from Issue #28 thread — do not deviate)

| Decision | Value |
|---|---|
| Ops supported | `replace`, `add`, `test`, `remove` (object keys only) |
| `remove` on array index | Explicit error, not silently ignored |
| MAX_PATCH_OPS | 32 |
| Pointer depth limit | 6 segments |
| Token length limit | 32 chars |
| Pointer validation | Must start with `/`, no empty string, no trailing `/` |
| Unknown ops | Rejected with error |
| Mutex type | `SemaphoreHandle_t` (FreeRTOS), timed acquisition |
| Mutex timeout | 50ms on HTTP path (return error on timeout), 10ms on main loop (skip tick) |
| Flash I/O under lock | **NO** — serialize under lock, persist outside lock |
| Module validation | `applyConfig()` → `saveModuleConfig()` — never persist patched JSON directly |
| Response shape | Return patched paths with stored (post-clamp) values |

## Reference Code Locations

- `modules/ConfigManager/ConfigManager.hpp` — class definition, member variables
- `modules/ConfigManager/ConfigManager.cpp` — `saveModuleConfig()` at line 209, `importAllConfig()` at line 330, `exportNamespaceConfig()` at line 405
- `include/interfaces/IConfigurable.hpp` — `applyConfig()`, `serializeConfig()` signatures

## Line-Item Instructions

### 1. Create `modules/ConfigManager/JsonPatch.hpp`

New file. Static utility class:

```cpp
#pragma once
#include <ArduinoJson.h>

/**
 * @brief RFC 6902 JSON Patch subset — element-level config addressing.
 *
 * Provides JSON Pointer resolution (RFC 6901) and patch op dispatch
 * for the ConfigManager gateway. All operations use fixed char buffers —
 * no String class, no heap allocation beyond ArduinoJson's own pool.
 *
 * Supported ops: replace, add, test, remove (object keys only).
 * Unsupported: move, copy.
 */
class JsonPatch {
public:
    /// Maximum depth of JSON Pointer segments (e.g. /a/b/c/d/e/f = 6)
    static constexpr int MAX_DEPTH = 6;
    /// Maximum length of a single pointer token
    static constexpr int MAX_TOKEN_LEN = 32;
    /// Maximum ops per patch request
    static constexpr int MAX_PATCH_OPS = 32;

    /// Error codes for patch operations
    enum class Error : uint8_t {
        OK = 0,
        INVALID_POINTER,       // malformed JSON Pointer
        PATH_NOT_FOUND,        // target path does not exist (for replace/test/remove)
        INDEX_OUT_OF_RANGE,    // array index >= size
        ARRAY_REMOVE_BLOCKED,  // remove on array index not supported in v1
        TEST_FAILED,           // test op value mismatch
        UNKNOWN_OP,            // unrecognised op string
        TOO_MANY_OPS,          // ops array exceeds MAX_PATCH_OPS
        MUTEX_TIMEOUT,         // could not acquire config mutex
    };

    /// Result of a patch operation
    struct PatchResult {
        Error  error    = Error::OK;
        int    opIndex  = -1;       // which op failed (-1 = N/A)
        char   path[128] = {};      // path of failed op (for error reporting)
        bool   applied  = false;    // true only if ALL ops succeeded and were persisted
        int    opsApplied = 0;      // count of ops successfully applied
    };

    /**
     * @brief Apply a single RFC 6902 op to a mutable JSON document.
     * @return Error::OK on success, specific error code on failure.
     */
    static Error applyOne(JsonObject& doc, JsonObjectConst op, char* failedPath, size_t pathCap);

    /**
     * @brief Validate a JSON Pointer string.
     * @return Error::OK if valid, Error::INVALID_POINTER if not.
     */
    static Error validatePointer(const char* ptr);

private:
    /**
     * @brief Resolve a JSON Pointer to its parent variant and final token.
     *
     * E.g. "/channels/5/label" → parent = doc["channels"][5], lastToken = "label"
     */
    static bool resolveParent(JsonVariant root, const char* ptr,
                              JsonVariant& parent, char* lastToken, size_t lastTokenCap);

    /// RFC 6901 §3 token unescaping: ~1 → /, ~0 → ~ (order matters)
    static void unescapeToken(char* token);
};
```

### 2. Create `modules/ConfigManager/JsonPatch.cpp`

Implementation. Key points:

- `validatePointer()`: reject empty, not starting with `/`, trailing `/`, depth > 6, token > 32
- `unescapeToken()`: `~1` → `/` first, then `~0` → `~` (RFC 6901 §3 order)
- `resolveParent()`: split pointer on `/`, walk the document. For array elements, detect all-digits tokens and index into the array. Walk to the second-to-last token, put the final token in `lastToken`.
- `applyOne()`: read the `"op"` field, dispatch:
  - `"replace"`: resolve parent + last token. Target MUST exist (error if not). Replace the value.
  - `"add"`: resolve parent + last token. For objects: set value (create or overwrite). For arrays: if last token is `-`, append; if numeric, insert at index (reject if > size).
  - `"test"`: resolve to value, compare with `"value"` field. Return `TEST_FAILED` if mismatch.
  - `"remove"`: resolve parent + last token. If parent is array → return `ARRAY_REMOVE_BLOCKED`. If parent is object → remove key. Key must exist (error if not).
  - Anything else → `UNKNOWN_OP`

Use `ESP_LOGW` for rejected ops (aids debugging on device).

### 3. Modify `modules/ConfigManager/ConfigManager.hpp`

Add these includes and members:

```cpp
#include "JsonPatch.hpp"
#include <freertos/semphr.h>
```

Add public method:
```cpp
    /**
     * @brief Apply RFC 6902 JSON Patch ops to a single namespace.
     *
     * Atomic: either ALL ops succeed and are persisted, or NONE are.
     * The config mutex is held during the snapshot→apply→serialize
     * window; flash I/O happens outside the lock.
     *
     * @param ns   Namespace to patch (e.g. "pm_channels").
     * @param ops  Array of RFC 6902 op objects.
     * @param result Output: error details and stored values.
     * @param storedValues Optional: if non-null, receives the post-clamp
     *                     values for each patched path.
     * @return true if all ops applied successfully.
     */
    bool patchNamespaceConfig(const char* ns, JsonArrayConst ops,
                              JsonPatch::PatchResult& result,
                              JsonDocument* storedValues = nullptr);
```

Add private members:
```cpp
    SemaphoreHandle_t _configMutex = nullptr;  ///< Protects serialize/applyConfig pairs

    /**
     * @brief Serialize a module's current config into a JsonDocument.
     *
     * Extracted from saveModuleConfig() so the serialize step can run
     * under mutex while the persist step runs outside it.
     */
    bool serializeModuleToDoc(IConfigurable* module, JsonDocument& outDoc);
```

### 4. Modify `modules/ConfigManager/ConfigManager.cpp`

**4a. Constructor — create mutex:**

The class currently uses `= default`. Add an explicit constructor body:
```cpp
ConfigManager::ConfigManager()
    : _configMutex(xSemaphoreCreateMutex())
{
}
```
Update the header to match: `ConfigManager();` (remove `= default`).

**4b. Extract `serializeModuleToDoc()`:**

Extract from `saveModuleConfig()` lines 221-225:
```cpp
bool ConfigManager::serializeModuleToDoc(IConfigurable* module, JsonDocument& outDoc) {
    JsonObject root = outDoc.to<JsonObject>();
    module->serializeConfig(root);
    outDoc["_v"] = module->getSchemaVersion();
    return true;
}
```

**4c. Refactor `saveModuleConfig()` to use `serializeModuleToDoc()`:**

```cpp
bool ConfigManager::saveModuleConfig(IConfigurable* module) {
    if (module == nullptr) {
        ESP_LOGE(TAG, "saveModuleConfig() — null module pointer");
        return false;
    }
    const char* ns = module->getConfigNamespace();
    char path[48];
    getConfigPath(ns, path, sizeof(path));
    ESP_LOGI(TAG, "Saving config for '%s' to %s", ns, path);

    // Serialize under mutex (if available), persist outside
    JsonDocument doc;
    if (_configMutex != nullptr) {
        if (xSemaphoreTake(_configMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
            ESP_LOGW(TAG, "saveModuleConfig() — mutex timeout for '%s'", ns);
            return false;
        }
    }
    serializeModuleToDoc(module, doc);
    if (_configMutex != nullptr) {
        xSemaphoreGive(_configMutex);
    }

    // Persist outside lock
    if (!LittleFS.exists("/mod")) { LittleFS.mkdir("/mod"); }
    File file = LittleFS.open(path, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open %s for writing", path);
        return false;
    }
    if (serializeJson(doc, file) == 0) {
        ESP_LOGE(TAG, "Failed to write config to %s", path);
        file.close();
        return false;
    }
    file.close();
    ESP_LOGI(TAG, "Config saved for '%s'", ns);
    return true;
}
```

**4d. Add mutex to `importAllConfig()`:**

Wrap the serialize→merge→apply sequence (lines 384-394) with timed mutex:
```cpp
if (_configMutex != nullptr) {
    if (xSemaphoreTake(_configMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        ESP_LOGW(TAG, "importAllConfig() — mutex timeout for '%s'", ns);
        allOk = false;
        continue;
    }
}
// ... existing serialize → merge → applyConfig ...
if (_configMutex != nullptr) {
    xSemaphoreGive(_configMutex);
}
// saveModuleConfig (persists outside lock via its own refactored path)
```

**4e. Implement `patchNamespaceConfig()`:**

```cpp
bool ConfigManager::patchNamespaceConfig(const char* ns, JsonArrayConst ops,
                                         JsonPatch::PatchResult& result,
                                         JsonDocument* storedValues) {
    // Find module by namespace
    IConfigurable* mod = nullptr;
    for (auto* m : _modules) {
        if (strcmp(m->getConfigNamespace(), ns) == 0) { mod = m; break; }
    }
    if (!mod) {
        result.error = JsonPatch::Error::PATH_NOT_FOUND;
        result.applied = false;
        snprintf(result.path, sizeof(result.path), "(namespace '%s' not found)", ns);
        return false;
    }

    if (static_cast<int>(ops.size()) > JsonPatch::MAX_PATCH_OPS) {
        result.error = JsonPatch::Error::TOO_MANY_OPS;
        result.applied = false;
        return false;
    }

    // Acquire mutex
    if (_configMutex != nullptr) {
        if (xSemaphoreTake(_configMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
            result.error = JsonPatch::Error::MUTEX_TIMEOUT;
            result.applied = false;
            return false;
        }
    }

    // 1. Snapshot current state
    JsonDocument cur;
    JsonObject sec = cur.to<JsonObject>();
    mod->serializeConfig(sec);

    // 2. Apply every op to the scratch copy
    int i = 0;
    for (JsonObjectConst op : ops) {
        JsonPatch::Error err = JsonPatch::applyOne(sec, op, result.path, sizeof(result.path));
        if (err != JsonPatch::Error::OK) {
            result.error = err;
            result.opIndex = i;
            result.applied = false;
            if (_configMutex != nullptr) xSemaphoreGive(_configMutex);
            return false;
        }
        ++i;
    }

    // 3. Commit — module ingests, clamps, normalises
    mod->applyConfig(sec);

    // 4. Serialize post-apply state (for response + persist) — still under lock
    JsonDocument postDoc;
    serializeModuleToDoc(mod, postDoc);

    if (_configMutex != nullptr) xSemaphoreGive(_configMutex);

    // 5. Extract stored values for patched paths (outside lock)
    if (storedValues != nullptr) {
        JsonObject postRoot = postDoc.as<JsonObject>();
        JsonObject stored = storedValues->to<JsonObject>();
        for (JsonObjectConst op : ops) {
            const char* path = op["path"] | "";
            // Walk the post-apply document to find the stored value at this path
            JsonVariant val = postRoot;
            // Simple pointer walk for value extraction
            char ptrCopy[128];
            strncpy(ptrCopy, path, sizeof(ptrCopy) - 1);
            ptrCopy[sizeof(ptrCopy) - 1] = '\0';
            char* token = strtok(ptrCopy + 1, "/");  // skip leading '/'
            while (token != nullptr && !val.isNull()) {
                // Check if token is numeric (array index)
                char* end;
                long idx = strtol(token, &end, 10);
                if (*end == '\0' && val.is<JsonArrayConst>()) {
                    val = val[static_cast<size_t>(idx)];
                } else {
                    val = val[token];
                }
                token = strtok(nullptr, "/");
            }
            if (!val.isNull()) {
                stored[path] = val;
            }
        }
    }

    // 6. Persist outside lock — reuse the serialised document
    const char* modNs = mod->getConfigNamespace();
    char filePath[48];
    getConfigPath(modNs, filePath, sizeof(filePath));
    if (!LittleFS.exists("/mod")) { LittleFS.mkdir("/mod"); }
    File file = LittleFS.open(filePath, "w");
    if (!file) {
        ESP_LOGE(TAG, "patchNamespaceConfig — failed to open %s for writing", filePath);
        result.applied = false;
        return false;
    }
    if (serializeJson(postDoc, file) == 0) {
        ESP_LOGE(TAG, "patchNamespaceConfig — failed to write %s", filePath);
        file.close();
        result.applied = false;
        return false;
    }
    file.close();

    result.applied = true;
    result.opsApplied = i;
    ESP_LOGI(TAG, "Patched '%s': %d ops applied", ns, i);
    return true;
}
```

### 5. Update stubs for native build

Check `test/stubs/platform/` — if `freertos/semphr.h` stub is missing, create a minimal one with `xSemaphoreCreateMutex()`, `xSemaphoreTake()`, `xSemaphoreGive()` returning `pdTRUE`. The existing `freertos/FreeRTOS.h` stub likely exists — extend it or create a `semphr.h` alongside it.

## Verification Requirement

1. Build with PlatformIO MCP for **`native`** environment — must compile cleanly
2. Build with PlatformIO MCP for **`esp32dev`** environment — must compile cleanly
3. Run existing native tests — must all pass (no regressions)
4. Code review: confirm mutex is never held across flash I/O
5. Code review: confirm `applyConfig()` → `serializeModuleToDoc()` sequence (never persist raw patched JSON)

Once verified, **STOP and send a message back to me** with results. Do not wrap up or distill.
