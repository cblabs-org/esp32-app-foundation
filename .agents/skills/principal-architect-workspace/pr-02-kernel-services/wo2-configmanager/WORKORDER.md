---
title: "WO2: configManager"
project: "PR-02: Kernel Services"
branch: "pr-02-wo2-configmanager"
status: "QUEUED"
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

## Objective
Implement the `configManager` kernel service.

## Constraints & Requirements
- **Directory**: `modules/configManager/`
- **Responsibilities**: Manage LittleFS `etc/config.json`, handle version migration, and provide a `requestReload()` capability.
- **Reference Mining**: You may look at `departures-board/modules/configManager/` for patterns. Must be written fresh.
- **House Style**: Apply the `house-style-docs` standards to every header and source file.
- **Branch**: Create and checkout branch `pr-02-wo2-configmanager` derived from `pr-02-kernel-services`.

## Deliverables
- [ ] `modules/configManager/configManager.hpp` and `.cpp`
- [ ] `test/test_unit/test_configManager/`

## Closure Gate
- **Native Unit Tests**: Native unit tests for `configManager` MUST pass (testing load/save/migrate functionality).
- Run `mcp_platformio_run_tests` natively to verify.
