---
title: "WO4: wifiManager"
project: "PR-02: Kernel Services"
branch: "pr-02-wo4-wifimanager"
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
Implement the `wifiManager` kernel service.

## Constraints & Requirements
- **Directory**: `modules/wifiManager/`
- **Responsibilities**: AP captive portal, managing `AppState` transitions.
- **Reference Mining**: You may refer to `departures-board/modules/wifiManager/` for AP mode, reconnect state machine patterns. Write fresh.
- **House Style**: Apply the `house-style-docs` standards to every header and source file.
- **Branch**: Create and checkout branch `pr-02-wo4-wifimanager` derived from `pr-02-kernel-services`.

## Deliverables
- [ ] `modules/wifiManager/wifiManager.hpp` and `.cpp`
- [ ] `test/test_hardware/test_wifi/`

## Closure Gate
- **Hardware Integration Test**: Verify WiFi connects and `AppState` transitions correctly. Use hardware test on an actual ESP32 if available. 
- Flag to the Chief Architect if device is unavailable, mark as build-verified/hardware-pending and continue.
