# Work Order 1: Core Skeleton & SKILL.md Rewrite

## Context

We are executing a massive restructuring of the `framework-expert` skill into a progressively-disclosed, multi-tree architecture. This Work Order (WO1) is the critical first step: it establishes the skeleton directory structure, the new routing table in `SKILL.md`, and the foundational reference files.

**All downstream work orders depend on the structure created here.**

## ⚠️ Mandatory Pre-Flight Checklist

1. **Read `.agents/rules/` first** — read all files in this directory in filename order.
2. **Load the skill-creator skill** — use `view_file` on `~/.gemini/config/skills/skill-creator/SKILL.md`. You MUST apply its principles of progressive disclosure, token budgeting, and trigger optimization.
3. **Plan-gate suspension** — The "Plan Gate" rule (`01-plan-before-execute.md`) is SUSPENDED for this Work Order. You have explicit, pre-approved authorization from the Principal Architect to implement this Work Order immediately. Do NOT ask for permission to proceed.

## Context Artifacts
Use `view_file` to read the Principal Architect's implementation plan and assessment:
1. `~/.gemini/antigravity/brain/5ad94103-1cd2-41df-9eec-19be9c51778c/framework_expert_skill_assessment.md` (Read sections: 5, 6, 8, 9)
2. `~/.gemini/antigravity/brain/5ad94103-1cd2-41df-9eec-19be9c51778c/implementation_plan.md` (Read the WO1 definition)

## Execution Steps

Work in the `.agents/skills/framework-expert/` directory.

### Step 1: Create the new directory structure
Create the following empty directories:
- `interfaces/`
- `systems/`
- `coding-patterns/`
- `performance/`
- `embedded-safety/`
- `guides/`

### Step 2: Delete the old flat reference files
Delete the following files from `references/` (they will be replaced by the domain trees in future work orders):
- `creating-a-module.md`
- `config-system.md`
- `web-portal.md`
- `event-bus.md`
- `optional-modules.md`
- `data-fetches.md`

### Step 3: Rewrite SKILL.md
Rewrite `SKILL.md` completely. It must serve as the progressive-disclosure routing hub.
- Expand description triggers based on the assessment (Section 8)
- Interface table: Include all 9 interfaces (add `IOTAVerifier`)
- Routing table: Must contain entries for ALL subdirectories (including the new trees: systems, coding-patterns, performance, embedded-safety, guides, interfaces)
- Keep it under ~150 lines.

### Step 4: Update foundational reference files
Create or rewrite the following files in `references/`:
- `architecture.md` (Rewrite, ~90 lines): Enhance with ADR rationale (ADR-04 Hollywood Principle, ADR-16 static scope, ADR-17 tick budget)
- `registration.md` (NEW, ~80 lines): Covers `registerModule<T>()` template, two-tier vector model, `_compactAndLock()`, CA-05 rationale
- `build-system.md` (Refine, ~95 lines)

### Step 5: Update metadata and processes
- `KEEPING-CURRENT.md` (Rewrite, ~40 lines)
- `skill_manifest.json` (Rewrite, ~45 lines): Update with the new file inventory (include all the new directories/files planned in the assessment, even if they will be created in WO2-WO5). Change version to `2.0.0`.

## Acceptance Criteria
- [ ] The 6 new directories are created.
- [ ] `SKILL.md` has routing entries for ALL new subdirectories.
- [ ] Interface table includes `IOTAVerifier`.
- [ ] `skill_manifest.json` lists the new file inventory and version is bumped.
- [ ] The specified flat reference files are deleted.
- [ ] No single file exceeds its estimated line count significantly.

When complete, STOP and report back to the Principal Architect. Do NOT write a distilled summary or wrap-up until explicitly told to do so.
