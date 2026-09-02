# PR-212 Remediation: Logging Subsystem Hardening (V2)

> **PR:** [#212](https://github.com/matthewmcneill/esp32-app-foundation/pull/212)
> **Issue:** [#211](https://github.com/matthewmcneill/esp32-app-foundation/issues/211)
> **Base branch:** `main`
> **Merge commit:** `7ac38167`

---

## Goal
Address critical defects found in the Chief Architect review of PR #212 (allocation in critical section, cross-task race condition in log hook, chunk-boundary secret straddle) and implement the hardened verbose HTTP payload dump architecture.

## Work Orders

| Phase | Work Order | Folder | Status |
|---|---|---|---|
| 1 | WO1 - Redactor Core Hardening | `wo1-redactor-core` | ✅ Completed |
| 1 | WO2 - LogPipeline Correctness | `wo2-logpipeline-correctness` | ✅ Completed |
| 2 | WO3 - Test Infrastructure | `wo3-test-infrastructure` | ✅ Completed |
| 3 | WO4 - dumpPayload API & Structural Scrubber | `wo4-dump-payload` | ✅ Completed |
| 4 | WO5 - Callers and Docs | `wo5-callers-docs` | ✅ Completed |

## Modules & Components

| Module | Files | WO(s) |
|--------|-------|-------|
| `lib/Redactor/` | Redactor.h/.cpp, StructuralScrubber.h/.cpp | WO1, WO4 |
| `lib/LogPipeline/` | LogPipeline.h/.cpp, CompositeSanitizer.h | WO2, WO4 |
| `include/interfaces/` | ISanitizer.hpp | WO1 |
| `modules/Application/` | Application.cpp | WO2, WO4 |
| `modules/AuditModule/` | AuditModule.cpp | WO5 |
| `modules/KeyManager/` | KeyManager.cpp | WO5 |
| `modules/WifiManager/` | WifiManager.cpp | WO5 |
| `test/test_unit/test_redactor/` | test_redactor.cpp | WO1, WO3 |
| `test/test_unit/test_logpipeline/` | test_logpipeline.cpp | WO3 |
| `test/test_unit/test_structural_scrubber/` | test_structural_scrubber.cpp | WO4 |
| `test/stubs/platform/` | esp_log.h | WO3 |
| `docs/design/` | logging.md | WO5 |
| `docs/` | design_specification.md §11A | WO5 |
| `platformio.ini` | native env CONFIG_LOG_MAXIMUM_LEVEL=5 | WO3 |

---

## Session Log

### 2026-09-01T20:35 — WO1-WO5 Executed (Agent 1)

**Summary**: All 5 WOs executed serially by the main agent after subagent spawn failures (RESOURCE_EXHAUSTED 429). Commits `12defa33`, `39bba7a4`.

**Cross-cutting decisions**:
- ESP-IDF 5.x (not 6.x) confirmed: pioarduino Arduino-ESP32 v3.3.11 is tied to IDF 5.5.5. xPortInIsrContext() used instead of xPortInIsr().
- Dual-framework (Arduino + ESP-IDF) retained: Arduino ecosystem drivers are the value prop for downstream consumers.
- Buffer strategy evolved: ring buffer dropped, 256B static + conditional heap fallback (gated on CONFIG_LOG_MAXIMUM_LEVEL >= 4).

**Dead ends (project-wide)**:
- xPortInIsr() does not exist in IDF 5.x — caused compilation failure, fixed to xPortInIsrContext().
- Subagent spawning hit API rate limits (5 spawn attempts). All WOs executed directly by the main agent.
- test_logpipeline.cpp #define CONFIG_LOG_MAXIMUM_LEVEL 5 only affects the test TU, not the library TU — heap path and dumpPayload were never compiled or tested. Fixed later by Agent 2.

### 2026-09-01T22:00 — Soak Test PASS (Agent 1)

**Summary**: Tagged v0.0.12-dev.56, flashed ESP32-S3 Nano, 28-min soak. Zero crashes, heap stable (124B oscillation), dram_min dropped 1,328B from WiFi transients then flat.

### 2026-09-01T22:36 — Release Audit (Agent 1)

**Summary**: Mechanical checks (25 checks, 10 dimensions). Remediated 3 missing headers, 1 Doxygen param, removed stray patch.py. Commit `39bba7a4`, tagged v0.0.12-dev.57. CONDITIONAL PASS.

### 2026-09-02T06:26 — Chief Architect Second Review

**Summary**: 6 blockers, 6 highs, 9 mediums. Key blockers: extern stub broke full native suite (B1), dumpPayload chunk-boundary straddle (B2), realloc outside mutex (B3), AuditModule missing isTruncated (B5), insertion sort broke append-only invariant (B6), heap path compiled in zero configs (H1).

### 2026-09-02T21:50 — Fixes by Agent 2 (9cbe6878)

**Summary**: All blockers reproduced and fixed with regression tests. 888 native tests pass. Append-only storage restored, sliding-window dumpPayload, heap under lock, fail-closed defaults, bounded hand-rolled search, resetForTest(), inline accessor for stub, native CONFIG_LOG_MAXIMUM_LEVEL=5.

### 2026-09-02T23:08 — PR Merged & Project Wrap-Up

**Architectural outcome**: Single esp_log_set_vprintf() sink with lock-free reads (append-only slots, atomic count, stack-local scan ordering), mutex-protected writes, structural scrubbing, sliding-window dumpPayload. 888 tests, 49 files, +2,512/-313 lines.

**Lessons for future projects**:
1. Never overwrite a PR body — always add a comment to preserve the narrative.
2. Run the full native test suite, not just affected suites. B1 would have been caught immediately.
3. A #define in a test file does not affect the library TU. Use platformio.ini build_flags.
4. "Descending-length scan order" != sorted storage. The requirement was ordering at scan time, not storage time.
5. The agent that reviews is more valuable than the agent that writes.

**Unresolved items**:
- Precondition for redaction saturation (secretCount()/MAX_SECRETS) — file as follow-up issue.
- vsnprintf truncation return under CONFIG_NEWLIB_NANO_FORMAT=y — confirmed working but worth targeted on-target verification before beta.
- ESP_EARLY_LOG / ESP_DRAM_LOG bypass the hook — documented as known limitation.
