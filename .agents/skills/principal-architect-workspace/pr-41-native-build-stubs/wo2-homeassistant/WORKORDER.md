---
name: pr41-wo2
description: Handoff instructions for Work Order 2 — Migrate HomeAssistant module
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Remove all `#ifdef NATIVE_BUILD` / `#ifndef NATIVE_BUILD` guards from the HomeAssistant module. The stub headers created in WO1 provide the API surface; the production code should compile cleanly in both native and hardware builds without any conditional compilation.

## Strict Boundary Box

- **DO** modify only files in `modules/HomeAssistant/`
- **DO** extend stub headers in `test/stubs/platform/` if the module needs API surface not yet provided
- **DO NOT** modify any other module's source files
- **DO NOT** modify `Application.hpp` or `Application.cpp`

## Tooling Constraints

- Use `platformio` MCP tools for building (`build_project`) and testing (`run_tests`)
- Use `background: true` for long-running operations

## Line-Item Instructions

- [ ] **`HomeAssistantModule.hpp`** — Remove `#ifndef NATIVE_BUILD` guard around `#include <mqtt_client.h>` and the `#else typedef void* esp_mqtt_client_handle_t` block. Include `<mqtt_client.h>` unconditionally (resolves to stub in native). Remove guard around `mqttEventHandler()` declaration.
- [ ] **`HomeAssistantModule.cpp`** — Remove all `#ifndef NATIVE_BUILD` guards:
  - Include guards for `<esp_log.h>`, `<esp_mac.h>`, `<WiFi.h>`, `<PsychicHttp.h>`, portal asset header
  - Constructor: remove guard around `esp_read_mac()` vs `"native_mac"` — let the stub's `esp_read_mac()` fill mock bytes
  - MQTT publish calls: remove guards around `esp_mqtt_client_publish()`
  - `connectMqtt()` / `disconnectMqtt()`: remove guards around MQTT client lifecycle
  - `mqttEventHandler()` static function: remove exclusion guard
  - `applyConfig()`: remove `WiFi.status()` guard
  - `registerRoutes()`: remove PsychicHttp guard
  - `registerMCPTools()`: remove guard
- [ ] **MAC address handling**: The current native stub returns `"native_mac"` as the MAC prefix. After removing the guard, the stub's `esp_read_mac()` will return `{0xAA,0xBB,0xCC,0xDD,0xEE,0xFF}`. Update any test expectations that rely on `"native_mac"` to match the new hex-formatted MAC prefix from the stub.
- [ ] **Verify**: Run `pio test -e native` — all HomeAssistant tests must pass
- [ ] **Verify**: Run `pio run -e esp32s3nano` — hardware build must succeed

## Verification Requirement

After completing all items, verify tests pass and hardware builds. **CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only after approval should you invoke the `@distillery` skill.

[STATUS UPDATE 2026-07-01]
Implementation complete. All conditional compilation guards removed. Stubs expanded for MQTT and PsychicHttp. Tests natively running successfully and hardware builds successfully.
