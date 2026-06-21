---
name: pr-35-wo1
description: Handoff instructions for Work Order 1 — Home Assistant Core Module
complexity: standard
---

# Work Order 1: Home Assistant Core Module

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Build the `HomeAssistantModule` as a core framework component wrapping `esp-mqtt`.

## Strict Boundary Box
- You may ONLY modify files within: `modules/HomeAssistant/`, `test/test_native/test_homeassistant/`, `modules/CMakeLists.txt` (or equivalent build inclusions).
- Do NOT touch `sample-app/` or any other modules.

## Tooling Constraints
- **CRITICAL:** You MUST use the PlatformIO MCP server tools (`build_project`, `run_tests`, `check_project`) to verify compilation and run tests. Do NOT use raw `pio` CLI commands in the terminal (e.g. no `pio run` or `pio test`).
- Read the framework-expert skill at `.agents/skills/framework-expert/SKILL.md` for house style and coding conventions.

## Line-Item Instructions
- [ ] **1.** Create `modules/HomeAssistant/HomeAssistantModule.hpp` and `.cpp`.
- [ ] **2.** Implement `ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor`.
  - **10ms Tick Budget:** `tick()` must remain strictly non-blocking. Offload all MQTT and TLS handling to `esp-mqtt` background tasks.
  - **Dependency Injection:** Applications wire the module via `setHomeAssistant(&haModule)`, not a framework getter.
- [ ] **3.** Build the `IHaEntity`, `HaSensor`, and `HaSwitch` concrete classes inside `modules/HomeAssistant/entities/`. (`HaSwitch` proves bidirectional inbound MQTT command routing.)
- [ ] **4.** Integrate with `EventBus` for `wifi.connected`/`disconnected` events.
- [ ] **5.** Generate Home Assistant Discovery JSON payloads with MAC-prefixed `unique_id` (e.g., `aabbccddeeff_entity_id`) restricted to `[a-zA-Z0-9_-]`, and per-entity `state_topic` (e.g., `homeassistant/sensor/aabbccddeeff/outside_temp/state`). Use dynamically chunked JSON payloads with `ArduinoJson` to avoid 256-byte limits.
- [ ] **6.** Configure the shared Last Will and Testament (LWT) topic.
- [ ] **7.** Implement post-reconnect state sync (re-publish all cached entity states on `MQTT_EVENT_CONNECTED`).
- [ ] **8.** Add the "Home Assistant" top-level tab to the web portal with connection settings and TLS cert upload pills (use `KeyManager` for pinned certs).
- [ ] **9.** Implement `ha_get_state` and `ha_call_service` MCP tool handlers via `IMCPContributor`.
- [ ] **10.** Add native unit tests for discovery JSON generation, MAC-prefixed unique IDs, LWT topic construction, and per-entity state topic formatting.

## Verification Requirement
1. Run `build_project` via the PlatformIO MCP server for the `esp32s3nano` environment to verify compilation.
2. Run `run_tests` via the PlatformIO MCP server for the `native` environment to verify your unit tests pass.
3. **CRITICAL:** Before distilling or committing, STOP and send a message back to the Principal Architect confirming results. Only after approval should you invoke the `@distillery` skill.
