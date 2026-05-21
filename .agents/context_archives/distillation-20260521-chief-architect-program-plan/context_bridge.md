---
title: "ESP32 App Foundation — Chief Architect Program Plan"
distilled_at: "2026-05-21T08:56:00+01:00"
original_plan_id: "f4e779d9-4aa1-46e2-8115-e1e8edc4fa42"
artifacts:
  - context_artifacts/adr_program_decisions.md
  - context_artifacts/graveyard_rejected_approaches.md
program_plan: .agents/skills/principal-architect-workspace/esp32-app-foundation/PROGRAM-PLAN.md
implementation_plan: "(session artifact — see parent conversation)"
pa01_conversation_id: "30aaf023-db75-4615-bf03-5b7bf9e9bacc"
---

# Context Bridge — ESP32 App Foundation Chief Architect

## Executive Summary

A 7-project implementation program has been fully planned, reviewed, and approved to build the `esp32-app-foundation` framework from scratch. PA-01 is **already running** (conversation `30aaf023`), bootstrapping the git repo and implementing PR-01 (Kernel Interfaces & Application Hub). This Chief Architect session is idle, waiting for PA-01 to report back before issuing PA-02.

## Current State

- **PA-01 (conversation `30aaf023`):** RUNNING — bootstrapping git repo + implementing PR-01
- **PA-02 through PA-07:** NOT STARTED — each gates on the previous PR merging to `main`
- **Repository:** `esp32-app-foundation` contains only `docs/` and `.agents/` — no git repo yet (PA-01 creates it)
- **Skills installed:** pio-manager, embedded-systems, house-style-docs (via skill-manager, hash `3f771f09`), oo-expert, principal-architect, distillery

## Next Steps (Chief Architect resuming autonomously)

1. **Wait** for PA-01 (`30aaf023`) to report back with PR-01 merged
2. **Confirm** `git log` shows initial commit + WO commits on `pr-01-kernel-interfaces`
3. **Issue PA-02 bridge prompt** (see PROGRAM-PLAN.md §PR-02) — branch `pr-02-kernel-services` off `main`
4. **Wait** for PA-02 → issue PA-03 → wait → issue PA-04 + PA-05 simultaneously → wait for both → issue PA-06 → wait → issue PA-07
5. **Final gate:** PR-07 merged — E2E boot test passes — program complete

## Key Rules for Resuming Chief Architect

- **One PA at a time** (except PR-04 + PR-05 which are parallel after PR-03)
- **Every PA bridge prompt MUST include the 5-skill boot sequence** verbatim (pio-manager, embedded-systems, house-style-docs, oo-expert, mcp-agent-reference.md)
- **Every WORKORDER issued by a PA MUST include the 5-skill boot sequence** verbatim at the top
- **Repo bootstrap is PA-01 WO1 step 1** — `git init` → `.gitignore` → initial commit
- **Branch naming:** `pr-NN-name` (feature), `pr-NN-name/woY-topic` (WO)
- **WO→Feature:** regular merge. **Feature→main:** squash permitted.
- **Testing gates:** native unit tests + hardware integration tests must pass before WO closes
- **E2E test** (`test/test_e2e/`) is the gate before PR-06 merges

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_program_decisions.md` — All 9 confirmed architectural decisions with rationale (monorepo, vectors, git strategy, skill boot sequence, etc.)
- `context_artifacts/graveyard_rejected_approaches.md` — 5 explicitly rejected approaches (lib_deps registry, separate modules repo, optional/ split, separate template repo, ADR-05 fixed arrays)
