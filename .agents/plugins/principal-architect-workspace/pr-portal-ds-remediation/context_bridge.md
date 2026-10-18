---
title: Portal Design System Remediation + Stacked Branch Recovery
distilled_at: 2026-08-11T12:33:00Z
original_plan_id: pr-portal-ds-remediation
artifacts:
  - context_artifacts/adr_css_specificity_touch.md
  - context_artifacts/adr_squash_merge_stack_recovery.md
  - context_artifacts/graveyard_mcp_connection_eof.md
---

# Portal Design System Remediation — Context Bridge

## Executive Summary

Eliminated all 39 design system violations from the web portal (4 work orders: colour tokens, monospace font, icon remediation, ARIA tab semantics), discovered and fixed a CSS specificity bug causing invisible pill text on touch devices, then recovered two orphaned stacked branches (wo6/wo7) that were stranded by GitHub squash-merge before rebasing the DS PR onto the updated main.

## Outcome

- **PR #87** (portal DS remediation) — merged to `main`
- **PR #88** (wo6 DisplayServiceHelper) — recovered and merged to `main`
- **PR #89** (wo7 portal sub-pills) — recovered and merged to `main`
- Stale remote branches `display-interface/wo5-portal-scripts-docs`, `wo6-service-helper`, `wo7-portal-pills` deleted
- CrowPanel flashed and verified at each stage

## Next Steps

- No immediate follow-ups from this session
- The portal is now fully DS-compliant; future portal changes should reference `var(--token)` exclusively
- Any new pills or interactive elements must include `:hover` specificity overrides (see ADR below)

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_css_specificity_touch.md` — Root-cause of invisible active-pill text on touch screens; the `:hover` sticky specificity problem and fix pattern
- `context_artifacts/adr_squash_merge_stack_recovery.md` — How to recover orphaned stacked branches after GitHub squash-merge breaks the chain
- `context_artifacts/graveyard_mcp_connection_eof.md` — PlatformIO MCP server EOF failures and CLI fallback pattern
