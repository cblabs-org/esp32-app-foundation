# Work Order 6: Developer Guides

## Context
We are restructuring the `framework-expert` skill. Phase 2 (Reference Trees) is complete. All interface, systems, performance, and coding pattern reference files exist. You are executing Phase 3 — specifically, **WO6: Developer Guides**. Your job is to create two persona-based progressive learning paths that tie the new structure together.

## ⚠️ Mandatory Pre-Flight Checklist
1. **Read `.agents/rules/` first** — read all files in this directory in filename order.
2. **Load the skill-creator skill** — use `view_file` on `~/.gemini/config/skills/skill-creator/SKILL.md`. You MUST apply its principles of progressive disclosure and token budgeting.
3. **Plan-gate suspension** — The "Plan Gate" rule (`01-plan-before-execute.md`) is SUSPENDED for this Work Order. You have explicit, pre-approved authorization. Do NOT ask for permission to proceed.

## Context Artifacts
Read the Principal Architect's implementation plan for details:
`~/.gemini/antigravity/brain/5ad94103-1cd2-41df-9eec-19be9c51778c/implementation_plan.md` (Read the WO6 section)

## Execution Steps

Work in `.agents/skills/framework-expert/guides/`. Create the following files:

1. `app-developer-quickstart.md` (~120 lines):
   - Progressive recipe for building an app (e.g., weather station/sensor dashboard).
   - Steps: template → ITickable → IConfigurable → IWebContributor → IDataSource → IDisplayBoard → EventBus.
   - For each step, include a tiny code snippet AND a relative link to the full interface reference in `../interfaces/`.
   - Include a "what to read next" section routing to `../systems/` and `../performance/` trees.

2. `module-developer-guide.md` (~100 lines):
   - Conventions for adding a kernel/core module to the framework.
   - Interface selection.
   - House style compliance (link to `../coding-patterns/style-guide.md`).
   - NATIVE_BUILD guards, integration testing, two-tier vector model.
   - Adding EventBus topics (link to `../systems/event-bus/custom-topics.md`).
   - Link to `../coding-patterns/` for OO patterns (SMP, HOM).

## Acceptance Criteria
- [ ] App developer guide walks through the 6 progressive steps.
- [ ] Each step includes a code snippet AND a relative link to the full interface reference.
- [ ] Module developer guide links to style guide and coding patterns.
- [ ] Both guides link to `../embedded-safety/` for constraints.
- [ ] Neither guide exceeds ~120 lines (progressive disclosure — link deeper, don't inline everything).

When complete, STOP and report back. Do NOT write a distilled summary.
