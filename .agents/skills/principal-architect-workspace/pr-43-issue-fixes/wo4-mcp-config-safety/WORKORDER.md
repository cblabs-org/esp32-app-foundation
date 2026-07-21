---
name: pr-43-wo4
description: MCP config safety (deep-merge) and namespace scoping — cap raise moved to WO5
complexity: standard
---

# WO4 — MCP Config Safety and Namespace Scoping

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix two related defects in the MCP config tools: (A) partial writes silently destroy config in modules that reset-then-load, and (B) both config tools are unbounded on the response path. Closes GitHub Issue #21.

**Note:** The body cap raise (formerly Part C) has been moved to WO5 — the unified provider-declared body limit mechanism. MCPServer will declare its own limit via `getMaxPostBody()` in WO5.

## Strict Boundary Box

- **DO NOT** modify any module's `applyConfig()` or `serializeConfig()` implementation.
- **DO NOT** implement JSON Patch (RFC 6902) — that is deferred to a follow-up issue.
- **DO NOT** change body size caps — that is WO5's responsibility.
- **DO NOT** modify any files outside the checklist below.

## Critical Design Constraints

1. **Deep-merge is key-level only.** ArduinoJson replaces arrays wholesale — a short `channels` array replaces a long one. This is a known limitation documented in the code. Element-level merge requires JSON Patch (deferred).
2. **`applyConfig()` must always receive a complete section.** The merge happens BEFORE calling `applyConfig()`, so module implementations don't need to change.
3. **The echo removal is a breaking change for MCP clients** that rely on the `set_config` response containing the full config. The new response shape is `{"status":"updated","updated_namespaces":{"wifi":true}}`.
4. **Do NOT expose `_modules` via `getModules()`.** Use a new `exportNamespaceConfig()` method on `ConfigManager` so that `MCPBuiltinTools` does not reach into internal state.
5. **Thread safety note:** `importAllConfig()` runs on the HTTP task. The new deep-merge widens the window where `serializeConfig()` (read) and `applyConfig()` (write) overlap with concurrent task access. This is a pre-existing concern — add a code comment noting `_configMutex` as a future improvement. Not blocking this PR because config writes are infrequent MCP-driven events.

## Checklist

### Part A — Deep-merge partial writes in `importAllConfig()`

- [ ] **`modules/ConfigManager/ConfigManager.cpp:374-375`** — Replace the direct `mod->applyConfig(section)` call with a merge-over-current-state pattern:
  ```cpp
  // Deep-merge incoming partial section over current state so that
  // applyConfig() always receives a COMPLETE section — preventing
  // modules that reset-then-load from losing omitted keys.
  // NOTE: Array values are replaced wholesale (ArduinoJson semantics)
  // — element-level merge requires JSON Patch (deferred to follow-up).
  // TODO: A _configMutex protecting serializeConfig/applyConfig pairs
  // would close the pre-existing thread-safety window between the HTTP
  // task (MCP handler) and the main loop. Scoped as a follow-up.
  JsonDocument cur;
  JsonObject curSec = cur.to<JsonObject>();
  mod->serializeConfig(curSec);        // snapshot current state

  // Merge: incoming keys overwrite current; current keys not in
  // incoming are preserved.
  for (JsonPair kv : section) {
      curSec[kv.key()] = kv.value();
  }

  mod->applyConfig(curSec);            // module sees complete section
  ```

- [ ] **`include/interfaces/IConfigurable.hpp:66-74`** — Amend the `applyConfig` docstring:
  ```
   * Called by ConfigManager during beginAll() and importAllConfig(). The
   * section always contains a COMPLETE configuration — during import,
   * ConfigManager merges partial input over the module's current state
   * before calling this method. Implementations may safely assume all
   * expected keys are present.
  ```

### Part B — Namespace-scoped tools + drop the echo

- [ ] **`modules/ConfigManager/ConfigManager.hpp`** — Add public method declaration:
  ```cpp
  /**
   * @brief Export a single module's configuration by namespace.
   *
   * Iterates the internal module list and calls serializeConfig() on the
   * matching module. Callers do not need access to the module list.
   *
   * @param ns        Namespace string to match (e.g. "wifi", "pm_channels").
   * @param outSection Mutable JSON object to serialize into.
   * @return true if namespace was found, false otherwise.
   */
  bool exportNamespaceConfig(const char* ns, JsonObject& outSection) const;
  ```

- [ ] **`modules/ConfigManager/ConfigManager.cpp`** — Add implementation:
  ```cpp
  bool ConfigManager::exportNamespaceConfig(const char* ns, JsonObject& outSection) const {
      for (const auto* mod : _modules) {
          if (strcmp(mod->getConfigNamespace(), ns) == 0) {
              mod->serializeConfig(outSection);
              return true;
          }
      }
      return false;
  }
  ```

- [ ] **`modules/MCPServer/MCPBuiltinTools.cpp` — `get_config` (lines 185-207):**
  - Add optional `namespace` string parameter to the input schema.
  - When `namespace` is provided: call `configManager->exportNamespaceConfig(ns, result)`. If it returns false, set `result["error"]` and `result["isError"] = true`.
  - When `namespace` is omitted: current all-namespace `exportAllConfig()` behaviour preserved.
  - Update the tool description to document the optional param.

- [ ] **`modules/MCPServer/MCPBuiltinTools.cpp` — `set_config` (lines 222-261):**
  - **Lines 252-257:** Remove the whole-config echo. Replace with:
    ```cpp
    result["status"] = success ? "updated" : "partial_failure";
    // Report which namespaces were updated, not the entire config
    JsonObject updated = result["updated_namespaces"].to<JsonObject>();
    for (JsonPair kv : configObj) {
        updated[kv.key()] = true;
    }
    ```
  - Update the tool description to note the response no longer echoes the full config.

## Verification

- [ ] PlatformIO build passes (ConfigManager.cpp, ConfigManager.hpp, MCPBuiltinTools.cpp all compile).
- [ ] Code review: confirm the deep-merge pattern correctly preserves existing keys while overwriting incoming ones.
- [ ] Code review: confirm `get_config` with namespace param calls `exportNamespaceConfig()` — not `getModules()`.
- [ ] Code review: confirm `set_config` response no longer contains the full system config.
