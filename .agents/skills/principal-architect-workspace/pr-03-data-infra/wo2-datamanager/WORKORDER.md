---
title: "WO2: dataManager"
description: "Implementation of the FreeRTOS background fetch worker"
---
## MANDATORY SKILL BOOT SEQUENCE — Execute FIRST

Before reading any code or writing any file, run these exact view_file calls in order:
1. view_file `.agents/skills/pio-manager/SKILL.md`
2. view_file `.agents/skills/embedded-systems/SKILL.md`
3. view_file `.agents/skills/house-style-docs/SKILL.md`
4. view_file `.agents/skills/oo-expert/SKILL.md`
5. view_file `.agents/skills/pio-manager/references/mcp-agent-reference.md`

Do NOT proceed past this block until all five are confirmed loaded.
---

## Git Workflow
Create and checkout branch `pr-03-wo2-datamanager` from `pr-03-data-infra`.

## Task: Implement dataManager
Implement the background data manager in `modules/dataManager/`.

### Constraints & Requirements
- **Location**: `modules/dataManager/dataManager.hpp` and `dataManager.cpp`.
- **Implementation Rules**:
  - Implement a FreeRTOS task with `STACK_SIZE >= 8KB`.
  - Handle `iDataSource` scheduling loop.
  - Understand and utilize `PriorityTier`.
  - You may reference `departures-board/modules/dataManager/` for the queue pattern, but DO NOT copy the file wholesale.
- **Testing Tier 1**: Add native unit tests in `test/test_unit/test_dataManager/` (test scheduling logic). Native tests MUST pass.
- **Testing Tier 2**: It must build clean on both `esp32dev` and `esp32s3nano`.

## Closure Gate
- Confirm native tests pass.
- Confirm build succeeds on both environments.
- **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/skills/principal-architect-workspace/pr-03-data-infra/wo2-datamanager/` as the target directory. Wait for it to finish before dropping control back to the PA.
