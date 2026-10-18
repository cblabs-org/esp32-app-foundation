# Design Specification v2.0 — Rebuild Against the Delivered Architecture

**Project:** `pr-spec-v2`  
**Base branch:** `main`  
**Status:** ✅ COMPLETE — PR [#224](https://github.com/matthewmcneill/esp32-app-foundation/pull/224) staged  
**Parent issue:** [#215](https://github.com/matthewmcneill/esp32-app-foundation/issues/215)  
**Audit source:** [design_spec_audit_report.md](conversation://8b733dfa-645a-478e-b6cc-e08c50620730) (Rev. B.1)  
**PR head branch:** `spec-v2/wo6-rot-gate` (tip of 6-WO stack)

---

## Approval Record

**User approved the WO decomposition on 2026-09-03T07:28:11+01:00.**

> "Then please take on the role of /principal-architect in charge of implementation and oversee implementation of the full project to produce the updated design spec, and stage it as a PR for review when you have completed."

This approval covers the full WO1–WO6 ledger as specified in Rev. B.1. The `_mcpReadOnly` P1 bug was filed separately as [#214](https://github.com/matthewmcneill/esp32-app-foundation/issues/214) per explicit user instruction.

---

## Decisions

| # | Decision | Ruling |
|---|---|---|
| **D-A** | Spec's future | **Produce v2.0.** Not a demotion. |
| **D-B** | `IDisplayBoard` placement | It belongs to U8G2. A written heuristic (ADR) is needed. |
| **D-C** | Tracking issues | One parent + one per WO. |
| **D-D** | WO stacking | Single linear chain. |
| **D-E** | Authority orders | Two-tier: Factual (code wins) + Architectural (spec wins). |
| **D-F** | `www/` LittleFS web assets | Phantom — never implemented. All assets are PROGMEM. Removed from spec. |
| **D-G** | Design doc disposition | All three shipped design docs (config-gate, microkernel, hardware-config) archived to `docs/archive/`. Rationale extract `docs/design/config-gate.md` deleted as redundant with archive. |

---

## Work Order Ledger

| WO | Title | Complexity | Status | Branch | Commit |
|----|-------|-----------|--------|--------|--------|
| **WO1** | [Interface placement ADR + code alignment](wo1-interface-placement/WORKORDER.md) | `architect` | ✅ Complete | `spec-v2/wo1-interface-placement` | `040da100` |
| **WO2** | [Documentation inventory, authority map, spec triage](wo2-inventory-and-triage/WORKORDER.md) | `mechanical` | ✅ Complete | `spec-v2/wo2-inventory-and-triage` | `e23e1655` |
| **WO3** | [v2.0 §3–§6 — architecture, kernel, modules, interfaces](wo3-v2-interfaces/WORKORDER.md) | `standard` | ✅ Complete | `spec-v2/wo3-v2-interfaces` | `7ef4c19b` |
| **WO4** | [v2.0 §7–§11A — display, config, lifecycle, portal, comms, logging](wo4-v2-runtime/WORKORDER.md) | `standard` | ✅ Complete | `spec-v2/wo4-v2-runtime` | `0d4e16d2` |
| **WO5** | [v2.0 §12–§17 — build, security, budgets, authoring, testing, decision log](wo5-v2-platform/WORKORDER.md) | `standard` | ✅ Complete | `spec-v2/wo5-v2-platform` | `d169ad3c` |
| **WO6** | [Doc-rot lint gate + fold-forward rules](wo6-rot-gate/WORKORDER.md) | `architect` | ✅ Complete | `spec-v2/wo6-rot-gate` | `8b170524` |

Post-WO follow-up commits: `a985fe2b` (v2.0 promotion), `10fdc8ad` (design doc archives), `a68a3bd3` (§1 rewrite + NATIVE_BUILD sweep), `1edbbf6e` (www/ fix), `d34f994d` (agent skill lint fixes), `f4ad5a64` (config-gate.md delete)

---

## Modules & Components

| Module/Area | WOs | Notes |
|---|---|---|
| `docs/design_specification.md` | WO2–WO6 + follow-up | The primary target — v1.1 → v2.0 (all 18 sections) |
| `include/interfaces/` | WO1 | `ILVGLHardwareAdapter` relocation |
| `include/AppFoundation.h` | WO1 | Facade closure (ISanitizer, IBootReporter, IDisplayModule) |
| `modules/MCPServer/` | WO1 | `_mcpReadOnly` initializer fix (#214) |
| `include/platform/LogShim.h` | WO1 | Doc comment + orphaned indent cleanup |
| `docs/developer/adr/` | WO1 | New `adr-interface-placement.md` |
| `docs/developer/coding-patterns.md` | WO1 | `std::function` exception rule |
| `CHANGELOG.md` | WO1 | New file — consumer notification |
| `docs/README.md` | WO2 + follow-up | Authority map, disposition table, archive index |
| `CONTRIBUTING.md` | WO2 | Documentation governance, folding rule |
| `docs/archive/` | WO2 + follow-up | lvgl-display, config-gate, microkernel, hardware-config |
| `scripts/doc_lint.py` | WO6 + follow-up | Lint gate + historical context opt-out |
| `docs/config-gate-design.md` | WO4 + follow-up | FOLDED banner added, then archived |
| `docs/design/logging.md` | WO4 | Deleted (folded into §11A) |
| `docs/design/config-gate.md` | WO4 + follow-up | Created as rationale extract, then deleted as redundant |
| `docs/developer/*.md` (10 files) | follow-up | NATIVE_BUILD → ESP_PLATFORM sweep |
| `.agents/skills/framework-expert/` (3 files) | follow-up | NATIVE_BUILD → ESP_PLATFORM sweep |
| `test/test_unit/test_mcpServer/` | WO1 | New regression test for _mcpReadOnly default |

---

## Session Log

### 2026-09-03T07:30:00+01:00 — WO1 Complete: Interface placement ADR + code alignment
**Summary**: 18-file change delivering the ADR, ILVGLHardwareAdapter relocation, AppFoundation.h facade closure, _mcpReadOnly security fix, CHANGELOG.md, migration guide, std::function policy doc, and regression test. All hardware builds pass (esp32dev, esp32s3nano, crowpanel-128-rotary). Native: 884/888 (4 pre-existing test_lvgl_stubs failures).
**Cross-cutting decisions**: Three-rule interface placement heuristic established as ADR. Consumer notification artifacts (CHANGELOG.md, migration guide) set the precedent for all future breaking changes.
**Surfaced findings**: Pre-existing test_lvgl_stubs failures are an LDF resolution issue, not spec-v2 related.

### 2026-09-03T08:05:00+01:00 — WO2 Complete: Documentation inventory, authority map, spec triage
**Summary**: docs/README.md rewritten as authority map (four trees, two conflict-resolution orders, folding rule, disposition table). Spec front matter updated with 18-row disposition table. lvgl-display-design.md archived. CONTRIBUTING.md governance section added.
**Cross-cutting decisions**: Two-tier conflict resolution (Factual: code wins; Architectural: spec wins) is now the governing principle for all spec maintenance.

### 2026-09-03T08:14:00+01:00 — WO3 Complete: v2.0 §3–§6 (critical band)
**Summary**: The largest and most critical WO — complete rewrite of §3 (architecture diagram), §4 (18-component kernel table), §5 (18-module catalog), §6 (14 kernel interfaces + 3 module-local + 3 event payloads). All interface signatures verified against actual headers. Zero stale identifiers confirmed by grep.
**Cross-cutting decisions**: §6 established the definitive interface reference — all 14 kernel interfaces documented with actual method signatures from headers. This is now the single source of truth for interface contracts.

### 2026-09-03T08:22:00+01:00 — WO4 Complete: v2.0 §7–§11A + config-gate fold
**Summary**: Second-largest WO — §7 (dual-display), §8 (ConfigManager-as-Gateway), §9 (3-state lifecycle + PreconditionEngine), §10 (PsychicHttp portal), §11 (EventBus esp_event), §11A (logging updates). Three fold operations: config-gate-design.md bannered, rationale extracted, logging.md deleted.
**Cross-cutting decisions**: ConfigManager-as-Gateway established as the central design pattern — modules never touch LittleFS directly.

### 2026-09-03T08:31:00+01:00 — WO5 Complete: v2.0 §12–§17 + D40–D54
**Summary**: §12 (build system, directory tree), §13 (AuthManager security, split-repo OTA), §14 (measurement-not-numbers per PA-9/PA-10), §15 (module authoring), §16 (stub-header testing), §17 (15 new decision log entries D40–D54, all ADRs cross-linked).
**Cross-cutting decisions**: §14 measurement-not-numbers policy prevents future doc-rot — partition sizes, binary footprints, and test counts point to measurement commands rather than stale hardcoded values.

### 2026-09-03T08:36:00+01:00 — WO6 Complete: Doc-rot lint gate
**Summary**: scripts/doc_lint.py created — zero-dependency Python 3 linter with 16 banned identifiers, STATUS banner checks, fold-forward verification. 0.08s over 36 files. Documented in README.md and CONTRIBUTING.md.
**Cross-cutting decisions**: Linter is local-only until CI integration lands (#144). Historical context lines (containing 'legacy', 'replaced', etc.) are auto-skipped to prevent false positives on legitimate removal documentation.

### 2026-09-03T21:45:00+01:00 — Post-WO follow-up: §1 rewrite, NATIVE_BUILD sweep, design doc archives
**Summary**: User-driven follow-up session addressing remaining audit items. §1 Purpose & Vision rewritten to v2.0 (all 18 sections now complete). NATIVE_BUILD → ESP_PLATFORM sweep across 10 developer guides + 3 agent skill files (35+ replacements). Three shipped design docs (config-gate, microkernel, hardware-config) archived with FOLDED banners. Phantom `www/` LittleFS web assets removed from spec (D8, D9 corrected). `docs/design/config-gate.md` rationale extract deleted as redundant with archive.
**Dead ends**: The linter initially flagged historical context lines as false positives (e.g., "the legacy ESPAsyncWebServer has been completely replaced"). Solved by adding a HISTORICAL_CONTEXT_REGEX opt-out pattern rather than rewriting legitimate historical prose.

### 2026-09-07T21:40:00+01:00 — Project Wrap-Up
**Architectural outcome**: The design specification is now a v2.0 document faithfully reflecting the delivered ESP32 App Foundation architecture. All 18 sections rebuilt. 15 new architectural decisions documented (D40–D54). Three shipped design documents consolidated and archived. A doc-rot lint gate prevents recurrence. The specification, developer guides, and agent skill files are internally consistent — zero lint findings.

**Lessons for future projects**:
1. **Design docs rot silently.** The spec had diverged from the codebase across 16 of 18 sections without anyone noticing. A lint gate should be standard for any project with a living specification.
2. **NATIVE_BUILD → ESP_PLATFORM was pervasive.** A single guard rename touched 45+ documentation sites across 4 document trees. Future guard renames need a doc sweep in the same PR as the code change.
3. **Design doc lifecycle matters.** The config-gate, microkernel, and hardware-config design docs all shipped but retained "WIP" or "FINALIZED — ready for implementation" banners. The FOLDED/ARCHIVED/IMPLEMENTED STATUS banner convention (enforced by the linter) prevents this.
4. **Rationale extracts are redundant when originals are archived.** Creating `docs/design/config-gate.md` as a curated rationale subset was unnecessary — the full archived document already preserves all rationale. Don't duplicate.

**Unresolved items**:
- [#214](https://github.com/matthewmcneill/esp32-app-foundation/issues/214) — `_mcpReadOnly` P1 broader remediation (code fix landed in WO1, separate remediation planned by user)
- [#223](https://github.com/matthewmcneill/esp32-app-foundation/issues/223) — Compile-check fenced C++ code samples in spec against real headers
- [#222](https://github.com/matthewmcneill/esp32-app-foundation/issues/222) — Can be closed on merge (resolved by WO1)
- Consumer notification for DraytonWiserController, departures-board-v3, PowerMonitor — PR comment posted, user handling separately
