---
title: WO3 MCPServer Stubs Refactor
distilled_at: 2026-07-01T06:48:00Z
original_plan_id: pr-41-native-build-stubs
artifacts:
  - context_artifacts/adr_mcpserver_stubs.md
  - context_artifacts/graveyard_mcpserver_stubs.md
---

### Executive Summary
Removed all `#ifdef NATIVE_BUILD` guards from the `MCPServer` module and successfully migrated hardware-specific implementations (e.g., `ESPClass`, `heap_caps`, `WiFi`, `millis`) into the `test/stubs/platform/` directory to ensure native build compatibility.

### Next Steps
- Verify the completion of WO3 and proceed to WO4 or the next applicable work order in the `pr-41-native-build-stubs` plan.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_mcpserver_stubs.md` - Details the stub additions and why `millis()` was relocated to avoid ODR conflicts.
- `context_artifacts/graveyard_mcpserver_stubs.md` - Documents failed attempts at solving the `millis()` multiple definition error.
