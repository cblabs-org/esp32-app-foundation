---
name: pr-35-wo4
description: Handoff instructions for Work Order 4 — Home Assistant Entity Parity
complexity: architect
---

# Work Order 4: Home Assistant Entity Parity

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Expand the `HomeAssistantModule` entity catalog to achieve full feature parity with the `arduino_ha` library.

## Strict Boundary Box
- You may ONLY modify files within: `modules/HomeAssistant/entities/`, `test/test_native/test_homeassistant/`, `modules/HomeAssistant/HomeAssistantModule.hpp`/`.cpp`.
- Do NOT touch `sample-app/`.

## Tooling Constraints
- **CRITICAL:** You MUST use the PlatformIO MCP server tools (`build_project`, `run_tests`, `check_project`) to verify compilation and run tests. Do NOT use raw `pio` CLI commands in the terminal.
- Read the framework-expert skill at `.agents/skills/framework-expert/SKILL.md` for house style and coding conventions.

## Line-Item Instructions
- [ ] **1.** Implement `HaBinarySensor` (boolean state telemetry).
- [ ] **2.** Implement `HaButton` (triggering stateless actions from HA).
- [ ] **3.** Implement `HaLight` (supporting brightness, RGB/HSV color, and effect state routing).
- [ ] **4.** Implement `HaFan` (speed and oscillation state routing).
- [ ] **5.** Implement `HaClimate` (target temperature, current temperature, and HVAC mode routing).
- [ ] **6.** Implement `HaCover` (position tracking and open/close/stop command routing).
- [ ] **7.** Implement `HaNumber` and `HaSelect` (for arbitrary slider and dropdown configurations).
- [ ] **8.** Implement `HaDeviceTrigger` (for stateless device-level events).
- [ ] **9.** Add exhaustive native unit tests (`test/test_native/`) for every entity class to verify their Discovery JSON outputs strictly match the Home Assistant MQTT Discovery schema specifications.
  - **Constraints:** Optimize all `const char*` storage (avoiding `String` usage where possible) to prevent heap fragmentation across dozens of entities. All entities must inherit from `IHaEntity` and correctly encapsulate their domain-specific Discovery JSON generation and state routing.

## Verification Requirement
1. Run `build_project` via the PlatformIO MCP server for the `esp32s3nano` environment to verify compilation.
2. Run `run_tests` via the PlatformIO MCP server for the `native` environment to verify your unit tests pass.
3. **CRITICAL:** Before distilling or committing, STOP and send a message back to the Principal Architect confirming results. Only after approval should you invoke the `@distillery` skill.
