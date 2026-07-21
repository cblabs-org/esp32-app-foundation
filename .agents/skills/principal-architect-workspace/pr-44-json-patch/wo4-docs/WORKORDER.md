---
name: pr-44-wo4
description: Handoff instructions for Work Order 4 — framework docs update for JSON Patch
complexity: mechanical
---

# WO4 — Framework Docs Update

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Update the framework-expert skill documentation to cover the new `patch_config` MCP tool, the `patchNamespaceConfig()` method, and the config mutex locking model.

## Strict Boundary Box

- **ONLY** modify files in `.agents/skills/framework-expert/`
- **DO NOT** modify any source code, test files, or other docs
- Surface any out-of-scope discoveries to me (the PA) in your completion report

## Line-Item Instructions

### 1. Modify `.agents/skills/framework-expert/systems/config/config-manager.md`

Read the file first. Then add a new section (after the existing `set_config` / import/export docs) covering:

**Element-Level Writes (JSON Patch)**
- `patchNamespaceConfig()` method: signature, atomic semantics (all-or-nothing), supported ops (replace, add, test, remove on objects)
- The `patch_config` MCP tool: wire format, response shape, error codes
- Example: renaming a single channel label (show the ~70 byte payload vs ~8 KB for set_config)
- Sizing table from Issue #28 §10 (rename 1 channel: 70B, edit 8 channels: 1.1 KB, relabel all 64: 4.5 KB)
- Limitations: `remove` on array indices blocked in v1, MAX_PATCH_OPS = 32, pointer depth ≤ 6

**Config Mutex**
- `_configMutex` protects the serialize/applyConfig window
- Timed acquisition (50ms) — returns error on timeout
- Flash I/O happens outside the lock (serialize under lock, persist outside)
- Why: prevents thread-safety window between HTTP task and main loop

### 2. Modify `.agents/skills/framework-expert/systems/web-portal/routes.md`

Read the file first. Add `patch_config` to the MCP tools listing alongside `get_config`, `set_config`, etc. One-line entry with description.

## Verification Requirement

1. Review that the docs are accurate and follow the existing style
2. No build needed — docs only

Once verified, **STOP and send a message back to me** with results. Do not wrap up or distill.
