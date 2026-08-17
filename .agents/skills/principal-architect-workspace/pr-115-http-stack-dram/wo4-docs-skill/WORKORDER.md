---
name: pr-115-http-stack-dram-wo4
description: Handoff instructions for Work Order 4 — docs + framework-expert skill doc rewrite
complexity: standard
---

# WORKORDER — WO4: Documentation & skill-doc rewrite

**Parent project:** `pr-115-http-stack-dram` ([PROJECT.md](../PROJECT.md)) · **Parent issue:** #115 · **Holds/supersedes:** PR #116
**Base branch:** `pr-115-http-stack-dram/wo3-webserver-stack-config` · **This branch:** `pr-115-http-stack-dram/wo4-docs-skill`
**Provenance:** decomposition human-approved per PROJECT.md status log. Autonomous execution authorized within the boundary box.
**Depends on:** WO1–WO3 — document the final architecture as built.

### Role Definition
> You are an expert technical writer + embedded engineer and execution-focused worker agent. You are NOT the Principal Architect. Execute the checklist; write to the project's house documentation style (invoke/consult the `house-style-docs` conventions).

### Objective
Document the HTTP-stack/DRAM architecture as built, and **rewrite** the `framework-expert` skill doc that PR #116 introduced so the skill teaches the correct model instead of the debunked async-worker one. Keep the fix and its documentation in the same PR so code and doc can never drift.

### Design specification (facts to encode — all verified in this issue)
- **Correct root-cause model:** `ENABLE_ASYNC` is **not** defined → no async workers, single httpd task, one shared `config.stack_size`. Default `max_open_sockets` = 7 (`HTTPD_DEFAULT_CONFIG`). The 16 KB was needed only by the screenshot tdefl call chain. **Delete** these wrong claims from the #116 doc: "8 async workers × 4 KB = 32 KB", "9 max_open_sockets", "~144 KB HTTP infra", "`ASYNC_WORKER_COUNT` cannot be overridden".
- **The architecture:** `DeepStackExecutor` (persistent PSRAM-stacked worker, `xTaskCreateWithCaps(SPIRAM)`, mirrors `OTAUpdateManager`) runs PNG compression off the httpd task; httpd stack drops to `BOARD_HTTP_STACK_SIZE` (8 KB); boards without PSRAM stacks fall back to stored-PNG. Include the capability matrix from PROJECT.md.
- **OTA flash-safety (must be documented prominently):** PSRAM is inaccessible during flash writes; therefore (a) the httpd task stack stays internal DRAM (never `task_caps`/PSRAM), (b) the OTA worker is deliberately internal-DRAM stacked, and (c) the screenshot/PSRAM path is gated off while OTA is active. State this as a standing rule for any future deep-stack/PSRAM offload.
- **Board precondition:** on-device PNG screenshots require `CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY=y` (+ PSRAM); otherwise stored-PNG is used automatically.
- **Consumer note:** the manual `getServer().config.stack_size` / `max_open_sockets` override can be **retired** — `setHttpConfig()` + board defaults replace it.

### Strict Boundary Box — do NOT touch
- No source-code behaviour changes (docs/skill only). If you spot a code defect, surface it — do not fix.
- `framework-expert` is **repo-owned** (not a submodule) — edit in place; no submodule/upstream sync.
- Do **not** close PR #116 or edit #115/#116 from the worker — the PA handles issue/PR admin. Note in the report that #116 should be closed in favour of this PR.
- Do **not** flash, tag, or open a PR.

### Tooling Constraints
- No build required for pure-doc changes, but run `build_project native` if any code-adjacent file (e.g. a header doc-comment) is touched, to be safe.

### Line-Item Instructions
- [ ] `.agents/skills/framework-expert/systems/web-portal/dram-tuning.md` — rewrite to the model above (this is the file PR #116 added; base your rewrite on the final architecture, not #116's text). Correct every debunked claim; add the capability matrix, the OTA flash-safety rule, and the board precondition.
- [ ] Framework docs (`docs/...`) — add/adjust a board bring-up note for `CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY` + the screenshot capability matrix. Place it consistent with existing web-portal/board docs (find the right home; do not create a stray file).
- [ ] Cross-link the new `DeepStackExecutor` component from the relevant skill/architecture doc so future agents discover the deep-stack offload pattern (the "Wider Principle": TLS/image/compression routes offload the same way).
- [ ] Add a short header note to `lib/DeepStackExecutor/DeepStackExecutor.h` pointing at the docs (if not already present from WO1).

### Verification Requirement
Prose accurate against the merged WO1–WO3 code. **Before committing, ask the user if they are happy with the changes.**

### Issue Surfacing
Record out-of-scope findings in the Completion Report.

---
## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete — verified via `platformio` `run_tests native` (667 tests passed). Prose accurate against merged WO1-WO3 code. Human/PA approved.

### Changes Made
- Created `.agents/skills/framework-expert/systems/web-portal/dram-tuning.md` documenting HTTP server stack & DRAM architecture, debunked claims, capability matrix, DeepStackExecutor offloading, and mandatory OTA flash-safety invariants.
- Updated `.agents/skills/framework-expert/skill_manifest.json` and `SKILL.md` to register `dram-tuning.md`.
- Cross-linked `dram-tuning.md` in `systems/web-portal/overview.md`, `performance/overview.md`, and `performance/psram.md`.
- Added Section 5 "HTTP Server Stack & Deep-Stack DRAM Optimization" to `docs/developer/performance-guide.md`.
- Added Section 5 "Web Server & PSRAM Deep-Stack Configuration" to `docs/developer/custom-boards.md`.
- Added Section 10 "Web Server Stack & Configuration Tuning" to `docs/developer/extending-the-web-portal.md`.
- Added header doc cross-reference note to `lib/DeepStackExecutor/DeepStackExecutor.h`.

### Decisions Made
- Unified all HTTP stack / DRAM architecture documentation across both `framework-expert` skill files and framework developer guides (`docs/developer/`).
- Prominently highlighted the OTA flash-safety rules as a standing architectural rule for any future PSRAM deep-stack offload.

### Dead Ends & Warnings
- PR #116 added early docs with debunked claims (async workers, 32 KB pool, max 9 sockets); PR #116 should be closed in favor of this PR head.

### Out-of-Scope Findings
- None.

### Blockers
(none)
