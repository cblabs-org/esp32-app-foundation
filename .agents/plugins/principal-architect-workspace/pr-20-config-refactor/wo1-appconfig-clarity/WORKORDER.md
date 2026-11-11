---
name: pr-20-config-refactor-wo1
description: Handoff instructions for Work Order 1 — clarify appconfig.js scope, resolve the portal-tab-wip stash, and map portal exposure of the 5 migrating FrameworkConfig fields. Investigation only.
complexity: standard
---

# pr-20-config-refactor / WO1 — Clarify `appconfig.js` Scope (Investigation Only)

**Branch:** `pr-20-config-refactor/wo1-appconfig-clarity` (cut by the PA, off `main` — stacked model, no bare parent branch; WO2a will base off this branch)

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are **NOT** the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Determine exactly what `web/portal/appconfig.js` surfaces in the portal, resolve the fate of the `portal-tab-wip` stash, and produce a written findings document — including a per-field map of whether the 5 migrating `FrameworkConfig` fields are exposed in the portal — so WO2's scope is de-risked. **This is pure investigation and documentation. No production source changes.**

## Strict Boundary Box

- **DO NOT** modify any C++ source, including `FrameworkConfig.hpp` — the refactor is WO2.
- **DO NOT** modify `web/portal/appconfig.js`, `app.js`, or `index.html`.
- **DO NOT** apply, pop, or merge any stash into the working tree. You may **inspect** stashes read-only (`git stash show -p`). Dropping a stash is allowed **only** in the final step and **only** after explicit user approval.
- **DO NOT** branch, merge, or rebase. The PA owns branch state.
- Your **only** write is the findings document at `.agents/plugins/principal-architect-workspace/pr-20-config-refactor/wo1-appconfig-clarity/FINDINGS.md` (and, on approval, the documented stash drop).

## Line-Item Instructions

- [ ] **Read prior context:** `.agents/plugins/principal-architect-workspace/adhoc-app-tab-rules-bug/context_bridge.md` (and its `context_artifacts/` only if needed) — this session partially built the App tab and left the stash unresolved.
- [ ] **Inspect `appconfig.js`:** read `web/portal/appconfig.js` and determine every configuration key it reads/writes and which device endpoint(s) it talks to.
- [ ] **Trace tab wiring:** read `web/portal/app.js` (tab registration) and `web/portal/index.html` (tab loading) to confirm whether the appconfig tab is actually registered/active or still dormant.
- [ ] **Inspect the stash (read-only):** the target is **`stash@{1}`** = `580f2f5` "portal-tab-wip" (NOTE: the original work order said `stash@{2}` — the index has drifted; verify by hash). Run `git stash show -p stash@{1}` and confirm its hash with `git stash list --format="%gd %H %gs"`.
- [ ] **Determine redundancy:** compare the stash contents against what is already on `main` (e.g. `git log --oneline -- web/portal/appconfig.js`, `git diff main -- web/portal/`) to decide whether `stash@{1}` is **already incorporated** (redundant) or **still needed**.
- [ ] **Per-field portal-exposure map (critical for WO2):** for each migrating field, record whether the portal currently exposes/edits it, and via which tab/file:
  - `hostname`, `timezone`, `firmwareUpdatesEnabled`, `otaQuietHour`, `mcpReadOnly`.
  - Check `appconfig.js` and any OTA/MCP/WiFi/Time portal tabs. This tells WO2 whether removing a C++ constant requires coordinating a portal change.
- [ ] **Confirm test baseline:** report whether the native unit-test suite currently builds/passes (per the `pio-manager` skill / project test command). Do **not** fix failures — just report the baseline state.
- [ ] **Write `FINDINGS.md`** in this WO directory containing:
  1. What `appconfig.js` does (keys, endpoints, active vs dormant).
  2. Stash verdict: `stash@{1}`/`580f2f5` redundant or still-needed, with evidence.
  3. The per-field portal-exposure map (table).
  4. Whether `appconfig.js` **duplicates or complements** the planned module-level config migration.
  5. Native test-baseline status.
- [ ] **Stash disposition:** if the stash is redundant, present the evidence to the user and ask for explicit approval to `git stash drop stash@{1}`. Only drop it after the user says yes. If still needed, document what must happen to it and leave it in place.

## Verification Requirement

There is no build to run (investigation only). Verify instead that:
- `FINDINGS.md` exists and covers all five required sections.
- No tracked source files were modified (`git status --short` shows only `FINDINGS.md` as a new/changed file under this WO directory).

**CRITICAL:** Before distilling or committing, **STOP and ask the user if they are happy** with the findings and the stash disposition. Only *after* explicit approval should you invoke the `@distillery` skill to package the state into this WO directory, then append a status line to this `WORKORDER.md` and terminate.
