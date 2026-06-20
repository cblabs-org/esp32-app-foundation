---
title: "WO6 Documentation + Framework-Expert Skill Update — Manifest-First OTA"
distilled_at: "2026-06-20T12:08:00Z"
original_plan_id: "pr-36-manifest-first-ota"
artifacts:
  - context_artifacts/adr_documentation_rewrite_strategy.md
  - context_artifacts/state_files_changed.md
---

# WO6 Distillation — Docs & Skill Update for Manifest-First OTA

## Executive Summary

Rewrote three documentation/skill files to fully reflect the manifest-first OTA architecture introduced in PR-36 (WO1–WO5). All references to GithubClient, floating tags, title hack, and v1 manifest schema have been replaced with the new manifest-first flow, v2 multi-channel schema, manifest signing, URL resilience, and three-gate defence-in-depth model. Committed as `e83a122` (+273 −124 lines, 3 files).

## Next Steps

1. **Merge into PR branch:** The commit is on `pr-36-manifest-first-ota-wo6-docs-and-skill`. Merge into the PR-36 feature branch when ready.
2. **Review §7.2 table formatting:** The repository configuration table in `ota-update-system.md` §7.2 now has long manifest URL strings in table cells — review rendering in GitHub markdown preview.
3. **Verify `--legacy-floating-tags` flag exists in deploy script:** The deployment.md documents this flag; confirm the WO4 deploy script actually implements it. If not, either add it or remove the documentation reference.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_documentation_rewrite_strategy.md` - Design decisions for the documentation rewrite scope and approach
- `context_artifacts/state_files_changed.md` - Exact files and sections modified with per-section change summaries
