# Work Order 2: Interface Reference Files

## Context
We are restructuring the `framework-expert` skill. Phase 1 (Core Skeleton) is complete. You are executing Phase 2 (parallel execution) — specifically, **WO2: Interfaces**. Your job is to create one complete reference file per interface with full method signatures, contracts, embedded safety notes, and "Why not X?" callouts.

## ⚠️ Mandatory Pre-Flight Checklist
1. **Read `.agents/rules/` first** — read all files in this directory in filename order.
2. **Load the skill-creator skill** — use `view_file` on `~/.gemini/config/skills/skill-creator/SKILL.md`. You MUST apply its principles of progressive disclosure and token budgeting.
3. **Plan-gate suspension** — The "Plan Gate" rule (`01-plan-before-execute.md`) is SUSPENDED for this Work Order. You have explicit, pre-approved authorization. Do NOT ask for permission to proceed.

## Context Artifacts
Read the Principal Architect's implementation plan for details:
`~/.gemini/antigravity/brain/5ad94103-1cd2-41df-9eec-19be9c51778c/implementation_plan.md` (Read the WO2 section)

## Execution Steps

Work in `.agents/skills/framework-expert/interfaces/`. Create the following files:

1. `tickable.md` (~70 lines): Signature, 10ms budget (ADR-17), throttle pattern, TWDT warning, `yield()`, and ADR-16 (static scope) constraint.
2. `configurable.md` (~90 lines): 5 methods + migration, ArduinoJson v7 `|` operator, `saveModuleConfig()`.
3. `web-contributor.md` (~80 lines): Routes, tabs, sub-tabs, `getTabIcon()`, PsychicHttp patterns, NATIVE_BUILD stub.
4. `mcp-contributor.md` (~60 lines): `registerMCPTools()`, `mutatesState` parameter, read-only mode blocking.
5. `config-gate.md` (~80 lines): ALL 5 methods: `isSatisfied()`, `getGateName()`, `getHelpText()`, `getDependencies()`, `getRegressionToleranceMs()`. Include regression tolerance explanation.
6. `data-source.md` (~90 lines): Threading model, `std::atomic`, WiFiClientSecure stack placement (ADR-14), `UpdateStatus` enum, `PriorityTier`.
7. `display-board.md` (~80 lines): ALL 6 methods: `render()`, `getBoardName()`, `isExpired()`, `onActivate()`, `onDeactivate()`, `tick()`. Include full lifecycle.
8. `status-reporter.md` (~50 lines): `reportStatus()`, `reportError()`, `clearStatus()`, NullStatusReporter fallback.
9. `ota-verifier.md` (~60 lines): `begin()`, `update()`, `verify()` — streaming crypto (ADR-13).

You should use your code search tools to inspect the actual `.hpp` files in `include/interfaces/` to get the exact method signatures and contracts.

## Acceptance Criteria
- [ ] Every method from every interface header is documented.
- [ ] Each file includes: full signature, contract rules, code example, embedded safety callout, common mistakes.
- [ ] ADR-16 (static scope) mentioned in `tickable.md` as a global constraint.
- [ ] `config-gate.md` includes regression tolerance explanation.
- [ ] `display-board.md` includes full lifecycle (activate → tick → render → deactivate → expire).
- [ ] No file significantly exceeds 100 lines.

When complete, STOP and report back. Do NOT write a distilled summary.
