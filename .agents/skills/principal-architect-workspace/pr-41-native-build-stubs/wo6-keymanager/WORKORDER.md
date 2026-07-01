---
name: pr41-wo6
description: Handoff instructions for Work Order 6 — Migrate KeyManager module
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Remove all `#ifdef NATIVE_BUILD` / `#ifndef NATIVE_BUILD` guards from the KeyManager module (~5 guards). The Preferences and LittleFS stub headers provide NVS and filesystem API surfaces.

## Strict Boundary Box

- **DO** modify only files in `modules/KeyManager/`
- **DO** extend stub headers in `test/stubs/platform/` if needed
- **DO NOT** modify any other module's source files

## Tooling Constraints

- Use `platformio` MCP tools for building and testing

## Line-Item Instructions

- [x] **`KeyManager.hpp`** — Remove guards around `<LittleFS.h>` and PsychicHttp forward declarations. Include unconditionally.
- [x] **`KeyManager.cpp`** — Remove guards around `<LittleFS.h>`, `<Preferences.h>` includes and their `#else` native stub blocks. Remove guard around `registerRoutes()` body.
- [x] **NVS usage**: The `Preferences` stub is backed by `std::map`. Verify KeyManager's `putBytes()`/`getBytes()` patterns work with the stub.
- [x] **Verify**: `pio test -e native` and `pio run -e esp32s3nano`

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.

## Status Update
**COMPLETED:** All `NATIVE_BUILD` guards were successfully removed from `KeyManager.hpp` and `KeyManager.cpp`. Native stubs (`LittleFS.h` and `PsychicHttp.h`) were extended to fully support KeyManager's compilation dependencies natively. Test environments were updated and both `pio test -e native` and `pio run -e esp32s3nano` passed successfully. Changes have been committed and distilled.
