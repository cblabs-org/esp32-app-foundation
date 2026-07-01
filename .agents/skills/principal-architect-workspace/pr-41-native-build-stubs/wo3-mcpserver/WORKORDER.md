---
name: pr41-wo3
description: Handoff instructions for Work Order 3 — Migrate MCPServer module
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Remove all `#ifdef NATIVE_BUILD` / `#ifndef NATIVE_BUILD` guards from the MCPServer module. Relocate the inline `MockSerial` and `Print` class stubs from `MCPServer.hpp` into the stub header library.

## Strict Boundary Box

- **DO** modify only files in `modules/MCPServer/`
- **DO** extend stub headers in `test/stubs/platform/` if needed
- **DO NOT** modify any other module's source files

## Tooling Constraints

- Use `platformio` MCP tools for building and testing

## Line-Item Instructions

- [ ] **`MCPServer.hpp`** — Remove the large `#ifdef NATIVE_BUILD` block (~L49-113) that defines `Print`, `String`, `MockSerial` stubs. These should already be available from the `Arduino.h` stub (Print, String) and `Serial` global. Remove the `#ifndef NATIVE_BUILD` guard around `processLineForTest()` declaration — make it unconditionally available.
- [ ] **`MCPServer.cpp`** — Remove the `#ifdef NATIVE_BUILD` block that defines `MockSerial Serial` instance (now provided by Arduino.h stub). Remove guards around `Serial.available()` / `Serial.read()` in `tick()`. Remove guard around `registerRoutes()` body.
- [ ] **`MCPBuiltinTools.cpp`** — Remove all guards: include guards for WiFi/OTA/system headers, `BUILD_VERSION` stub, system_info tool (ESP chip model, heap, millis, WiFi IP), `esp_log_level_set()`, LittleFS filesystem listing.
- [ ] **Verify**: `pio test -e native` — all MCP tests pass
- [ ] **Verify**: `pio run -e esp32s3nano` — hardware build succeeds

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.

## Status Update
**COMPLETED:** All `NATIVE_BUILD` guards removed. All stubs added. `pio test -e native` and `pio run -e esp32s3nano` passing perfectly. Changes committed to branch and output packaged via `@distillery`.
