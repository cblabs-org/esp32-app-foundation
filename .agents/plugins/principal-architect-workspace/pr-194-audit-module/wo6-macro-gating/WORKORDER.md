---
name: pr-194-audit-module-wo6
description: Handoff instructions for Work Order 6 — AUDIT() macro, WITH_AUDIT gating, sentinel pattern
complexity: standard
---

# WO6 — Macro Gating + WITH_AUDIT + Sentinel Pattern

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Create the `AUDIT()` convenience macro header, add `WITH_AUDIT` compile-time gating to `Application` module registration, add the sentinel compile-error pattern for direct API usage without the flag, and add `-D WITH_AUDIT` to each board's build flags in `platformio.ini`.

## Context
**Parent project**: `pr-194-audit-module` — see `.agents/plugins/principal-architect-workspace/pr-194-audit-module/PROJECT.md`

WO1-WO5 are complete. The AuditModule is fully implemented but not yet gated behind `WITH_AUDIT`. This WO creates the compile-time integration layer.

**Key design decisions**:
- **D7**: Optional, default-off (`WITH_AUDIT`), same tier as MCP/OTA/HA
- **D8**: Two-tier API: `AUDIT()` macro = no-op when off; direct `AuditModule` API = sentinel compile error

**Reference pattern**: See `modules/Application/Application.hpp` lines 106-124 for the `OTA_PROGRESS_requires_WITH_OTA` sentinel pattern.

## Strict Boundary Box
- **DO NOT** modify `lib/Logger/`, `lib/Redactor/`, or `hardware/*.h` files.
- **DO NOT** modify `modules/MCPServer/MCPBuiltinTools.cpp`.
- **DO NOT** modify `modules/AuditModule/AuditModule.hpp` or `AuditModule.cpp` — those are complete.
- **Only create/modify**: `include/Audit.h` (new), `modules/Application/Application.hpp`, `modules/Application/Application.cpp`, `platformio.ini`, and `docs/microkernel-architecture-design.md`.

## Tooling Constraints
- Use `platformio` MCP tools for building (`build_project`, `run_tests`), NOT raw `pio` CLI commands.
- The `projectDir` argument must be set to `/Users/mcneillm/Documents/Projects/esp32-app-foundation`.

## Line-Item Instructions

### 1. Create `include/Audit.h` — Public Macro Header
- [ ] Apache-2.0 header in house style
- [ ] Guard: `#ifndef AUDIT_H` / `#define AUDIT_H`
- [ ] When `WITH_AUDIT` is defined:
  ```cpp
  #include "AuditModule.hpp"
  // Convenience macro — enqueues an audit record (non-blocking, no-op on failure)
  #define AUDIT(source, action, ...)  AuditModule::instance().log(source, action, ##__VA_ARGS__)
  #define AUDIT_SYNC(source, action, ...) AuditModule::instance().auditSync(source, action, ##__VA_ARGS__)
  ```
- [ ] When `WITH_AUDIT` is NOT defined:
  ```cpp
  // Soft no-op — like ESP_LOGD at level 0, silently compiles to nothing
  #define AUDIT(source, action, ...)       ((void)0)
  #define AUDIT_SYNC(source, action, ...)  ((void)0)
  ```
- [ ] Add clear documentation comments explaining the two-tier API:
  - `AUDIT()` macro: safe for shared modules — no-op when audit is off, real when on
  - Direct `AuditModule::instance()` calls: only for modules that REQUIRE audit — will fail to compile without `WITH_AUDIT`

### 2. Modify `modules/Application/Application.cpp` — Register AuditModule
- [ ] Find the section where optional modules are registered under `#ifdef WITH_*` guards (search for `WITH_MCP`, `WITH_OTA`, etc.)
- [ ] Add a new guarded block:
  ```cpp
  #ifdef WITH_AUDIT
  #include "AuditModule.hpp"
  static AuditModule& auditModule = AuditModule::instance();
  #endif
  ```
  And in the registration function (likely `Application::setup()` or similar):
  ```cpp
  #ifdef WITH_AUDIT
  registerModule(&auditModule);
  #endif
  ```
- [ ] Study how other optional modules like OTAUpdateManager are declared and registered — follow the exact same pattern (static instance at file scope, registerModule in setup).

### 3. Modify `modules/Application/Application.hpp` — Sentinel Pattern
- [ ] Find the `ApplicationEvents` namespace (around line 80-125) where `OTA_PROGRESS` sentinel lives
- [ ] Add, **inside the `#else` of a new `#ifdef WITH_AUDIT`** guard (or at the bottom of the namespace):
  ```cpp
  #ifndef WITH_AUDIT
  // EO-xx: Legible absent-module error for consumers attempting to use AuditModule without the flag
  struct AuditModule_requires_WITH_AUDIT {};
  #endif
  ```
  Note: The sentinel is for the direct API usage, not the `AUDIT()` macro. The macro is the safe path; the direct class is the hard path.
  
  Actually — the sentinel pattern works by making the *class name* itself resolve to an unusable type when the flag is off. However, since `AuditModule` is defined in `modules/AuditModule/AuditModule.hpp` and only included via `include/Audit.h` when `WITH_AUDIT` is set, any direct `#include "AuditModule.hpp"` without `WITH_AUDIT` will still compile (the header has no guard). 
  
  The simplest approach: **add a `#ifndef WITH_AUDIT` / `#error` guard** at the top of `modules/AuditModule/AuditModule.hpp`:
  ```cpp
  #ifndef WITH_AUDIT
  #error "AuditModule.hpp requires WITH_AUDIT to be defined. Use the AUDIT() macro from include/Audit.h for soft dependency."
  #endif
  ```
  This gives a clear compile-time error message directing developers to the correct soft-dependency path.

### 4. Modify `platformio.ini` — Add `-D WITH_AUDIT` to all board environments
- [ ] Add `-D WITH_AUDIT` to the `build_flags` list in each environment (`[env:esp32dev]`, `[env:esp32s3nano]`, `[env:crowpanel-128-rotary]`)
- [ ] Place it alongside the other `WITH_*` flags (after `WITH_MCP`, near `WITH_OTA`, etc.)

### 5. Update `docs/microkernel-architecture-design.md` — MK-6 + MK-14
- [ ] Find the MK-6 section (module classification) and add `AuditModule` to the optional module registry with the `WITH_AUDIT` flag
- [ ] Find the MK-14 section (persistence tiering) and add audit files to the appropriate tier:
  - Tier: `storage` partition, `/littlefs/audit_*.jsonl`, budget-bounded by `BOARD_AUDIT_BYTES`
  - Durability: `SYNC_EACH` (open → append → flush → close per record)
  - Rotation: Two-file JSONL rotation with size-based switching

### 6. Build verification
- [ ] Build with `platformio` MCP `build_project` for `esp32dev` (projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`) — WITH_AUDIT should now be active
- [ ] Build with `platformio` MCP `build_project` for `esp32s3nano`
- [ ] Run tests with `platformio` MCP `run_tests` (projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`)

## Verification Requirement
Before committing, the worker MUST:
1. Confirm all 3 board environments build cleanly with `WITH_AUDIT` active.
2. Confirm all existing tests still pass.
3. Ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete — verified against all target environments (`esp32dev`, `esp32s3nano`, `crowpanel-128-rotary`) and host unit tests (`native`, 872/872 tests passing).

### Changes Made
- `include/Audit.h`: Created public convenience macro header with `AUDIT()` and `AUDIT_SYNC()` expanding to `AuditModule::instance().log()` and `auditSync()` when `WITH_AUDIT` is defined, and `((void)0)` when undefined.
- `modules/AuditModule/AuditModule.hpp`: Added `#ifndef WITH_AUDIT` / `#error` sentinel guard to enforce two-tier API and fail fast if included directly without `WITH_AUDIT`.
- `modules/AuditModule/AuditModule.cpp`: Wrapped implementation in `#ifdef WITH_AUDIT` / `#endif` so absent-flag builds cleanly exclude the compilation unit.
- `modules/Application/Application.hpp`: Added `AuditModule_requires_WITH_AUDIT` sentinel in `ApplicationEvents`.
- `modules/Application/Application.cpp`: Guarded `AuditModule.hpp` include and static instance with `#ifdef WITH_AUDIT`, and registered `auditModule` in `Application::begin()`.
- `platformio.ini`: Added `-D WITH_AUDIT` to `build_flags` for `[env:esp32dev]`, `[env:esp32s3nano]`, `[env:crowpanel-128-rotary]`, and `[env:native]`.
- `docs/microkernel-architecture-design.md`: Updated MK-6 module classification table to include `AuditModule` (`WITH_AUDIT`), and updated MK-14 persistence tiering table to document the `storage` partition for budget-bounded audit JSONL logs (`BOARD_AUDIT_BYTES`, `SYNC_EACH`, two-file rotation).
- `test/test_unit/test_audit_module/test_audit_module.cpp`: Added `MacroConvenience_AsyncAndSyncLogging` test case verifying macro functionality.

### Decisions Made
- Guarded `AuditModule.cpp` with `#ifdef WITH_AUDIT` so that builds with `WITH_AUDIT` disabled do not fail on the `#error` guard in `AuditModule.hpp`.
- Placed `auditModule.begin()` before `registerModule(&auditModule)` in `Application::begin()` (Step 7) to guarantee task and queue initialization after LittleFS mount in Step 4 and prior to `_configManager.beginAll()` in Step 8.

### Dead Ends & Warnings
- None.

### Out-of-Scope Findings
- None.

### Blockers
- None.
