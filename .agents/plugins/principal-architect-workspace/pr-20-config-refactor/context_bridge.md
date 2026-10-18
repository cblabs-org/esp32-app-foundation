---
title: PR-20 — FrameworkConfig Refactor + appconfig.js Clarity (Principal Architect session)
distilled_at: 2026-06-09T00:00:00Z
original_plan_id: pr-20-config-refactor
artifacts:
  - context_artifacts/adr_frameworkconfig_split.md
  - context_artifacts/adr_harness_portability.md
  - context_artifacts/graveyard_pr20.md
  - context_artifacts/state_pr20_endstate.md
---

# PR-20 Context Bridge

## Executive Summary
PR-20 is **complete and verified**: `FrameworkConfig` was slimmed from 8 fields to the **3 compile-time OTA secrets**, with the 5 runtime defaults relocated into their owning modules' `resetConfig()` (WO2a: hostname/timezone/mcpReadOnly; WO2b: firmwareUpdatesEnabled→persisted `updatesEnabled`; otaQuietHour deleted as dead). Native **525/525** + `esp32dev` build green. The 3-WO stack is pushed to origin; **`main` is NOT pushed and no PRs are open** (user is holding the base decision).

## Next Steps (to resume)
1. **Base decision (blocking submission):** `main` is 6 commits ahead of `origin/main` (harness/skill ×3 + OTA fix ×3). Decide: `git push origin main` (fast-forward) vs carve the OTA fix into its own PR. Until then the stack can't submit cleanly.
2. **Open PRs** (when ready, plain `gh` — no gt/gh-stack): `wo1→main`, `wo2a→wo1`, `wo2b→wo2a`, Torvalds-standard descriptions; manual rebase cascade on merge.
3. Branches already on origin: `pr-20-config-refactor/{wo1-appconfig-clarity, wo2a-persisted-moves, wo2b-ota-new-persistence}` (tips `07e41f7`, `ca4801f`, `5b7f313`).
4. Unrelated but queued: device flash ~89.5% → `pr-10-firmware-optimization`.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_frameworkconfig_split.md` — the two-layer split, WO decomposition, and all 6 resolved design decisions (why each field went where).
- `context_artifacts/adr_harness_portability.md` — the cross-cutting principal-architect harness-adapter work (self-detection, model tiers, transcript lookup, artifact-write authorization, `.claude→.agents` wiring).
- `context_artifacts/graveyard_pr20.md` — dead-ends & traps: dead `otaQuietHour`, pre-existing native build breakage, subagent `.md`-write refusal, worker `pio`/python sandbox limits, git ref collision, stash drift, shared-checkout surprise.
- `context_artifacts/state_pr20_endstate.md` — final `FrameworkConfig`, OTA schema v4, exact branch/commit topology, verification numbers, tooling reality.
