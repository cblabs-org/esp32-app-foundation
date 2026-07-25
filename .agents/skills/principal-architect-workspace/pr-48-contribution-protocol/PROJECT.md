# PR-48 — Contribution Protocol for framework-expert Skill

**Issue:** [#43 — No contribution protocol](https://github.com/matthewmcneill/esp32-app-foundation/issues/43)
**Base branch:** `main`
**Status:** 🟢 APPROVED — executing WO1

## Objective

Close the documentation gap identified in Issue #43: the `framework-expert` skill
covers **reporting** defects (`issue-management.md`) but provides no protocol for
**contributing** fixes, **proposing** enhancements, or **authoring** ADRs. A downstream
consumer wanting to upstream a change has no documented route.

## Scope

- **In scope:** Skill documentation only — `.agents/skills/framework-expert/` files,
  root `CONTRIBUTING.md`, and `skill_manifest.json` refresh.
- **Out of scope:** No C++ code changes. No firmware. No partition/build changes.
  No ADR register creation (filed as separate concern). No `WORKORDER.md` or PA
  workspace structural changes.

## Work Order Ledger

- **[ ] [wo1-protocol-doc](wo1-protocol-doc/WORKORDER.md):** Create `references/contribution-protocol.md` (the core deliverable) + add `P4-enhancement` severity tier to `issue-management.md` + cross-link the two documents.
- **[ ] [wo2-discovery-wiring](wo2-discovery-wiring/WORKORDER.md):** Wire discovery into SKILL.md (routing table + trigger vocabulary) + repoint root `CONTRIBUTING.md` + update `KEEPING-CURRENT.md` ADR guidance + refresh `skill_manifest.json`.

## Concurrency Analysis

WO1 and WO2 touch different primary files. WO2 depends on WO1 (it links to the
file WO1 creates). **Serialise: WO1 → WO2.**

## Status Log

| Date | Event |
|------|-------|
| 2026-07-25 | Project created. Decomposition drafted, awaiting user approval. |
| 2026-07-25 | **User approved decomposition.** Proceeding with WO1 execution. |
