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

- [ ] **`KeyManager.hpp`** — Remove guards around `<LittleFS.h>` and PsychicHttp forward declarations. Include unconditionally.
- [ ] **`KeyManager.cpp`** — Remove guards around `<LittleFS.h>`, `<Preferences.h>` includes and their `#else` native stub blocks. Remove guard around `registerRoutes()` body.
- [ ] **NVS usage**: The `Preferences` stub is backed by `std::map`. Verify KeyManager's `putBytes()`/`getBytes()` patterns work with the stub.
- [ ] **Verify**: `pio test -e native` and `pio run -e esp32s3nano`

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.
