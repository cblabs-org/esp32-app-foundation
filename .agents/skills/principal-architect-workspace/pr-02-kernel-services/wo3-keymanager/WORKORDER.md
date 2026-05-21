---
title: "WO3: keyManager"
project: "PR-02: Kernel Services"
branch: "pr-02-wo3-keymanager"
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
Implement the `keyManager` kernel service.

## Constraints & Requirements
- **Directory**: `modules/keyManager/`
- **Responsibilities**: Provide an encrypted registry for credentials with methods: `addKey`, `getKeyById`, `deleteKey`.
- **Reference Mining**: Reference `departures-board` as needed, but write fresh.
- **House Style**: Apply the `house-style-docs` standards to every header and source file.
- **Branch**: Create and checkout branch `pr-02-wo3-keymanager` derived from `pr-02-kernel-services`.

## Deliverables
- [ ] `modules/keyManager/keyManager.hpp` and `.cpp`
- [ ] `test/test_unit/test_keyManager/` (must mock `DeviceCrypto` for native tests)

## Closure Gate
- **Native Unit Tests**: Native unit tests for `keyManager` MUST pass (testing mocked crypto round-trip).
- Run `mcp_platformio_run_tests` natively to verify.
