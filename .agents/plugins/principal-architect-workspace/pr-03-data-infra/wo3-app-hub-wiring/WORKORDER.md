---
title: "WO3: NullStatusReporter & App Hub Wiring"
description: "Implementation of NullStatusReporter and integrating EventBus and DataManager into the Application hub."
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
Create and checkout branch `pr-03-wo3-app-hub-wiring` from `pr-03-data-infra`.

## Task: NullStatusReporter & Application Wiring
Integrate the newly created systems into the kernel.

### Constraints & Requirements
- **NullStatusReporter**:
  - Implement `NullStatusReporter` inline within `Application` (or as a private/internal class to `Application`).
  - It acts as a silent no-op when no display is registered.
- **Application Hub Wiring**:
  - Wire `EventBus` and `DataManager` into the `Application` hub's `begin()` and `tick()` methods.
  - Publish `"system.heap"` event via `EventBus` after `_compactAndLock()` is called.
- **Testing Tier 1**: Update `test/test_unit/test_application/` if necessary. Native tests MUST pass.
- **Testing Tier 2**: It must build clean on both `esp32dev` and `esp32s3nano`.

## Closure Gate
- Confirm native tests pass.
- Confirm build succeeds on both environments.
- **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/plugins/principal-architect-workspace/pr-03-data-infra/wo3-app-hub-wiring/` as the target directory. Wait for it to finish before dropping control back to the PA.
