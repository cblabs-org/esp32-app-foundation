---
name: mk02-wo1
description: Handoff instructions for Work order 1 (Contract clause + docs)
complexity: mechanical
---

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Promote the "≤10 ms per tick()" obligation from a comment to an explicit contract clause in `ITickable`, and update relevant documentation to reflect this first-class contract.

## Strict Boundary Box
- Touch **only** the `ITickable.hpp` file and the `creating-a-module.md` / `performance-guide.md` documentation files.
- **Do not** alter module ownership, EventBus, or composition.
- **Do not** file issues for out-of-scope discoveries; surface them in your completion report instead.

## Tooling Constraints
- Prioritize using the `platformio` MCP server tools (e.g., `build_project`, `run_tests`) for any compilation or verification.
- Verify that your changes compile successfully under `native` and `esp32dev` environments.

## Line-Item Instructions
- [ ] Edit `include/interfaces/ITickable.hpp` to make the ≤10 ms tick budget an explicit contract clause in the class documentation/comments.
- [ ] Update `docs/developer/creating-a-module.md` to mention the 10 ms budget in relevant sections regarding the tick cycle.
- [ ] Update `docs/developer/performance-guide.md` to reinforce the 10 ms budget.

## Verification Requirement
1. Run `platformio` MCP tool `build_project` for `native` and `esp32dev` to ensure the project still compiles.
2. Run `platformio` MCP tool `run_tests` to ensure tests pass.
3. Stop and ask the user (me, the Principal Architect) if they are happy with the changes. **Only after approval**, invoke the `@distillery` skill to package the state and drop control, passing `.agents/skills/principal-architect-workspace/microkernel-refactor/pr-mk02-kernel-hardening/wo1-contract-docs/` as the target directory.
4. Append a status update to this `WORKORDER.md` file and terminate your session.
