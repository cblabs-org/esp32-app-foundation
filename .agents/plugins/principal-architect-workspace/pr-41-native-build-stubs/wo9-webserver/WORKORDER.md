---
name: pr41-wo9
description: Handoff instructions for Work Order 9 — Migrate WebServer module
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Remove all `#ifdef NATIVE_BUILD` / `#ifndef NATIVE_BUILD` guards from the WebServerManager module (~15 guards). Relocate the inline PsychicHttp mock classes from `WebServerManager.hpp` to the stub header `test/stubs/platform/PsychicHttp.h` (if not already done in WO1).

## Strict Boundary Box

- **DO** modify only files in `modules/WebServer/`
- **DO** extend stub headers in `test/stubs/platform/` if needed
- **DO NOT** modify any other module's source files

## Tooling Constraints

- Use `platformio` MCP tools for building and testing

## Line-Item Instructions

- [ ] **`WebServerManager.hpp`** — Remove the large `#ifdef NATIVE_BUILD` block (~L53-140) containing mock `PsychicResponse`, `PsychicRequest`, `PsychicHttpServer`, `PsychicHttpsServer` classes. These are now in the PsychicHttp.h stub. Remove guard around `_httpsServer` member. Include `<PsychicHttp.h>` unconditionally.
- [ ] **`WebServerManager.cpp`** — Remove all guards:
  - Include guards for `<WiFi.h>`, `<PsychicHttp.h>`, portal asset headers
  - `begin()` method: remove guard around HTTPS server setup, cert loading, route registration
  - `#ifdef NATIVE_BUILD` static member definition block
  - All route handler bodies (system info, tabs, portal assets, OTA)
- [ ] **Portal asset headers**: The portal assets (embedded HTML/CSS/JS) may include headers that don't exist in native builds. If so, add minimal stub asset headers that provide empty string constants.
- [ ] **Verify**: `pio test -e native` and `pio run -e esp32s3nano`

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.

## Status Update
WO9 completed successfully. Tests passed natively and on esp32s3nano.
