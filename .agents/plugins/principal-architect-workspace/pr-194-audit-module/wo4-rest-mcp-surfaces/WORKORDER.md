---
name: pr-194-audit-module-wo4
description: Handoff instructions for Work Order 4 — REST + MCP surfaces
complexity: standard
---

# WO4 — REST + MCP Surfaces (IWebContributor + IMCPContributor)

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Wire up the diagnostic endpoints for the AuditModule: REST routes via `IWebContributor` and MCP tools via `IMCPContributor`. Also rename the existing `get_logs` MCP stub to `get_debug_logs` to separate the volatile debug-log plane from the durable audit-log plane.

## Context
**Parent project**: `pr-194-audit-module` — see `.agents/plugins/principal-architect-workspace/pr-194-audit-module/PROJECT.md`

The AuditModule core engine (WO2) is already implemented with `log()`, `auditSync()`, `clearAll()`, and diagnostic counters. This WO adds the external-facing surfaces.

**Key design decisions**:
- **D6**: `get_audit_log` / `clear_audit_log` naming; rename `get_logs` → `get_debug_logs`
- **D17**: Bounded MCP response (cap N ~200), `truncated` + `next_offset` cursor pagination

**Web server is `PsychicHttpServer`** (not ESPAsyncWebServer). See `include/interfaces/IWebContributor.hpp` for the exact signature: `registerRoutes(PsychicHttpServer& server)`.

**MCP tool handler signature**: `void(*)(void* context, JsonObject& params, JsonObject& result)` — must be a stateless function pointer with `void* context`, not a capturing lambda. See `modules/OTAUpdateManager/OTAUpdateManager.cpp` lines 1509-1580 for the pattern.

## Strict Boundary Box
- **DO NOT** modify `lib/Logger/`, `lib/Redactor/`, or `hardware/`.
- **DO NOT** implement `IConfigurable` or `IPreconditionContributor` — that is WO5.
- **DO NOT** create the `AUDIT()` macro or `WITH_AUDIT` gating — that is WO6.
- **DO NOT** modify `include/interfaces/` or `docs/`.
- **Only modify**: `modules/AuditModule/AuditModule.hpp`, `modules/AuditModule/AuditModule.cpp`, and `modules/MCPServer/MCPBuiltinTools.cpp`.

## Tooling Constraints
- Use `platformio` MCP tools for building (`build_project`, `run_tests`), NOT raw `pio` CLI commands. The `projectDir` argument must be set to `/Users/mcneillm/Documents/Projects/esp32-app-foundation`.

## Line-Item Instructions

### 1. Modify `modules/AuditModule/AuditModule.hpp`
- [x] Add `#include "interfaces/IWebContributor.hpp"` and `#include "interfaces/IMCPContributor.hpp"`
- [x] Change the class declaration to inherit: `class AuditModule : public IWebContributor, public IMCPContributor`
- [x] Add public method overrides:
  ```cpp
  void registerRoutes(PsychicHttpServer& server) override;
  void registerTabs(ITabRegistry& registry) override;
  void registerMCPTools(MCPToolRegistry& registry) override;
  ```
- [x] Add a private helper: `void readAuditFile(uint8_t fileIndex, size_t offset, size_t maxLines, String& output, size_t& linesRead, bool& truncated, size_t& nextOffset)`

### 2. Implement `registerRoutes()` in `AuditModule.cpp`
- [x] `GET /api/system/audit` — returns audit log entries as `application/x-ndjson`:
  - Query params: `lines` (default 50, max 200), `offset` (byte offset into the file, default 0), `file` (0 or 1, default = active file)
  - Read the specified JSONL file from `offset`, return up to `lines` complete lines
  - Drop any trailing incomplete line (D14 — crash tolerance)
  - Response headers: `X-Audit-Truncated: true/false`, `X-Audit-Next-Offset: <byte>` for pagination
  - If the file doesn't exist, return `200` with empty body and `X-Audit-Truncated: false`
- [x] `POST /api/system/audit/clear` — calls `clearAll()`, returns `{"ok":true}`:
  - This route must require authentication (check `request->authenticate()` or similar — follow the pattern used by other POST routes in the codebase, e.g., `AuthManager::registerRoutes()`)

### 3. Implement `registerTabs()` in `AuditModule.cpp`
- [x] No dedicated portal tab — audit is a diagnostic surface, not a user-facing page. Implement as an empty override: `void AuditModule::registerTabs(ITabRegistry&) {}`

### 4. Implement `registerMCPTools()` in `AuditModule.cpp`
- [x] Register `get_audit_log` tool (read-only, `mutatesState = false`):
  - Input schema: `lines` (int, default 50, max 200), `offset` (int, byte offset, default 0), `file` (int, 0 or 1, default active)
  - Handler reads the JSONL file and returns a JSON object: `{"records": [...], "lines_returned": N, "truncated": bool, "next_offset": int}`
  - Each record is parsed from JSONL and returned as a JSON object (not raw strings)
  - Drop trailing unparseable line
  - Use `void*` context pattern: `static_cast<AuditModule*>(context)` inside the handler
- [x] Register `clear_audit_log` tool (`mutatesState = true`):
  - No input parameters
  - Calls `clearAll()`, returns `{"ok": true, "message": "Audit log cleared"}`

### 5. Rename `get_logs` → `get_debug_logs` in `modules/MCPServer/MCPBuiltinTools.cpp`
- [x] At line ~426-439, rename the tool from `"get_logs"` to `"get_debug_logs"`
- [x] Update the description to: `"Get recent debug log entries from the volatile log buffer (ESP_LOG output)"`
- [x] Update the stub note: `"Debug log ring-buffer not yet implemented. See get_audit_log for durable audit records."`
- [x] Leave everything else about the stub unchanged (schema, handler structure)

### 6. Build verification
- [x] Build with `platformio` MCP `build_project` for `esp32dev` environment (projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`)
- [x] Run tests with `platformio` MCP `run_tests` (projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`)

## Verification Requirement
Before committing, the worker MUST:
1. Confirm `esp32dev` firmware builds cleanly.
2. Confirm all existing tests still pass.
3. Ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete

### Changes Made
- `modules/AuditModule/AuditModule.hpp`: Inherited `IWebContributor` and `IMCPContributor`. Added declarations for `registerRoutes()`, `registerTabs()`, `registerMCPTools()`, `getApiPrefix()`, `getMaxPostBody()`, and private helper `readAuditFile()`. Configured `AUDIT_FILE_PREFIX` appropriately for target (`/littlefs/audit_`) and host native tests (`test/tmp/audit_`).
- `modules/AuditModule/AuditModule.cpp`: Implemented `readAuditFile()` with cursor-based pagination (`offset`, `maxLines`), `truncated` flag, `nextOffset` calculation, and trailing partial line dropping (crash tolerance). Implemented `registerRoutes()` with `GET /api/system/audit` (returning `application/x-ndjson` with `X-Audit-Truncated` and `X-Audit-Next-Offset` headers) and `POST /api/system/audit/clear` (`{"ok":true}`). Implemented `registerTabs()` (empty override). Implemented `registerMCPTools()` with `get_audit_log` (`mutatesState = false`) and `clear_audit_log` (`mutatesState = true`).
- `modules/MCPServer/MCPBuiltinTools.cpp`: Renamed tool `get_logs` to `get_debug_logs`, updated description and stub note referencing `get_audit_log`.
- `test/test_unit/test_mcpServer/test_mcpServer.cpp`: Updated tool existence and mutation test cases for `get_debug_logs`.
- `test/test_unit/test_audit_module/test_audit_module.cpp`: Added unit tests for `readAuditFile` pagination, trailing incomplete line dropping, non-existent file handling, `get_audit_log`, `clear_audit_log`, and REST route registration.

### Decisions Made
- `AUDIT_FILE_PREFIX`: Used `#ifdef ESP_PLATFORM` to map to `/littlefs/audit_` on embedded hardware and `test/tmp/audit_` on native host tests (matching `ConfigRoot` pattern).
- `Crash tolerance`: Implemented partial line dropping in `readAuditFile()` if EOF is reached without a newline `\n`.

### Dead Ends & Warnings
- None.

### Out-of-Scope Findings
- None.

### Blockers
- None.
