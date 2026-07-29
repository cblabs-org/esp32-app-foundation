---
name: pr-mk01-eventbus-concurrency-wo2
description: Handoff instructions for Work order 2
complexity: architect
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Implement `esp_event` delegation behind the `EventBus` interface for the `esp32dev` and `esp32s3nano` targets, addressing the OQ-MK-3 resolution. Ensure the native build (`native`) retains the synchronous stub dispatch implementation from WO1. The `EventBus` public interface must remain stable to avoid unnecessarily breaking consumers.

# Strict Boundary Box
- Touch **only** `modules/EventBus/*`. Use pre-processor macros (`#ifndef NATIVE_BUILD`) to switch between the `esp_event` backing and the native stub backing.
- Surface any findings out of scope in your completion report; do not file issues or make out-of-scope changes.
- Do not migrate existing caller subscriptions outside of `EventBus` yet; that is for WO3. If a consumer build fails due to interface changes, adapt the interface instead of the consumer.

# Tooling Constraints
Use the `platformio` MCP tools (`build_project`, `run_tests`) — **not** raw `pio` CLI. Native test env: `native`. Verify both `native` and `esp32dev` compile.

# Checklist
- [x] Read `docs/microkernel-architecture-design.md` for context (specifically MK-11, MK-12, and OQ-MK-3 resolution).
- [x] Under `#ifndef NATIVE_BUILD`, configure the `EventBus` to initialize the default `esp_event` loop inside `EventBus::begin()` (e.g. `esp_event_loop_create_default()`).
- [x] Under `#ifndef NATIVE_BUILD`, map `subscribe()` to `esp_event_handler_instance_register` and `unsubscribe()` to `esp_event_handler_instance_unregister`.
- [x] Ensure that topics (which are now 32-bit hashes) are dispatched correctly via `esp_event` using `esp_event_post`. You may need to map topics to a specific event base and use the topic hash as the event ID, or use `ESP_EVENT_ANY_BASE` and pass the hash inside the event payload. Pick the best mapping for `esp_event`.
- [x] Under `#ifdef NATIVE_BUILD`, retain the synchronous vector-based dispatch logic from WO1 as the native stub.
- [x] Verify compilation on `esp32dev` env using `call_mcp_tool` with `platformio` `build_project`.
- [x] Verify native tests on `native` env using `call_mcp_tool` with `platformio` `run_tests`.
- [ ] Ask the user for approval. Only after explicit approval, invoke `@distillery` to package the output targeting `.agents/skills/principal-architect-workspace/microkernel-refactor/pr-mk01-eventbus-concurrency/wo2-on-target-backing/`.
- [x] Append a status update to this `WORKORDER.md` file.

### Status Update
- Replaced `NATIVE_BUILD` with `UNIT_TEST` in EventBus as `NATIVE_BUILD` was removed in PR-41.
- Implemented `esp_event` backing for ESP32 target environments.
- Kept synchronous vector-based EventBus for `UNIT_TEST` environment.
- Successfully verified `esp32dev` compilation.
- Successfully verified `native` unit tests via MCP tool.
