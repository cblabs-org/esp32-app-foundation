---
project: pr-194-audit-module
parent-issue: https://github.com/matthewmcneill/esp32-app-foundation/issues/194
base-branch: main
---

# PR-194: Generalized Flash-Persisted AuditModule

## Architectural Summary

Implement a generalized `AuditModule` providing a standardized, flash-persisted audit trail for significant application events. The module follows the DataManager worker-task pattern (DRAM queue + low-priority writer), uses two-file JSONL rotation on the `storage` partition, and surfaces records via REST and MCP. A prerequisite `lib/Redactor` extraction decouples secret redaction from the Logger so both Logger and AuditModule can share a single secret registry without coupling to each other.

**Key design decisions** (19 total, documented in the implementation plan):
- `SYNC_EACH` durability (open → append → flush → close per record)
- Two-tier API: `AUDIT()` macro (soft, no-op when off) + direct API (hard, sentinel compile error when off)
- Board-derived flash budget (`BOARD_AUDIT_BYTES`) with free-space preflight
- Factory reset clears audit files by default
- `get_logs` renamed to `get_debug_logs` (volatile plane); `get_audit_log` is the durable plane
- Build version (`BUILD_VERSION`) in every audit record
- Runtime toggle writes self-documenting bracketing records

## Work Order Ledger

- **[x] [wo1-redactor-extraction](wo1-redactor-extraction/WORKORDER.md):** Extract secret registry + redaction from `lib/Logger` into dependency-free `lib/Redactor` leaf *(commit `8d2f471a`, 851 tests pass)*
- **[x] [wo2-audit-core](wo2-audit-core/WORKORDER.md):** AuditModule core engine — queue, DRAM writer task, JSONL rotation, rate limiter, `log()`, `auditSync()`, `clearAll()` *(commit `7a1800c4`, 861 tests pass)*
- **[x] [wo3-board-constants](wo3-board-constants/WORKORDER.md):** Add `BOARD_AUDIT_BYTES` to board.h files and `board_contract.h` fallback *(commit `0f85c42d`, all boards build clean)*
- **[x] [wo4-rest-mcp-surfaces](wo4-rest-mcp-surfaces/WORKORDER.md):** IWebContributor + IMCPContributor — REST endpoints, MCP tools, `get_logs` → `get_debug_logs` rename *(commit `696e44e6`, 867 tests pass)*
- **[x] [wo5-config-preconditions](wo5-config-preconditions/WORKORDER.md):** IConfigurable + IPreconditionContributor — runtime toggle, health reporting *(commit `c5813561`, 871 tests pass)*
- **[x] [wo6-macro-gating](wo6-macro-gating/WORKORDER.md):** `AUDIT()` macro, `WITH_AUDIT` build flag, sentinel pattern, MK-6/MK-14 doc updates *(commit `e1ff0a3e`, 872 tests pass)*
- **[x] [wo7-docs-ci](wo7-docs-ci/WORKORDER.md):** Framework-expert skill updates, developer guide, CI build-matrix case *(872 tests pass)*

## Modules & Components

| Module/Area | Status | Notes |
|---|---|---|
| `lib/Redactor` | New | Extracted from Logger |
| `lib/Logger` | Modified | Delegates to Redactor |
| `modules/AuditModule` | New | Core module |
| `hardware/*/board.h` | Modified | BOARD_AUDIT_BYTES |
| `hardware/_common/board_contract.h` | Modified | Fallback constant |
| `modules/MCPServer/MCPBuiltinTools.cpp` | Modified | get_logs → get_debug_logs |
| `include/Audit.h` | New | Public macro header |
| `docs/microkernel-architecture-design.md` | Modified | MK-6 + MK-14 updates |
| `.agents/skills/framework-expert/` | Modified | Skill docs |

## Session Log

### Session 1 — 2026-08-29/30: RFC Review → Plan → Implementation

**RFC Review & Design**: Reviewed Issue #194 proposal. Posted initial review (6 items: thread safety, flash wear, interfaces, NTP, rotation, clear endpoint). Gathered external input (3 deep-dive comments covering flash budget blocker on 4MB boards, MK-14 tiering, Logger coupling, secret leakage, `get_logs` reconciliation). Iterated through Logger-independence breakthrough (extracted Redactor concept), precondition scoping (compile-off = no precondition, runtime-only), and two-tier API design (soft `AUDIT()` macro + hard direct API sentinel).

**Decisions recorded on Issue #194**: 19 consolidated decisions (D1–D19). All open questions resolved:
- Factory reset clears audit (D18)
- `get_logs` → `get_debug_logs` + `get_audit_log` separation (D6)
- Build version in schema (D19)

**Implementation**: Proceeding with 7-WO decomposition. Base branch: `main` at `72c5cf12`.

**Human approval**: User explicitly approved the implementation plan and said "please proceed with implementation" at 2026-08-30T09:17:11+01:00. Decomposition authorized for autonomous worker execution.

### 2026-08-30T09:30:00+01:00 — WO1 Complete: Redactor Extraction
**Summary**: Extracted `lib/Redactor` (secret registry + `redact()`) from `lib/Logger` as a dependency-free leaf library. Logger now delegates to Redactor via thin shim. 7 new unit tests.
**Cross-cutting decisions**: Parameterized `bufferSize` on `redact()` (default 512) so AuditModule can pass its own buffer size without coupling to Logger's constant. Thread safety contract documented: `registerSecret()` during init only.

### 2026-08-30T09:45:00+01:00 — WO2 Complete: AuditModule Core Engine
**Summary**: Core engine with DRAM queue (depth 16), background writer task (prio 2, 4 KiB stack), dual-file JSONL rotation, token-bucket rate limiter (120/min), `log()`, `auditSync()` (semaphore-based), `clearAll()`, and `formatRecord()` static method. 10 new unit tests.
**Cross-cutting decisions**: Schema frozen: `{t_ms, ts, seq, boot_id, build, source, action, target_id, details}`. `SYNC_EACH` durability model. `std::nothrow` allocation with explicit drop counting.

### 2026-08-30T09:55:00+01:00 — WO3 Complete: Board Constants
**Summary**: Added `BOARD_AUDIT_BYTES` to all board.h files and `board_contract.h` fallback (32 KiB). esp32dev: 16 KiB, PSRAM boards: 64 KiB.
**Dead ends**: Worker couldn't use MCP tools for building (used raw `pio` CLI which failed). PA had to build+commit manually. **Lesson**: WORKORDER must include explicit MCP tool guidance with `projectDir` parameter.

### 2026-08-30T10:10:00+01:00 — WO4 Complete: REST + MCP Surfaces
**Summary**: AuditModule inherits `IWebContributor` + `IMCPContributor`. REST: `GET /api/system/audit` (NDJSON, cursor pagination, crash-tolerant), `POST /api/system/audit/clear`. MCP: `get_audit_log` (paginated, read-only), `clear_audit_log` (mutatesState=true). Renamed `get_logs` → `get_debug_logs` in MCPBuiltinTools.
**Cross-cutting decisions**: `readAuditFile()` private helper drops trailing incomplete lines for crash tolerance. D6 update confirmed: volatile debug logs vs durable audit logs are distinct planes.

### 2026-08-30T10:20:00+01:00 — WO5 Complete: Config + Preconditions
**Summary**: AuditModule inherits `IConfigurable` + `IPreconditionContributor`. Config namespace `"audit"`, schema v1, fields: `enabled` (bool), `rate_limit` (int). Self-documenting bracketing: `audit_disabled` written BEFORE disabling, `audit_enabled` AFTER enabling. Internal records (source=="audit") bypass the enabled check. Precondition `audit_health` with 4 states.
**Cross-cutting decisions**: Sticky degradation — `FallbackEngaged` remains until `clearAll()`. This is intentional: audit trails should surface historical reliability problems.

### 2026-08-30T10:44:00+01:00 — WO6 Complete: Macro Gating
**Summary**: Created `include/Audit.h` with `AUDIT()`/`AUDIT_SYNC()` macros (no-op when `WITH_AUDIT` off). Added `#error` sentinel guard in `AuditModule.hpp`. Added `-D WITH_AUDIT` to all environments. Updated MK-6 and MK-14 in microkernel design doc.
**Cross-cutting decisions**: Sentinel pattern uses `#error` in the header (clearer than the OTA struct-based sentinel). Macro test added to native test suite.

### 2026-08-30T10:48:00+01:00 — WO7 Complete: Docs + Framework-Expert Skill
**Summary**: Created `references/audit-module-guide.md` developer guide. Updated SKILL.md routing table, trigger terms, MCP overview, web portal routes, and build system docs.

### 2026-08-30T15:25:00+01:00 — Project Wrap-Up

**Architectural outcome**: The ESP32 App Foundation now has a generalized, optional audit trail subsystem. Any module — framework or consumer — can emit structured audit records via `#include "Audit.h"` and `AUDIT(source, action, ...)` with zero coupling when `WITH_AUDIT` is off. The system implements all 4 framework interfaces (IWebContributor, IMCPContributor, IConfigurable, IPreconditionContributor), establishing AuditModule as the reference implementation for a "full-surface" optional module. 38 files changed, +3,291 / -66 lines, 872 tests, PR #203 merged.

**Lessons for future projects**:
1. **WORKORDER must include explicit MCP tool guidance** — WO3 worker failed when it used raw `pio` CLI instead of MCP tools. All subsequent WORKORDERs included `projectDir` parameter guidance, and no further failures occurred.
2. **Two-tier macro API pattern is reusable** — the `AUDIT()` macro / `#error` sentinel pattern can be templated for any future optional module that needs a soft-dependency path for shared code.
3. **Pre-release audit catches real issues** — the framework-expert release audit protocol found 6 house-style findings (5 fixed, 1 deferred). Running it before merge avoids post-merge cleanup PRs.
4. **`get_logs` → `get_debug_logs` rename is a breaking change** — any MCP client scripts targeting the old name will break. This should be called out in release notes.

**Unresolved items**:
- D4 (DRAM) hardware soak test not yet performed — `heap_min_free` with audit active needs measurement on physical hardware before stable release.
- D1 finding #6 deferred: `// --- Step N` flow comments in short helper functions (seedActiveFile, formatRecord, readAuditFile, enqueueInternal, checkRateLimit).
- `get_debug_logs` is still a renamed stub (returns debug log output) — a proper PSRAM ring-buffer implementation for debug logs is a future enhancement.
