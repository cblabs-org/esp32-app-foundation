---
name: lvgl-display-wo8
description: Handoff instructions for Work Order 8 — MCP tools (IMCPContributor)
complexity: standard
---

# lvgl-display / WO8 — MCP Tools (IMCPContributor)

## Provenance & Authorization
- **Parent project/PR:** PR-47 — [`../PROJECT.md`](../PROJECT.md)
- **Base branch:** `lvgl-display/wo7-web-portal`
- **Your branch:** `lvgl-display/wo8-mcp-tools` (created for you — **do not branch**)
- **Stack position:** WO8 of 11.
- **Authorization:** WORKORDER + `.agents/rules/`, valid once PR-47 `PROJECT.md` records the human's decomposition approval. Verify; if absent, **stop and report**.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Make `LVGLDisplayModule` an `IMCPContributor`: register the `display_status`, `display_control`, and `capture_display` JSON-RPC tools, reusing the WO7 capture primitive.

## Context You Need
- Read the `framework-expert` MCP Server docs and `IMCPContributor` contract; mirror the existing display MCP tools (`pr-34-mcp-tools`) and their tests in `test/test_unit/test_mcpServer/test_displayMcpTools.cpp`.
- **ArduinoJson 7 pitfall (must heed):** for tool args use `is<JsonObjectConst>()` not `is<JsonObject>()` (Issue #13). Confirm against the current MCP arg-parsing convention.
- `capture_display` must reuse WO7's capture-primitive method — do not reimplement snapshotting.

## Strict Boundary Box — do NOT:
- Re-implement display capture (call WO7's method).
- Add web routes or change config schema.
- Alter the MCP server core or other modules' tools.

## Tooling Constraints
`platformio` MCP tools (`build_project`, `run_tests`) via `pio-manager` — never raw `pio`. `house-style-docs`. Heed `.agents/rules/`.

## Line-Item Instructions
- [x] Add `public IMCPContributor`; implement `registerMCPTools(MCPToolRegistry&)` for:
  - `display_status` — brightness, screensaver state, active screen, resolution.
  - `display_control` — set brightness, wake/sleep, show message.
  - `capture_display` — returns a snapshot via the WO7 primitive.
- [x] Follow the JSON-RPC schema/registration and arg-parsing conventions of existing tools (mind Issue #13 `JsonObjectConst`).
- [x] Add tests mirroring `test/test_unit/test_mcpServer/test_displayMcpTools.cpp` (and/or under `test_lvglDisplay/`): tool discovery, arg handling, each tool's contract.

## Verification Requirement
- [x] `run_tests` (native) — MCP tool tests pass, full suite green.
- [x] `build_project` `env:esp32s3nano` green.

## Completion Protocol
1. Validate, then **STOP and message the Principal Architect** with results. Surface out-of-boundary findings; do not file issues.
2. **Do not distill until the human approves.**
3. On approval: `@distillery` with target `.agents/skills/principal-architect-workspace/pr-47-lvgl-display/wo8-mcp-tools/`.
4. Append a status update to this `WORKORDER.md` and terminate.

## Status Log
| Date | Event |
|------|-------|
| 2026-07-24 | WO8 execution complete. `LVGLDisplayModule` inherits `IMCPContributor` and registers `display_status`, `display_control`, and `capture_display`. 4 new unit tests added in `test_mcp_tools.cpp`. 487/487 native tests passed; `esp32s3nano` build green. |

