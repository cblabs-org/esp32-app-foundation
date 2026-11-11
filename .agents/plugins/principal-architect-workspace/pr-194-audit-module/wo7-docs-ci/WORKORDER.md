---
name: pr-194-audit-module-wo7
description: Handoff instructions for Work Order 7 — Framework-expert skill updates and developer guide
complexity: standard
---

# WO7 — Documentation + Framework-Expert Skill Updates

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Update the framework-expert skill to include AuditModule in its knowledge base, add developer guidance on audit usage patterns, and ensure the skill's routing table includes audit-related trigger terms.

## Context
**Parent project**: `pr-194-audit-module` — see `.agents/plugins/principal-architect-workspace/pr-194-audit-module/PROJECT.md`

WO1-WO6 are complete. The AuditModule is fully implemented, gated, and documented in the microkernel design doc. This final WO updates the agent skill documentation so future AI-assisted development sessions know about the audit system.

**Key patterns to document**:
- Two-tier API: `AUDIT()` macro (soft, no-op when off) vs direct `AuditModule` API (hard, compile error when off)
- Use `include/Audit.h` for shared modules, never `AuditModule.hpp` directly
- `SYNC_EACH` durability — every record is immediately flushed
- Board-specific budget via `BOARD_AUDIT_BYTES`
- Factory reset clears audit files
- Runtime toggle via config (`enabled`, `rate_limit`)

## Strict Boundary Box
- **DO NOT** modify any source code files (`modules/`, `lib/`, `include/`, `hardware/`, `src/`).
- **DO NOT** modify `platformio.ini` or `docs/microkernel-architecture-design.md` (already done in WO6).
- **Only modify**: Files under `.agents/skills/framework-expert/`.

## Line-Item Instructions

### 1. Update `.agents/skills/framework-expert/SKILL.md`
- [x] Read the existing SKILL.md to understand the structure
- [x] Add `AuditModule` to the skill's description trigger list. Add terms: `AUDIT`, `audit trail`, `audit log`, `WITH_AUDIT`, `AuditModule`, `Audit.h`, `get_audit_log`, `clear_audit_log`, `BOARD_AUDIT_BYTES`, `audit_health`
- [x] Add an `AuditModule` entry to the Interface Inventory table (if one exists)
- [x] Add `AuditModule` to the Module Registry (if one exists) with: status = optional, flag = `WITH_AUDIT`, interfaces = `IWebContributor`, `IMCPContributor`, `IConfigurable`, `IPreconditionContributor`

### 2. Create `.agents/skills/framework-expert/references/audit-module-guide.md`
- [x] Create a developer-facing guide covering:
  - **Overview**: What AuditModule does, when to use it
  - **Two-Tier API**:
    - `AUDIT()` macro from `include/Audit.h` — safe for shared modules, zero-cost when `WITH_AUDIT` is off
    - Direct `AuditModule::instance()` — only for modules that require audit, compile error without flag
  - **Schema**: `{t_ms, ts, seq, boot_id, build, source, action, target_id, details}`
  - **Usage examples**:
    ```cpp
    // In any shared module (soft dependency)
    #include "Audit.h"
    AUDIT("config", "setting_changed", entityId, "temperature=22");
    
    // For must-not-lose events
    AUDIT_SYNC("auth", "login_failed", userId, "invalid credentials");
    ```
  - **Board budget**: How `BOARD_AUDIT_BYTES` works, default 32 KiB, board overrides
  - **Rate limiting**: Default 120 records/min, configurable via IConfigurable
  - **Runtime toggle**: `applyConfig({enabled: false})` writes self-documenting gap
  - **Factory reset**: Clears audit files by default
  - **MCP tools**: `get_audit_log` (read-only, paginated), `clear_audit_log` (mutating)
  - **REST endpoints**: `GET /api/system/audit`, `POST /api/system/audit/clear`
  - **Precondition**: `audit_health` — InternalError/Unavailable/FallbackEngaged/Met
  - **Flash wear guidance**: SYNC_EACH is appropriate for audit (low volume, high importance), but warn against using AUDIT() in tight loops. Rate limiter protects flash.
  - **Anti-patterns**:
    - ❌ Don't `#include "AuditModule.hpp"` directly in shared modules
    - ❌ Don't call `AUDIT()` in ISRs or from PSRAM tasks
    - ❌ Don't call `auditSync()` from EventBus callbacks or the main loop tick
    - ❌ Don't log high-frequency sensor readings as audit events

### 3. Update the routing table in SKILL.md
- [x] If the SKILL.md has a routing table mapping topics to reference files, add:
  - `audit` → `references/audit-module-guide.md`
  - `AUDIT()` → `references/audit-module-guide.md`
  - `WITH_AUDIT` → `references/audit-module-guide.md`

## Verification Requirement
Before committing, the worker MUST:
1. Verify the guide is accurate by cross-referencing key claims against the actual source files (check at least: `include/Audit.h`, `modules/AuditModule/AuditModule.hpp`, `hardware/_common/board_contract.h`)
2. Ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report

### Status
Complete — all line items executed, verified against codebase sources, unit tests verified (872/872 pass), approved by user.

### Changes Made
1. Created `.agents/skills/framework-expert/references/audit-module-guide.md`:
   - Comprehensive guide covering overview, two-tier API (`include/Audit.h` macros vs direct `AuditModule::instance()`), JSONL record schema (`formatRecord`), flash budget (`BOARD_AUDIT_BYTES`), rate limiting (120/min token bucket), runtime config toggle (`enabled`, `rate_limit`), factory reset behavior, REST endpoints (`GET /api/system/audit`, `POST /api/system/audit/clear`), MCP tools (`get_audit_log`, `clear_audit_log`), operational precondition (`audit_health`), flash wear guidance, and anti-patterns.
2. Updated `.agents/skills/framework-expert/SKILL.md`:
   - Added `AuditModule` trigger terms (`AUDIT`, `audit trail`, `audit log`, `WITH_AUDIT`, `AuditModule`, `Audit.h`, `get_audit_log`, `clear_audit_log`, `BOARD_AUDIT_BYTES`, `audit_health`) to frontmatter `description`.
   - Added **Audit System** (`references/audit-module-guide.md`) to Detailed References table.
   - Added `Audit trail / event logging` section to Trigger Reference, plus `/api/system/audit*` REST endpoints and `get_audit_log` / `clear_audit_log` MCP tools.
3. Updated Supporting References:
   - `.agents/skills/framework-expert/references/build-system.md`: Added `WITH_AUDIT` to Optional-Module Build Flags table.
   - `.agents/skills/framework-expert/systems/mcp-server/overview.md`: Renamed `get_logs` to `get_debug_logs` and added `AuditModule` tools.
   - `.agents/skills/framework-expert/systems/web-portal/routes.md`: Added Audit REST Endpoints section and renamed `get_logs` to `get_debug_logs`.
   - `.agents/skills/framework-expert/skill_manifest.json`: Added `references/audit-module-guide.md`.

### Decisions Made
- Maintained strict isolation of documentation to `.agents/skills/framework-expert/` per the Strict Boundary Box.
- Documented both `AUDIT()` and `AUDIT_SYNC()` in the two-tier API section to provide full coverage of asynchronous and synchronous use cases.

### Dead Ends & Warnings
- None encountered.

### Out-of-Scope Findings
- None.

### Blockers
- None.
