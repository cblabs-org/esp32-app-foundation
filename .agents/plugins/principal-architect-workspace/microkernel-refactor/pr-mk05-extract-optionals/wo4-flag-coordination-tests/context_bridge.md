---
title: WO4 Flag Coordination and App Templates
distilled_at: 2026-07-29T12:00:00Z
original_plan_id: microkernel-refactor-pr-mk05
artifacts:
  - context_artifacts/adr_wo4_flag_coordination_tests.md
  - context_artifacts/graveyard_wo4_flag_coordination.md
---

### Executive Summary
Work Order 4 completed the microkernel optional module extraction (`pr-mk05`) by updating `template-app` and `sample-app`, ensuring clean compilation across the flag matrix (`WITH_MCP`, `WITH_OTA`, `WITH_TIME`), verifying 449 native unit tests, and recording flash/RAM size savings (>105 KB flash saved in `template-app` with optionals off).

### Next Steps
1. Merge branch `pr-mk05-extract-optionals/wo4-flag-coordination-tests` into base branch.
2. Proceed to PR-MK06 (`pr-mk06-periodic-helper`) or PR-MK07 (`pr-mk07-persistence-partitions`).

### Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_wo4_flag_coordination_tests.md` - Architectural details on display bridge refactoring, HA sensor guards, and size savings.
- `context_artifacts/graveyard_wo4_flag_coordination.md` - Build flag syntax traps (`-UWITH_TIME`) and library dependency resolution.
