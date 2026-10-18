---
name: pr-44-wo2
description: Handoff instructions for Work Order 2 — patch_config MCP tool registration
complexity: standard
---

# WO2 — `patch_config` MCP Tool Registration

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Register a new `patch_config` MCP tool in `MCPBuiltinTools.cpp` that exposes the `ConfigManager::patchNamespaceConfig()` method (added in WO1) to MCP clients.

## Strict Boundary Box

- **ONLY** modify `modules/MCPServer/MCPBuiltinTools.cpp`
- **DO NOT** modify ConfigManager, JsonPatch, tests, docs, or any other files
- Surface any out-of-scope discoveries to me (the PA) in your completion report

## Tooling Constraints

- Use the **PlatformIO MCP server** tools for compilation verification (build `native` and `esp32dev`)
- Follow existing tool registration patterns in `MCPBuiltinTools.cpp` (see Tools 1-3 for examples)

## Reference

- Existing tool registration pattern: `MCPBuiltinTools.cpp` lines 182-290 (get_config, set_config)
- `ConfigManager::patchNamespaceConfig()` signature in `modules/ConfigManager/ConfigManager.hpp`
- `JsonPatch::PatchResult` struct in `modules/ConfigManager/JsonPatch.hpp`
- `BuiltinToolsContext` struct at top of `MCPBuiltinTools.cpp` contains `configManager` pointer

## Line-Item Instructions

### 1. Add `patch_config` tool registration after `set_config` (Tool 3)

Add as Tool 4, following the exact same pattern. The tool:

**Input schema:**
```json
{
  "type": "object",
  "properties": {
    "namespace": {
      "type": "string",
      "description": "Config namespace to patch (e.g. \"wifi\", \"pm_channels\")"
    },
    "ops": {
      "type": "array",
      "description": "Array of RFC 6902 JSON Patch ops. Each op: {\"op\": \"replace\"|\"add\"|\"test\"|\"remove\", \"path\": \"/json/pointer\", \"value\": ...}. Max 32 ops. Atomic: all succeed or none are applied.",
      "items": {
        "type": "object"
      }
    }
  },
  "required": ["namespace", "ops"]
}
```

**Handler logic:**
1. Validate `configManager` is available
2. Validate `namespace` is a string, `ops` is an array
3. Call `configManager->patchNamespaceConfig(ns, ops, result, &storedDoc)`
4. On success: return `{"status": "patched", "namespace": "<ns>", "ops_applied": N, "results": {...}}` where `results` contains the stored values from `storedDoc`
5. On failure: return `{"error": "<message>", "failed_op": N, "path": "<path>", "applied": false, "isError": true}`

**Error message mapping** (from `JsonPatch::Error` enum):
- `INVALID_POINTER` → "Invalid JSON Pointer"
- `PATH_NOT_FOUND` → "Path not found"
- `INDEX_OUT_OF_RANGE` → "Array index out of range"
- `ARRAY_REMOVE_BLOCKED` → "Remove on array index not supported (use replace with sentinel)"
- `TEST_FAILED` → "Test op failed — value mismatch (batch aborted)"
- `UNKNOWN_OP` → "Unknown op (supported: replace, add, test, remove)"
- `TOO_MANY_OPS` → "Too many ops (max 32)"
- `MUTEX_TIMEOUT` → "Config mutex busy — try again"

**Tool description string:**
`"Apply RFC 6902 JSON Patch ops to a single config namespace. Atomic: all ops succeed or none are applied. Returns stored (post-validation) values for patched paths."`

**Mutating flag:** Set `true` in `addTool()` (5th parameter) — this tool modifies config.

### 2. Include the JsonPatch header

Add `#include "../../modules/ConfigManager/JsonPatch.hpp"` at the top of the file alongside the existing ConfigManager include.

## Verification Requirement

1. Build `native` with PlatformIO MCP — must compile cleanly
2. Build `esp32dev` with PlatformIO MCP — must compile cleanly
3. Run existing native tests — 361/361 must pass (no regressions)

Once verified, **STOP and send a message back to me** with results. Do not wrap up or distill.
