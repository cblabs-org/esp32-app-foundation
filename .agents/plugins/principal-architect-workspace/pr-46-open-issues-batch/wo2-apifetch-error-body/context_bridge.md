---
title: "WO2 — Preserve Error Body in apiFetch (Issue #31)"
distilled_at: 2026-07-22T19:30:00Z
original_plan_id: pr-46-open-issues-batch
artifacts:
  - context_artifacts/adr_error_body_pattern.md
  - context_artifacts/state_modified_files.md
---

# WO2 — Context Bridge

## Executive Summary

Fixed `window.apiFetch()` to preserve structured JSON error bodies on non-2xx responses by attaching `.status` (number) and `.body` (parsed JSON / text / null) to the thrown Error at both throw sites (main path and auth retry path) in `ApiQueue.processNext()`. Updated `shared-utilities.md` to correct the return type, usage examples, and document the error shape. Committed as `079f5c6`.

## Completion Status

**Done.** All 5 checklist items complete. 69/69 Playwright tests passing. Committed and approved.

## Next Steps

- No further action needed for WO2.
- PA should proceed to WO3 (PSRAM guard, Issue #36).
- The commit `079f5c6` is on the `open-issues-batch/wo2-apifetch-error-body` branch and ready for stack integration.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_error_body_pattern.md` - Design rationale for the error body preservation pattern and backwards-compatibility constraints
- `context_artifacts/state_modified_files.md` - Exact files modified with line-level change summary
