# PR-211: Logging Subsystem Unification

> **Issues:** [#210](https://github.com/matthewmcneill/esp32-app-foundation/issues/210) (closed), [#211](https://github.com/matthewmcneill/esp32-app-foundation/issues/211) (open)
> **Base branch:** `main`
> **Stack tip:** `pr-211-logging/wo4-docs` (when complete)

---

## Architectural Context

The design specification (§11A) describes a complete logging and observability
subsystem — decisions D27–D32 were documented and the architecture diagram shows
`LogRouter (vprintf pipeline)` as a kernel component. **None of it was built.**
What exists: a decorative `Logger` wrapper bypassed by 538/545 log sites, a
`Redactor` with a runtime thread-safety race, and spec-reality divergence that
creates a false sense of security.

This PR implements the core redaction pipeline that the spec designed: intercept
all `ESP_LOG*` output at the sink via `esp_log_set_vprintf()`, redact secrets
globally via a hardened Redactor, and output to serial. The redundant `Logger`
library is deleted.

### Key Design Decisions (approved in session)

1. **`esp_log_set_vprintf()` hook** — global redaction at the sink. Zero call-site churn. All 538 `ESP_LOG*` sites covered without code changes.
2. **Redactor promoted to Singleton** with `ISanitizer` interface. Fixed-capacity lock-free array (16 secrets, `std::atomic` count). Zero-allocation `strstr` + `memset` in-place redaction.
3. **`ISanitizer` interface** for DIP — AuditModule depends on abstraction, not concrete class. LogPipeline uses Singleton directly (C callback constraint).
4. **KeyManager as primary feeder** — auto-registers secrets on decrypt. Callers never think about redaction. Manual `registerSecret()` stays public for non-key secrets.
5. **Redactor stays in `lib/Redactor/`** — correct PlatformIO layer (low-level utility, no upward deps). KeyManager receives `Redactor&` via `begin()` for explicit wiring.
6. **`lib/Logger/` replaced by `lib/LogPipeline/`** — same slot in codebase, correct implementation. Contains vprintf hook, 2 KB static format buffer, serial output.
7. **2 KB static format buffer in .bss** — not stack, not heap. ESP-IDF serializes the vprintf callback via `esp_log_impl_lock()`, so single static buffer is safe. Covers HTTP headers with auth tokens.
8. **Ring buffer deferred** — serial monitor + log file is the current workflow. Ring buffer is RAM that's lost on crash anyway. Will add in follow-up PR when MCP log tools or web portal log viewer justify it.
9. **ILogSink interface deferred** — no consumers until ring buffer exists. Clean follow-up PR.
10. **`LogPipeline` name** (not `LogRouter`) — more descriptive of format→redact→output pipeline.

### OO Architecture: Redactor as Shared Sanitization Engine

- `Redactor` promoted from static utility class to Singleton with instance methods.
- `ISanitizer` interface: `virtual void sanitize(char* buf, size_t len) = 0`.
- AuditModule receives `ISanitizer*` via DI (testable with mock).
- LogPipeline uses `Redactor::instance()` directly (forced by C function pointer — no context parameter on `esp_log_set_vprintf`).
- Single secret registry shared by all consumers.

---

## Modules & Components

| Module | Files | WO(s) |
|--------|-------|-------|
| `lib/Redactor/` | `Redactor.h`, `Redactor.cpp` | WO1 |
| `include/interfaces/` | `ISanitizer.hpp` (new) | WO1 |
| `modules/AuditModule/` | `AuditModule.hpp`, `AuditModule.cpp` | WO1 |
| `lib/LogPipeline/` (new, replaces Logger) | `LogPipeline.h`, `LogPipeline.cpp` | WO2 |
| `modules/Application/` | `Application.hpp`, `Application.cpp` | WO2 |
| `include/platform/LogShim.h` | | WO2 |
| `lib/Logger/` (delete) | `Logger.hpp`, `Logger.cpp` | WO3 |
| `lib/DeviceCrypto/` | `DeviceCrypto.cpp` | WO3 |
| `modules/WifiManager/` | `WifiManager.cpp` | WO3 |
| `modules/KeyManager/` | `KeyManager.cpp` | WO3 |
| `modules/HomeAssistant/` | `HomeAssistantModule.cpp` | WO3 |
| `test/test_unit/test_redactor/` | `test_redactor.cpp` | WO3 |
| `test/test_unit/test_logger/` (delete) | `test_logger.cpp` | WO3 |
| `docs/design_specification.md` | §4.1, §11A, §12.1, §16.3, Decision Log | WO4 |
| `.agents/skills/framework-expert/` | SKILL.md, references/*.md | WO4 |

---

## Work Order Ledger

> All WOs are strictly serialized — each builds on the previous.

- **[x] [wo1-redactor-hardening](wo1-redactor-hardening/WORKORDER.md):** Harden Redactor with OO redesign — Singleton, fixed-capacity lock-free array, ISanitizer interface, AuditModule wiring. Fixes defects D1, D4, D5.
- **[x] [wo2-logpipeline](wo2-logpipeline/WORKORDER.md):** Create LogPipeline with `esp_log_set_vprintf()` hook, 2 KB static format buffer, re-entrancy guard, serial output. Install from `Application::begin()`. Implements D28.
- **[x] [wo3-logger-deletion](wo3-logger-deletion/WORKORDER.md):** Delete `lib/Logger/`, convert `LOG_SECURE` → `ESP_LOGD`, migrate `registerSecret` callers to Redactor, remove dead includes, enhance test_redactor, delete test_logger. Fixes D3.
- **[x] [wo4-docs](wo4-docs/WORKORDER.md):** Realign design spec §11A, update framework-expert skill, security checklist, thread-safety docs, audit-module guide. Fixes D6.

---

## Status Log

### Session 1 — 2026-09-01 (Investigation, Design & Approval)

**Context:** User raised issue #211 (Logger/LOG_SECURE redaction bypassed).
During investigation, user escalated scope to cover the entire logging subsystem
fragmentation, referencing issue #210 (migrate ESP_LOG to Logger — the opposite
approach, now closed as wrong direction).

**Design evolution (recorded decisions):**
1. Initial proposal: 7-phase plan with ring buffer, ILogSink, MCP tools.
2. User approved holistic approach, asked for PA decomposition with OO-expert analysis.
3. OO analysis: proposed ISanitizer interface + Singleton Redactor for shared use by LogPipeline and AuditModule.
4. User challenged: "should Redactor live in KeyManager?" — resolved that KeyManager is the primary *feeder* but doesn't *own* Redactor (PlatformIO layering: lib/ can't depend on modules/).
5. User challenged: ring buffer RAM cost vs. value — deferred ring buffer and ILogSink to follow-up PR.
6. User chose 2 KB static format buffer over stack buffer (covers HTTP auth headers).
7. User approved LogPipeline name over LogRouter.
8. Final decomposition: 4 WOs approved.

**Human approval:** User explicitly approved the 4-WO decomposition on 2026-09-01T20:35:07+01:00 with instruction to proceed with full implementation and stage PR for review.


### Session 1 — Execution (2026-09-01 20:35–21:18)

**WO1 (Redactor Hardening):** Completed. Worker executed on Pro model. Paranoid-worker failure mode encountered (refused PA approval messages) — PA committed directly per operating-model fallback. Commit `2f1db263`.

**WO2 (LogPipeline):** Completed. Worker executed on Flash model. Clean execution, committed autonomously. Commits `feb7d2cc`, `69d389bc`.

**WO3 (Logger Deletion):** Completed. Worker executed on Flash model. Clean execution, committed autonomously. 8 tests retired (863→855). Commit `0de8760a`.

**WO4 (Documentation):** Completed. Worker executed on Flash model. Clean execution, committed autonomously. Design spec, framework-expert skill, thread-safety, audit-module guide, architecture reference all updated. Commit `71532e18`.

**Final state:** 5 commits on `pr-211-logging/wo4-docs`, 31 files changed, +1243/−316, 855 tests passing, firmware builds clean. PR staged for review.

### Session 2 — Review, Remediation & Merge (2026-09-01 22:00 – 2026-09-02 23:08)

**Summary**: PR #212 received two rounds of Chief Architect review. First round (our commits): soak-tested, release-audited, remediated D1 findings. Second round found 6 blockers in our code (most significantly: insertion sort breaking append-only invariant, dumpPayload chunk-boundary straddle, extern stub breaking full native suite). A second agent (`9cbe6878`) fixed all blockers with reproduction-backed regression tests. 888 native tests pass. PR merged as `7ac38167`.

**Continuation project**: See [pr-212-remediation/PROJECT.md](../pr-212-remediation/PROJECT.md) for the full remediation session log.
