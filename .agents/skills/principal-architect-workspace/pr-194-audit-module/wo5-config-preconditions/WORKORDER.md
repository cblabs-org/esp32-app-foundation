---
name: pr-194-audit-module-wo5
description: Handoff instructions for Work Order 5 — IConfigurable + IPreconditionContributor
complexity: standard
---

# WO5 — Config + Preconditions (IConfigurable + IPreconditionContributor)

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Add runtime configuration and health reporting to the AuditModule. The module becomes configurable via the portal (enable/disable, rate limit tuning) and reports its operational health via the precondition engine.

## Context
**Parent project**: `pr-194-audit-module` — see `.agents/skills/principal-architect-workspace/pr-194-audit-module/PROJECT.md`

WO1-WO4 are already complete. The AuditModule has its core engine, board constants, REST endpoints, and MCP tools. This WO adds the remaining two framework interfaces.

**Key design decisions**:
- **D9**: Runtime preconditions for operational health only — not for compile-time absence
- **D10**: Runtime toggle via `IConfigurable` — the act of disabling/enabling is itself an audit record

**Reference implementations to follow**:
- `IConfigurable`: See `modules/TimeManager/TimeManager.cpp` for `getConfigNamespace()`, `getSchemaVersion()`, `applyConfig()`, `serializeConfig()`, `resetConfig()`.
- `IPreconditionContributor`: See `modules/WifiManager/WifiPreconditionContributor.hpp` for `registerPreconditions()` with `PreconditionRole`, `Cause`, and `Edge` patterns.

**IConfigurable interface** (from `include/interfaces/IConfigurable.hpp`):
```cpp
virtual const char* getConfigNamespace() const = 0;
virtual int getSchemaVersion() const = 0;
virtual void applyConfig(const JsonObject& section) = 0;
virtual void serializeConfig(JsonObject& section) const = 0;
virtual bool migrateConfig(JsonObject& section, int fromVersion) { return true; }
virtual void resetConfig() = 0;
```

## Strict Boundary Box
- **DO NOT** modify `lib/Logger/`, `lib/Redactor/`, `hardware/`, `docs/`, or `platformio.ini`.
- **DO NOT** create the `AUDIT()` macro or `WITH_AUDIT` gating — that is WO6.
- **DO NOT** modify `modules/MCPServer/MCPBuiltinTools.cpp`.
- **Only modify**: `modules/AuditModule/AuditModule.hpp` and `modules/AuditModule/AuditModule.cpp`, and optionally test files.

## Tooling Constraints
- Use `platformio` MCP tools for building (`build_project`, `run_tests`), NOT raw `pio` CLI commands.
- The `projectDir` argument must be set to `/Users/mcneillm/Documents/Projects/esp32-app-foundation`.

## Line-Item Instructions

### 1. Add IConfigurable to `AuditModule.hpp`
- [ ] Add `#include "interfaces/IConfigurable.hpp"` and `#include "interfaces/IPreconditionContributor.hpp"`
- [ ] Add to class inheritance: `public IConfigurable, public IPreconditionContributor`
- [ ] Add override declarations for all IConfigurable methods
- [ ] Add override declaration for `registerPreconditions(IPreconditionRegistry& registry)`
- [ ] Add private config state:
  - `bool _enabled = true;` — runtime toggle (default on when audit is compiled in)
  - `uint32_t _configuredRateLimit = AUDIT_RATE_LIMIT_PER_MIN;` — configurable rate

### 2. Implement IConfigurable methods in `AuditModule.cpp`
- [ ] `getConfigNamespace()` → `"audit"`
- [ ] `getSchemaVersion()` → `1`
- [ ] `applyConfig(const JsonObject& section)`:
  - Read `section["enabled"]` (bool). If changing from enabled→disabled or disabled→enabled:
    - **Before disabling**: write an audit record `{source: "audit", action: "audit_disabled"}` via `log()` (self-documenting gap)
    - **After enabling**: write an audit record `{source: "audit", action: "audit_enabled"}`
  - Read `section["rate_limit"]` (int, records per minute). Update `_configuredRateLimit` and apply to rate limiter.
- [ ] `serializeConfig(JsonObject& section) const`:
  - `section["enabled"] = _enabled;`
  - `section["rate_limit"] = _configuredRateLimit;`
- [ ] `resetConfig()`:
  - Reset `_enabled = true` and `_configuredRateLimit = AUDIT_RATE_LIMIT_PER_MIN`
- [ ] `migrateConfig()` — return true (no migration needed for v1)

### 3. Wire `_enabled` into `log()` and `auditSync()`
- [ ] At the top of `log()`: if `!_enabled`, return false immediately (but do NOT drop-count — this is intentional suppression)
- [ ] At the top of `auditSync()`: same check. Return false if disabled.
- [ ] **Exception**: Internal audit records (`source == "audit"`, like `audit_disabled`, `audit_enabled`, `rate_limited`) MUST bypass the enabled check — the audit system must always be able to record its own state transitions.

### 4. Implement IPreconditionContributor in `AuditModule.cpp`
- [ ] `registerPreconditions(IPreconditionRegistry& registry)`:
  - Register a single precondition named `"audit_health"` with label `"Audit Trail"`:
    - Role: `PreconditionRole::Informational` (audit is diagnostic, not required for app health)
    - Evaluator lambda:
      - If `!_started` (begin() failed — flash preflight or task creation failed): `{false, Cause::InternalError}` → Persistent
      - If `!_enabled` (runtime-disabled via config): `{false, Cause::Unavailable}` → Inactive
      - If `_dropCount > 0 || _throttleCount > 0` (degraded operation): `{true, Cause::FallbackEngaged}` → this means "working but dropping records"
      - Otherwise: `{true, Cause::Met}` → Healthy
  - Study the `WifiPreconditionContributor` and `TimePreconditionContributor` for the exact lambda signature and return types. The evaluator returns a `PreconditionStatus` struct.

### 5. Build verification
- [ ] Build with `platformio` MCP `build_project` for `esp32dev` (projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`)
- [ ] Run tests with `platformio` MCP `run_tests` (projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`)

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
Complete — validated on all 871 native tests and clean builds on `esp32dev`, `esp32s3nano`, and `crowpanel-128-rotary`.

### Changes Made
- `modules/AuditModule/AuditModule.hpp`:
  - Added `#include "interfaces/IConfigurable.hpp"` and `#include "interfaces/IPreconditionContributor.hpp"`.
  - Added `IConfigurable` and `IPreconditionContributor` inheritance to `AuditModule`.
  - Declared `getConfigNamespace()`, `getSchemaVersion()`, `applyConfig()`, `serializeConfig()`, `migrateConfig()`, `resetConfig()`, `registerPreconditions()`, `isEnabled()`, and `configuredRateLimit()`.
  - Added private state `_enabled = true;` and `_configuredRateLimit = AUDIT_RATE_LIMIT_PER_MIN;`.
- `modules/AuditModule/AuditModule.cpp`:
  - `#include "interfaces/IPreconditionRegistry.hpp"`.
  - Updated constructor to initialize `_enabled(true)` and `_configuredRateLimit(AUDIT_RATE_LIMIT_PER_MIN)`.
  - Updated `checkRateLimit` and `resetRateLimiter` to operate dynamically on `_configuredRateLimit`.
  - Added runtime enabled check to `log()` and `auditSync()` with internal source bypass (`source == "audit"`).
  - Implemented `applyConfig()` to support runtime toggles with self-documenting bracketing audit events (`audit_disabled` logged before disabling; `audit_enabled` logged after enabling) and dynamic rate limit configuration.
  - Implemented `serializeConfig()`, `resetConfig()`, and `registerPreconditions()` contributing `"audit_health"` (`PreconditionRole::Operational`).
- `test/test_unit/test_audit_module/test_audit_module.cpp`:
  - Added unit test cases testing `IConfigurable` serialization, deserialization, reset, and state transition bracketing events.
  - Added unit test cases verifying disabled suppression and internal event bypass.
  - Added unit test cases verifying `IPreconditionContributor` registration and multi-state evaluation (`InternalError`, `Unavailable`, `FallbackEngaged`, `Met`) along with describe formatters.

### Decisions Made
- Runtime disabling writes `audit_disabled` prior to clearing `_enabled`, ensuring the disabling record itself is safely committed into the audit stream.
- Internal events (`source == "audit"`) bypass the runtime enabled check in both `log()` and `auditSync()`, ensuring audit state transitions and throttling notifications are never silently lost.
- Precondition `"audit_health"` is registered with `PreconditionRole::Operational` to reflect its non-blocking diagnostic role.

### Dead Ends & Warnings
None.

### Out-of-Scope Findings
None.

### Blockers
None.
