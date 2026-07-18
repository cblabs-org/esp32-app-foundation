---
title: Issue #13 — MCPServer Tool Arguments Silently Dropped (ArduinoJson 7 const-variant bug)
distilled_at: 2026-07-18T17:49:00Z
original_plan_id: standalone
github_pr: 14
github_issue: 13
merged_commit: ced3aba
tag: v0.0.12-dev.27
artifacts:
  - context_artifacts/adr_const_variant_fix.md
  - context_artifacts/graveyard_investigation.md
---

## Executive Summary

Every MCP tool handler silently received an empty `arguments` object regardless of what the JSON-RPC client sent. Root cause: ArduinoJson 7's `is<JsonObject>()` always returns `false` on a `JsonVariantConst` (produced by subscripting a `const JsonDocument&`). Fixed with a one-character type change to `is<JsonObjectConst>()`, plus a regression test and documentation guard. GitHub PR #14 merged to `main` at `ced3aba`.

## What Was Done

Single-commit fix (`06aacc9`) containing three changes:

1. **`modules/MCPServer/MCPServer.cpp:294`** — `is<JsonObject>()` → `is<JsonObjectConst>()` + expanded Step 3 comment documenting the pitfall.
2. **`test/test_unit/test_mcpServer/test_mcpServer.cpp`** — New test `ToolCallArgumentsReachHandler` that dispatches a full JSON-RPC `tools/call` with `{"value": 99}` and asserts the handler received it. Fails without fix, passes with it.
3. **`docs/developer/mcp-server.md`** — `[!WARNING]` callout in the Handler Type section explaining `JsonObjectConst` requirement for any internal code operating on a `const JsonDocument&`.

## Verification

| Check | Result |
|---|---|
| Native tests (361 cases) | ✅ 361/361 PASSED |
| Framework `esp32dev` | ✅ Flash 82.2%, RAM 16.5% |
| Framework `esp32s3nano` | ✅ Flash 32.1%, RAM 17.1% |
| Sample-app `esp32dev` | ✅ Flash 89.9%, RAM 16.6% |
| Sample-app `esp32s3nano` | ✅ Flash 34.7%, RAM 16.9% |
| Template-app `esp32dev` | ✅ Flash 80.9%, RAM 14.3% |
| Template-app `esp32s3nano` | ✅ Flash 31.3%, RAM 14.6% |

## Next Steps

None — issue is closed, PR merged, regression test in place. If a future session touches `_handleToolsCall`, verify all `is<>()` calls use `Const` variants where the source is a `const JsonDocument&` or `JsonVariantConst`.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_const_variant_fix.md` — The exact ArduinoJson 7 type system asymmetry, why `JsonObjectConst` is required, and the doc+test strategy
- `context_artifacts/graveyard_investigation.md` — Initial investigation approach, sandbox network restrictions encountered, workarounds used
