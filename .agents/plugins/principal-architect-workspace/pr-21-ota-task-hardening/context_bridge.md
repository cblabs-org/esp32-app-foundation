---
title: PR-21 — OTA Task Creation Hardening — Principal Architect Distillation
distilled_at: "2026-06-14T10:59:00Z"
original_plan_id: pr-21-ota-task-hardening
pa_session: 8c58afa2-8ca2-4a85-b127-3fb94f785271
merge_commit: 5f15490
status: MERGED TO MAIN
artifacts:
  - context_artifacts/adr_pa_design_decisions.md
  - context_artifacts/graveyard_pa_session.md
  - context_artifacts/state_branch_topology.md
---

## Executive Summary

PR-21 hardens the OTA FreeRTOS task creation path against degraded-DRAM conditions that
caused a self-heal deadlock during the PR-19 live test. Two work orders were executed
serially: WO1 replaced `xTaskCreate` with `xTaskCreateWithCaps(MALLOC_CAP_SPIRAM)` and
added a DRAM pre-flight guard in `OTAUpdateManager.cpp`; WO2 added a non-blocking
advisory warning banner to the portal OTA tab. Both are merged and pushed to `main`
at commit `5f15490`.

---

## Final State

| Item | Value |
|------|-------|
| Feature branch | `pr-21-ota-task-hardening` |
| Merge commit | `5f15490` on `main` |
| WO1 commit | `175dfc8` — firmware heap guard + PSRAM stack |
| WO2 commit | `4d8aff6` — portal low-memory warning banner |
| Bug-fix commits (other agent) | `dff3b22`, `a0c91dc` — crypto PKCS7 + WiFi modal |
| Push | ✅ `origin/main` — `df510d3..5f15490` |

---

## Next Steps

PR-21 is fully closed. No further action required on this project.

Possible follow-on work (not committed to):
1. **Live portal verification** — connect to device AP at `192.168.4.1` and confirm the
   low-memory banner hides correctly at normal heap levels (8.6 MB PSRAM free >> 32 KB).
2. **PR-22** — `pr-22-portal-ui-consistency` PROJECT.md was pre-staged by the WO2 worker
   (included in the merge). Review and decide whether to activate.
3. **PSRAM-absent build test** — validate `xTaskCreateWithCaps` DRAM fallback on a
   board without PSRAM (e.g. esp32dev environment).

---

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_pa_design_decisions.md` — The three key ADRs: API injection strategy, PSRAM API choice, constant extraction rationale
- `context_artifacts/graveyard_pa_session.md` — Branch namespace collision, portal asset gitignore trap, WO2 worker permission-wall commit failure, AP network curl unreachable
- `context_artifacts/state_branch_topology.md` — Final branch/commit map for the full PR-21 stack

### WO-level distillations (already written by workers):
- `wo1-firmware-heap-guard/context_bridge.md` — WO1 end-state, code snippets, verification table
- `wo2-portal-memory-warning/context_bridge.md` — WO2 end-state, advisory design notes
