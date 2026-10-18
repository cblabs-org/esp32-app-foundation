---
title: "WO5: timeManager"
project: "PR-02: Kernel Services"
branch: "pr-02-wo5-timemanager"
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
Implement the `timeManager` kernel service.

## Constraints & Requirements
- **Directory**: `modules/timeManager/`
- **Responsibilities**: Manage NTP synchronization, handle POSIX timezone rules, ensure non-blocking tick updates.
- **House Style**: Apply the `house-style-docs` standards to every header and source file.
- **Branch**: Create and checkout branch `pr-02-wo5-timemanager` derived from `pr-02-kernel-services`.

## Deliverables
- [ ] `modules/timeManager/timeManager.hpp` and `.cpp`

## Closure Gate
- **Build Verification**: `esp32dev` must build cleanly.
- **Verification**: Ensure non-blocking tick is implemented and validated.
