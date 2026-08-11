---
title: "ARIA Tabpanel Semantics — Portal Accessibility Fix"
distilled_at: "2026-08-11T14:47:00+01:00"
original_plan_id: "issue-92"
artifacts:
  - context_artifacts/adr_aria_pattern_choice.md
  - context_artifacts/graveyard_worker_mcp_violation.md
---

# ARIA Tabpanel Semantics — Session Bridge

## Executive Summary

Resolved GitHub issue #92 by migrating the web portal from an incomplete single-wrapper `role="tabpanel"` pattern to the standard WAI-ARIA multi-panel tabs pattern. PR #93 merged — covers both top-level (bottom nav) and sub-pill (pill bar) levels with full ARIA attributes including `role="tabpanel"`, `aria-labelledby`, `aria-controls`, `aria-selected`, and `tabindex="0"`.

## Next Steps

No follow-up work required. The ARIA tabs pattern is now complete at both levels.

If a future session touches the web portal tab system:
1. Read `framework-expert/systems/web-portal/overview.md` — steps 5–6 document the ARIA contract
2. Any new tab content div (static or dynamic) must include `role="tabpanel" tabindex="0" aria-labelledby="..."` 
3. Any new tab trigger must include `role="tab" aria-selected aria-controls="..."`
4. Sub-pill buttons must have `id="pill-{name}"` for the `aria-labelledby` back-reference

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_aria_pattern_choice.md` - Why we chose multi-panel over single-wrapper tabpanel pattern
- `context_artifacts/graveyard_worker_mcp_violation.md` - Flash-tier worker ignored MCP tooling constraints; mitigation for future WORKORDERs
