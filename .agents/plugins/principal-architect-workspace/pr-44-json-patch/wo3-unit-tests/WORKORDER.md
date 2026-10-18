---
name: pr-44-wo3
description: Handoff instructions for Work Order 3 — unit tests for JSON Patch engine and ConfigManager patch integration
complexity: standard
---

# WO3 — JSON Patch Unit Tests

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Write comprehensive unit tests covering the JSON Patch engine (`JsonPatch`) and the `ConfigManager::patchNamespaceConfig()` integration. The test cases are specified in Issue #28 §11.

## Strict Boundary Box

- **ONLY** modify/create files in `test/test_unit/test_configManager/`
- **DO NOT** modify ConfigManager, JsonPatch, MCPBuiltinTools, or any source files
- **DO NOT** modify stub files unless absolutely required for test compilation
- Surface any out-of-scope discoveries to me (the PA) in your completion report

## Tooling Constraints

- Use the **PlatformIO MCP server** tools for building and running native tests
- Follow existing test patterns in `test/test_unit/test_configManager/test_configManager.cpp` (GoogleTest, `MockModule` pattern)
- All tests run on the `native` environment only

## Reference Code

- `modules/ConfigManager/JsonPatch.hpp` — `Error` enum, `PatchResult` struct, `applyOne()`, `validatePointer()` signatures
- `modules/ConfigManager/JsonPatch.cpp` — implementation of pointer resolution and op dispatch
- `modules/ConfigManager/ConfigManager.hpp` — `patchNamespaceConfig()` signature
- `test/test_unit/test_configManager/test_configManager.cpp` — existing test patterns, `MockModule` class

## Line-Item Instructions

### 1. Create `test/test_unit/test_configManager/test_jsonPatch.cpp`

Standalone unit tests for `JsonPatch` static methods (no ConfigManager dependency needed). Use GoogleTest. Include house-style header block.

**Test cases for `validatePointer()`:**
- Valid pointer `/a/b/c` → `Error::OK`
- Empty string `""` → `Error::INVALID_POINTER`
- No leading `/` (`"a/b"`) → `Error::INVALID_POINTER`
- Trailing `/` (`"/a/"`) → `Error::INVALID_POINTER`
- Depth exceeds 6 → `Error::INVALID_POINTER`
- Token exceeds 32 chars → `Error::INVALID_POINTER`
- Single segment `/a` → `Error::OK`
- Root-level `/` (just a slash — single empty token edge case) — test the actual behavior

**Test cases for `applyOne()` — replace:**
- Replace existing key → value updated
- Replace non-existent key → `Error::PATH_NOT_FOUND`
- Replace nested path `/a/b/c` → value updated
- Replace array element `/arr/2` → value updated
- Replace array out of range `/arr/99` → `Error::INDEX_OUT_OF_RANGE`

**Test cases for `applyOne()` — add:**
- Add new key to object → key created
- Add to array with `-` (append) → element appended
- Add to array at valid index → element inserted
- Add to array beyond size → `Error::INDEX_OUT_OF_RANGE`

**Test cases for `applyOne()` — test:**
- Test matching value → `Error::OK`
- Test non-matching value → `Error::TEST_FAILED`
- Test non-existent path → `Error::PATH_NOT_FOUND`

**Test cases for `applyOne()` — remove:**
- Remove object key → key removed
- Remove non-existent key → `Error::PATH_NOT_FOUND`
- Remove array index → `Error::ARRAY_REMOVE_BLOCKED`

**Test cases for `applyOne()` — edge cases:**
- Unknown op `"move"` → `Error::UNKNOWN_OP`
- Missing op field → `Error::UNKNOWN_OP`

**Test cases for pointer escaping:**
- Key containing `/` (use `~1` in pointer) → correctly resolves
- Key containing `~` (use `~0` in pointer) → correctly resolves
- Key containing `~0` literally → use `~01` in pointer → correctly resolves

### 2. Modify `test/test_unit/test_configManager/test_configManager.cpp`

Add a new test group for `patchNamespaceConfig()` integration. Use the existing `MockModule` or create a `PatchableModule` variant that has an array in its config for array-targeting tests.

**Required `PatchableModule`:**
A mock that serializes config like:
```json
{
  "label": "default",
  "enabled": true,
  "channels": [
    {"name": "ch0", "type": "voltage"},
    {"name": "ch1", "type": "current"},
    {"name": "ch2", "type": "voltage"}
  ]
}
```
With `applyConfig()` that reads these values back. Importantly, it should **clamp** some value (e.g. if `label` length > 20, truncate to 20) to test §7 (module validation stays authoritative).

**Integration test cases:**
1. **Mid-batch failure:** Ops 0-2 valid, op 3 invalid path → namespace byte-identical to before (verify via serializeConfig)
2. **Atomic test-and-replace:** `[{op: test, path: /label, value: "default"}, {op: replace, path: /label, value: "new"}]` → succeeds
3. **Test mismatch aborts batch:** `[{op: test, path: /label, value: "wrong"}, {op: replace, ...}]` → `TEST_FAILED`, nothing changed
4. **Module clamping (§7):** Patch an over-length label → assert the *clamped* value persists (not the raw patch)
5. **Single flash write:** Patch multiple fields → only one file write occurs (verify by checking the file exists and has all changes)
6. **Unknown namespace:** Patch non-existent namespace → error
7. **Too many ops:** Patch with >32 ops → `TOO_MANY_OPS` error
8. **Replace array element:** `/channels/1/name` → value changes, other channels untouched
9. **Remove on array index:** `{op: remove, path: /channels/1}` → `ARRAY_REMOVE_BLOCKED`
10. **Stored values returned:** Pass `storedValues` doc, verify it contains the patched paths with post-clamp values

## Verification Requirement

1. Build `native` with PlatformIO MCP — must compile cleanly
2. Run native tests — ALL must pass (including existing 361 + new tests)
3. Report the total new test count

Once verified, **STOP and send a message back to me** with results. Do not wrap up or distill.
