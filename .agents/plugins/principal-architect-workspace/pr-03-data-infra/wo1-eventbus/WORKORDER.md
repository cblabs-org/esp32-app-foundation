---
title: "WO1: EventBus"
description: "Implementation of the EventBus typed pub/sub system"
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
Create and checkout branch `pr-03-wo1-eventbus` from `pr-03-data-infra`.

## Task: Implement EventBus
Implement the EventBus system in `modules/eventBus/`.

### Constraints & Requirements
- **Location**: `modules/eventBus/EventBus.hpp` and `EventBus.cpp`.
- **Implementation Rules**:
  - Typed pub/sub system.
  - MUST use a static function pointer table for subscribers (`void (*callback)(const char* topic, const void* payload, void* ctx)`).
  - Do NOT use `std::function`. Subscribers provide a static function pointer and a `void* ctx` (context).
  - `publish()` must be synchronous.
  - Payloads are only valid for the duration of the callback.
  - Include topic validation.
- **Testing Tier 1**: Add native unit tests in `test/test_unit/test_eventbus/` (test dispatch, lifetime, overflow). Native tests MUST pass.
- **Testing Tier 2**: It must build clean on both `esp32dev` and `esp32s3nano`.

## Closure Gate
- Confirm native tests pass.
- Confirm build succeeds on both environments.
- **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/plugins/principal-architect-workspace/pr-03-data-infra/wo1-eventbus/` as the target directory. Wait for it to finish before dropping control back to the PA.
