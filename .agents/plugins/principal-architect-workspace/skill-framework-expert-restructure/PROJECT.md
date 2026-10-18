---
name: skill-framework-expert-restructure
status: "complete"
branch: pr-25-framework-docs-skill
merge_commit: e3388d1
---

# Framework-Expert Skill Restructure

**Status:** ✅ COMPLETE — merged as part of PR-25 (`e3388d1`)
**Branch:** `pr-25-framework-docs-skill`
**Parent Project:** [pr-25-framework-docs-skill](../pr-25-framework-docs-skill/PROJECT.md)

## Objective

Decompose the monolithic `framework-expert` SKILL.md into an 8-domain
progressive-reveal architecture with reference docs, a security section,
compliance audit guide, and human-facing developer guides. This was the
largest sub-effort within PR-25 and warranted its own workspace due to
the 13 work orders involved.

## Architecture

The restructure produced:
- **8 skill domains**: Core interfaces, coding patterns, system trees (OTA,
  config, display, web, MCP, WiFi), performance & safety, security
- **Progressive-reveal references**: Each domain has its own reference doc
  loaded on demand rather than baked into the SKILL.md
- **Human developer guides**: 12 human-facing docs covering module creation,
  build/flash, OTA, security, MCP, etc.
- **Compliance audit guide**: Structured checklist for reviewing modules
  against framework interfaces and patterns

## Work Order Ledger

### Phase 1 — Core Restructure

| WO | Directory | Topic | Status |
|---|---|---|---|
| WO1 | [wo1-core-skeleton](wo1-core-skeleton/WORKORDER.md) | Skeleton rewrite — 8 domains, progressive refs | ✅ Done |
| WO2 | [wo2-interfaces](wo2-interfaces/WORKORDER.md) | Interface contracts domain | ✅ Done |
| WO3 | [wo3-coding-patterns](wo3-coding-patterns/WORKORDER.md) | Coding patterns domain | ✅ Done |
| WO4 | [wo4-systems-trees](wo4-systems-trees/WORKORDER.md) | System domain trees (OTA, config, display, web, MCP, WiFi) | ✅ Done |
| WO5 | [wo5-perf-safety](wo5-perf-safety/WORKORDER.md) | Performance & safety domain | ✅ Done |
| WO6 | [wo6-developer-guides](wo6-developer-guides/WORKORDER.md) | Human-facing developer guides | ✅ Done |

### Phase 2 — Hot-Deploy Fixes

| WO | Directory | Topic | Status |
|---|---|---|---|
| HD0 | [wo-hd0-corrections](wo-hd0-corrections/WORKORDER.md) | Corrections pass | ✅ Done |
| HD1a | [wo-hd1a](wo-hd1a/WORKORDER.md) | Skill overhaul — structure and content fixes | ✅ Done |
| HD1b | [wo-hd1b](wo-hd1b/WORKORDER.md) | Docs corrections + human doc refinements | ✅ Done |
| HD2 | [wo-hd2](wo-hd2/WORKORDER.md) | Final quality pass | ✅ Done |

### Phase 3 — Skill Gap Remediation

| WO | Directory | Topic | Status |
|---|---|---|---|
| SK1 | [wo-sk1-fixes](wo-sk1-fixes/WORKORDER.md) | Fix 5 skill gap findings | ✅ Done |
| SK2 | [wo-sk2-additions](wo-sk2-additions/WORKORDER.md) | Add missing coverage | ✅ Done |
| SK3 | [wo-sk3-security](wo-sk3-security/WORKORDER.md) | Security domain expansion | ✅ Done |

## Notes

- This workspace was created retroactively. The work was executed as part of
  PR-25 but the 13 WOs were complex enough to warrant a separate workspace.
- No `context_bridge.md` was generated at the time. The PR-25 PROJECT.md
  covers the broader scope including this restructure.
- Phase 3 (SK1–SK3) was triggered by findings from the PR-35 deep review
  session and committed on main via `b63b919`.
