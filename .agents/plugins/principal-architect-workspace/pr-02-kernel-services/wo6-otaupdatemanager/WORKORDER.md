---
title: "WO6: otaUpdateManager"
project: "PR-02: Kernel Services"
branch: "pr-02-wo6-otaupdatemanager"
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
Implement the `otaUpdateManager` kernel service.

## Constraints & Requirements
- **Directory**: `modules/otaUpdateManager/`
- **Responsibilities**: Streaming MD5 validation, OTA rollback handling, blessing the new firmware image AFTER WiFi and LittleFS are confirmed operational.
- **Reference Mining**: You may look at `departures-board/modules/otaUpdateManager/` for GitHub HTTPS, streaming MD5, rollback patterns. Must write fresh code.
- **House Style**: Apply the `house-style-docs` standards to every header and source file.
- **Branch**: Create and checkout branch `pr-02-wo6-otaupdatemanager` derived from `pr-02-kernel-services`.

## Deliverables
- [ ] `modules/otaUpdateManager/otaUpdateManager.hpp` and `.cpp`
- [ ] `test/test_hardware/test_ota/`

## Closure Gate
- **Hardware Integration Test**: Verify OTA streaming MD5 implementation. (Flag to Chief Architect if device unavailable, mark build-verified/hardware-pending and continue).
