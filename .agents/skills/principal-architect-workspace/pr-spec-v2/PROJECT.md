# Design Specification v2.0 — Rebuild Against the Delivered Architecture

**Project:** `pr-spec-v2`  
**Base branch:** `main`  
**Status:** WO1/6 — in progress  
**Parent issue:** [#215](https://github.com/matthewmcneill/esp32-app-foundation/issues/215)  
**Audit source:** [design_spec_audit_report.md](conversation://8b733dfa-645a-478e-b6cc-e08c50620730) (Rev. B.1)

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

---

## Work Order Ledger

| WO | Title | Complexity | Status | Branch |
|----|-------|-----------|--------|--------|
| **WO1** | [Interface placement ADR + code alignment](wo1-interface-placement/WORKORDER.md) | `architect` | ✅ Complete | `spec-v2/wo1-interface-placement` |
| **WO2** | [Documentation inventory, authority map, spec triage](wo2-inventory-and-triage/WORKORDER.md) | `mechanical` | 🔄 In progress | `spec-v2/wo2-inventory-and-triage` |
| **WO3** | [v2.0 §3–§6 — architecture, kernel, modules, interfaces](wo3-v2-interfaces/WORKORDER.md) | `standard` | ⬜ Not started | `spec-v2/wo3-v2-interfaces` |
| **WO4** | [v2.0 §7–§11A — display, config, lifecycle, portal, comms, logging](wo4-v2-runtime/WORKORDER.md) | `standard` | ⬜ Not started | `spec-v2/wo4-v2-runtime` |
| **WO5** | [v2.0 §12–§17 — build, security, budgets, authoring, testing, decision log](wo5-v2-platform/WORKORDER.md) | `standard` | ⬜ Not started | `spec-v2/wo5-v2-platform` |
| **WO6** | [Doc-rot lint gate + fold-forward rules](wo6-rot-gate/WORKORDER.md) | `architect` | ⬜ Not started | `spec-v2/wo6-rot-gate` |

---

## Modules & Components

| Module/Area | WOs | Notes |
|---|---|---|
| `docs/design_specification.md` | WO2–WO6 | The primary target — v1.1 → v2.0 |
| `include/interfaces/` | WO1 | `ILVGLHardwareAdapter` relocation |
| `include/AppFoundation.h` | WO1 | Facade closure |
| `modules/MCPServer/` | WO1 | `_mcpReadOnly` initializer fix (#214) |
| `include/platform/LogShim.h` | WO1 | Doc comment + orphaned indent cleanup |
| `docs/developer/adr/` | WO1 | New `adr-interface-placement.md` |
| `docs/developer/coding-patterns.md` | WO1 | `std::function` exception rule |
| `CHANGELOG.md` | WO1 | New file — consumer notification |
| `docs/README.md` | WO2 | Authority map |
| `CONTRIBUTING.md` | WO2 | Folding rule |
| `docs/archive/` | WO2 | `lvgl-display-design.md` move |
| `scripts/doc_lint.py` | WO6 | New lint gate |

---

## Session Log

### Session 1 — 2026-09-03 — Project creation and WO1 kickoff

**Context:** Audit conversation 8b733dfa produced a 111-finding audit report (Rev. B.1) covering all 17 sections of the design spec. The chief architect reviewed and annotated the audit with [PA-1] through [PA-12]. The auditing agent reviewed and endorsed with four refinements, all accepted by the PA. User approved the full WO1–WO6 ledger.

**Actions taken:**
- Filed `_mcpReadOnly` P1 bug as [#214](https://github.com/matthewmcneill/esp32-app-foundation/issues/214)
- Created project workspace `pr-spec-v2/`
- Filed parent issue and WO child issues (pending)
- Spawning WO1 worker (pending)
