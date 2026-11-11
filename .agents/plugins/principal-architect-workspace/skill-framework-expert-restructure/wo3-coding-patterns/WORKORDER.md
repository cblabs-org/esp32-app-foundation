# Work Order 3: Coding Patterns Tree

## Context
We are restructuring the `framework-expert` skill. Phase 1 (Core Skeleton) is complete. You are executing Phase 2 (parallel execution) — specifically, **WO3: Coding Patterns Tree**. Your job is to document the OO best practices, style guide, and architectural patterns that the framework enforces.

## ⚠️ Mandatory Pre-Flight Checklist
1. **Read `.agents/rules/` first** — read all files in this directory in filename order.
2. **Load the skill-creator skill** — use `view_file` on `~/.gemini/config/skills/skill-creator/SKILL.md`. You MUST apply its principles of progressive disclosure and token budgeting.
3. **Plan-gate suspension** — The "Plan Gate" rule (`01-plan-before-execute.md`) is SUSPENDED for this Work Order. You have explicit, pre-approved authorization. Do NOT ask for permission to proceed.

## Context Artifacts
Read the Principal Architect's implementation plan for details:
`~/.gemini/antigravity/brain/5ad94103-1cd2-41df-9eec-19be9c51778c/implementation_plan.md` (Read the WO3 section)
You should also read the following reference materials to distill from:
- `.agents/skills/oo-expert/SKILL.md`
- `.agents/skills/oo-expert/references/hierarchical-object-model.md`
- `.agents/skills/oo-expert/references/service-module-pattern.md`
- `.agents/plugins/principal-architect-workspace/adhoc-oo-expert-review/context_bridge.md`
- `.agents/skills/house-style-docs/references/cpp-embedded.md`

## Execution Steps

Work in `.agents/skills/framework-expert/coding-patterns/`. Create the following files:

1. `overview.md` (~80 lines): Modules vs libs table, SOLID mapping to framework, DI conventions, deferred DIP interfaces.
2. `style-guide.md` (~120 lines): Distill from `house-style-docs/references/cpp-embedded.md`: naming table, interface-grouped header template, Doxygen, NATIVE_BUILD stubs, directory↔file↔class alignment, acronym rules. Make it specific to this framework.
3. `hierarchical-object-model.md` (~100 lines): Domain Meaning Test, concrete-at-caller/interface-at-engine, data vs presentation concerns, Two-Step Smell, layered subsystem priority, display module as canonical example.
4. `service-module-pattern.md` (~90 lines): 4 symptoms of Orchestrator Anti-Pattern, correct inversion, service-intrinsic exception, coupling audit table, OTAUpdateManager as sole remaining violation.
5. `common-anti-patterns.md` (~70 lines): Global state, hub references, monolithic Config, `std::function` heap fragmentation (GRAVE-05), late registration, stack-allocated modules (ADR-16).

## Acceptance Criteria
- [ ] Modules vs libs distinction is clear with decision criteria.
- [ ] All 5 SOLID principles mapped to concrete framework mechanisms.
- [ ] HOM includes the Domain Meaning Test with pass/fail examples from the display module.
- [ ] SMP includes the coupling audit table.
- [ ] Style guide covers: naming table, header template, Doxygen, directory alignment, acronym rule.
- [ ] Anti-patterns includes the 6 rejected approaches with rationale.

When complete, STOP and report back. Do NOT write a distilled summary.
