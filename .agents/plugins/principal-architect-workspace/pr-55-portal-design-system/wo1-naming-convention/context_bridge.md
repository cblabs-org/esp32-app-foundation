---
title: WO1 BEM Naming Convention ADR + Legacy CSS Class Migration Distillation
distilled_at: 2026-08-05T17:10:00Z
original_plan_id: pr-55-portal-design-system
artifacts:
  - context_artifacts/adr_bem_migration.md
  - context_artifacts/graveyard_wo1_lessons.md
---

# WO1 — Context Bridge

## Executive Summary
Work Order 1 is complete and verified. Adopting BEM (`block--modifier`) across the web portal design system eliminated 23 legacy non-conforming classes, established `adr-css-naming-convention.md`, and updated all JS/HTML and Playwright test references with zero breakages (70/70 Playwright tests passing).

## Next Steps
Proceed to **Work Order 2 (`wo2-debt-repair`)**:
1. Formalize `.card` CSS class (extracting bare `article` styles) and `.alert` primitives (`--success`, `--error`, `--warning`, `--info`).
2. Extend `ApiQueue` with priority lanes (`highQueue` vs `lowQueue`) for pre-emption.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_bem_migration.md` - BEM CSS naming convention migration decision and rules
- `context_artifacts/graveyard_wo1_lessons.md` - Debugging notes, sandbox command fixes, and test assertion updates
