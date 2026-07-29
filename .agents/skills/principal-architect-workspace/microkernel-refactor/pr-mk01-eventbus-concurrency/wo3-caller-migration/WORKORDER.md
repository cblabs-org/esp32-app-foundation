---
name: pr-mk01-eventbus-concurrency-wo3
description: Handoff instructions for Work order 3
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Migrate all `EventBus` publishers and subscribers to use the new concurrency contract and handle the `esp_event` async dispatch semantics on-target. Ensure all 8 failing test suites pass by updating the callers. Add concurrency and regression tests to exercise cross-task publish/subscribe behavior.

# Strict Boundary Box
- You may touch `modules/EventBus/*` for test additions, and any module or component that uses `EventBus` (publishers/subscribers) to align them with the new contract.
- If fixing tests in `test/`, update the test setup to ensure proper EventBus initialization (`EventBus::begin()`) and cleanup if required.
- Do not make architectural changes to module ownership or initialization beyond what is required to fix the tests.
- Surface any findings out of scope in your completion report; do not file issues.

# Tooling Constraints
Use the `platformio` MCP tools (`build_project`, `run_tests`) — **not** raw `pio` CLI. Native test env: `native`. Verify both `native` and `esp32dev` compile.

# Checklist
- [ ] Read `docs/microkernel-architecture-design.md` for context (specifically MK-11, MK-12, and MK-9).
- [ ] Analyze the 8 ERRORED test suites (`test_eventbus`, `test_application`, `test_configManager`, `test_timeManager`, `test_authManager`, `test_mcpServer`, `test_homeassistant`, `test_lvglDisplay`). Fix them by adjusting how they interact with `EventBus`.
- [ ] Ensure any tests simulating `esp_event` async behavior or depending on synchronous dispatch are updated.
- [ ] Audit on-target modules (e.g. `OTAUpdateManager`, `DataManager`) for thread safety now that `EventBus` is async.
- [ ] Add concurrency tests exercising cross-task publish/subscribe to `test_eventbus`.
- [ ] Verify compilation on `esp32dev` env using `call_mcp_tool` with `platformio` `build_project`.
- [ ] Verify native tests on `native` env using `call_mcp_tool` with `platformio` `run_tests`. (All 153 tests should pass).
- [ ] Ask the user for approval. Only after explicit approval, invoke `@distillery` to package the output targeting `.agents/skills/principal-architect-workspace/microkernel-refactor/pr-mk01-eventbus-concurrency/wo3-caller-migration/`.
- [ ] Append a status update to this `WORKORDER.md` file.
