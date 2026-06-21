---
name: pr-35-wo2
description: Handoff instructions for Work Order 2 — WeatherModule Logic
complexity: standard
---

# Work Order 2: WeatherModule Logic

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Build the `WeatherModule` for the sample app that fetches OpenWeatherMap data and pushes it to Home Assistant.

## Strict Boundary Box
- You may ONLY modify files within: `sample-app/src/`.
- Do NOT touch `modules/` or any framework code.

## Tooling Constraints
- **CRITICAL:** You MUST use the PlatformIO MCP server tools (`build_project`, `run_tests`, `check_project`) to verify compilation and run tests. Do NOT use raw `pio` CLI commands in the terminal.
- Read the framework-expert skill at `.agents/skills/framework-expert/SKILL.md` for house style and coding conventions.

## Line-Item Instructions
- [ ] **1.** Create `sample-app/src/WeatherModule.hpp` and `.cpp`.
- [ ] **2.** Implement `ITickable`, `IConfigurable`, `IWebContributor`.
- [ ] **3.** Contribute a **"Weather" top-level tab** (replacing the generic "App" tab) with sub-pills:
  - **Settings**: Latitude, Longitude, Polling Interval.
  - **API Key**: Instructions for obtaining an OpenWeatherMap key + secure input via `KeyManager`.
- [ ] **4.** Instantiate `HaSensor` objects for Temperature, Wind Speed, and Weather Condition.
- [ ] **5.** Register the sensors with the injected `HomeAssistantModule*` (received via `setHomeAssistant()`).
- [ ] **6.** Fetch JSON data from OpenWeatherMap using the API key securely retrieved from `KeyManager`.
  - **Non-blocking Fetch:** Use the `IDataSource` pattern to ensure the JSON fetch does not block the 10ms tick budget.
  - **Clean-room Provenance:** Write the parsing logic completely from scratch using `JsonStreamingParser` (do not copy Gadec UK code).
- [ ] **7.** Publish states to the `HaSensor` objects to sync with Home Assistant.
- [ ] **8.** Broadcast a `weather.updated` event to the `EventBus` for local displays.

## Verification Requirement
1. Run `build_project` via the PlatformIO MCP server for the `esp32s3nano` environment to verify compilation.
2. **CRITICAL:** Before distilling or committing, STOP and send a message back to the Principal Architect confirming results. Only after approval should you invoke the `@distillery` skill.
