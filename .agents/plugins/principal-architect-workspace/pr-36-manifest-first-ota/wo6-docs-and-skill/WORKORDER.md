---
name: pr-36-wo6
description: Handoff instructions for Work Order 6 (Documentation + Framework-Expert Skill Update)
complexity: mechanical
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Update all documentation and the `/framework-expert` skill to reflect the manifest-first OTA architecture. Remove all references to the title hack, floating tags as a version mechanism, and the GithubClient-based flow.

# Strict Boundary Box
- DO NOT touch any C++ source files or Python scripts.
- Only modify documentation and skill files.

# Context
Read the project plan for the full architecture description:
`.agents/plugins/principal-architect-workspace/pr-36-manifest-first-ota/PROJECT.md`

# Line-Item Instructions

## docs/developer/ota-update-system.md

- [x] Rewrite section 4.1 (Release Discovery) to describe the manifest-first flow: single manifest URL → download + verify → channel selection → version comparison → download binary.
- [x] Remove section 4.1.1 (Floating Tags) or replace with a note that floating tags are deprecated in favour of the signed manifest.
- [x] Update the pipeline steps diagram/table to reflect the new flow (2 requests for "no update", 4 for "update").
- [x] Add a new section on manifest signing — describe the threat model, the detached signature, and the verification-first rule.
- [x] Add a new section on manifest URL resilience — compiled-in vs learned URL, fallback behaviour.
- [x] Update the "Defence-in-Depth" section to include the three gates: manifest signature → firmware signature → binary version header.
- [x] Update the error table if new error codes were added.
- [x] Remove any references to `GithubClient` in the OTA flow description.

## .agents/skills/framework-expert/systems/ota/deployment.md

- [x] Rewrite the "Floating Tags" section to describe the `ota-manifest` fixed release approach.
- [x] Update the deploy script documentation to reflect `manifest.json` + `manifest.json.sig` upload to the `ota-manifest` release.
- [x] Update the manifest format section to show the v2 schema with channels and absolute URLs.
- [x] Remove the title hack documentation and any "CRITICAL CONTRACT" notes about release titles.
- [x] Add documentation on manifest signing and the `--legacy-floating-tags` flag.

## .agents/skills/framework-expert/SKILL.md

- [x] Update trigger keywords: add "manifest signing", "manifest-first", "otaManifestUrl", "ota-manifest release", "manifest.json.sig", "signed manifest".
- [x] Remove or update trigger keywords related to the old architecture: "title hack", "floating tag version", "release title contract".
- [x] Update any inline descriptions that reference the old GithubClient-based OTA flow.

# Verification Requirement
Read each modified file end-to-end to verify consistency. Ensure no stale references to the old architecture remain. **CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.

# Status

**✅ COMPLETE** — committed `e83a122` on `pr-36-manifest-first-ota-wo6-docs-and-skill` (2026-06-20)

3 files changed, +273 −124 lines. Zero stale references to old architecture confirmed via comprehensive grep scan.

