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

- **[ ] [wo1-redactor-extraction](wo1-redactor-extraction/WORKORDER.md):** Extract secret registry + redaction from `lib/Logger` into dependency-free `lib/Redactor` leaf
- **[ ] [wo2-audit-core](wo2-audit-core/WORKORDER.md):** AuditModule core engine — queue, DRAM writer task, JSONL rotation, rate limiter, `log()`, `auditSync()`, `clearAll()`
- **[ ] [wo3-board-constants](wo3-board-constants/WORKORDER.md):** Add `BOARD_AUDIT_BYTES` to board.h files and `board_contract.h` fallback
- **[ ] [wo4-rest-mcp-surfaces](wo4-rest-mcp-surfaces/WORKORDER.md):** IWebContributor + IMCPContributor — REST endpoints, MCP tools, `get_logs` → `get_debug_logs` rename
- **[ ] [wo5-config-preconditions](wo5-config-preconditions/WORKORDER.md):** IConfigurable + IPreconditionContributor — runtime toggle, health reporting
- **[ ] [wo6-macro-gating](wo6-macro-gating/WORKORDER.md):** `AUDIT()` macro, `WITH_AUDIT` build flag, sentinel pattern, MK-6/MK-14 doc updates
- **[ ] [wo7-docs-ci](wo7-docs-ci/WORKORDER.md):** Framework-expert skill updates, developer guide, CI build-matrix case

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
