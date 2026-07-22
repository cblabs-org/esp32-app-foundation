---
title: "PR-46 Open Issues Batch — Principal Architect Session"
distilled_at: "2026-07-22T20:09:00Z"
original_plan_id: "pr-46-open-issues-batch"
artifacts:
  - context_artifacts/adr_pa_session_decisions.md
  - context_artifacts/adr_psram_consumer_discovery.md
  - context_artifacts/graveyard_wo4_anti_injection.md
  - context_artifacts/state_pr_stack.md
---

## Executive Summary

Principal Architect session that investigated, planned, and executed fixes for all 4 open GitHub issues (#29, #31, #36, #37) on `esp32-app-foundation`. Work was decomposed into 4 serialized work orders executed by worker sub-agents, culminating in PR [#38](https://github.com/matthewmcneill/esp32-app-foundation/pull/38). During full-suite verification, discovered and fixed the same PSRAM misconfiguration (Issue #36) in both `sample-app` and `template-app`.

## Next Steps

1. **Merge PR #38** — LGTM review from `matthewmcneill` already on the PR. All tests green (423 native, 69 web, 3 S3 firmware builds).
2. **Close issues** — `Fixes #29, #31, #36, #37` keywords in the PR body will auto-close on merge.
3. **Pop the stash** — On `ha-retry-backoff/wo1-backoff-fix` branch there's a stash `PA: pre-existing skill edits on ha-retry-backoff branch` (`git stash pop` when returning to that branch).
4. **Power-monitor consumer** — Reviewer noted they will bump `framework.lock` off `05c9348`, rebuild, reflash, and confirm pills render on hardware after merge.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_pa_session_decisions.md` - Design decisions across all 4 issues (ordering, PSRAM guard scope, ArduinoJson remediation breadth)
- `context_artifacts/adr_psram_consumer_discovery.md` - How full-suite verification caught the Issue #36 bug in sample/template apps
- `context_artifacts/graveyard_wo4_anti_injection.md` - WO4 worker's Synthetic Approval Injection false positive and workaround
- `context_artifacts/state_pr_stack.md` - Complete branch stack, commit map, and test results
