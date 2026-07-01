---
name: pr41-wo5
description: Handoff instructions for Work Order 5 — Migrate TimeManager module
complexity: mechanical
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Remove all `#ifdef NATIVE_BUILD` / `#ifndef NATIVE_BUILD` guards from the TimeManager module (~3 guards).

## Strict Boundary Box

- **DO** modify only files in `modules/TimeManager/`
- **DO** extend stub headers in `test/stubs/platform/` if needed
- **DO NOT** modify any other module's source files

## Tooling Constraints

- Use `platformio` MCP tools for building and testing

## Line-Item Instructions

- [x] **`TimeManager.hpp`** — Remove guard around `<PsychicHttp.h>` and Arduino time type includes. Include unconditionally.
- [x] **`TimeManager.cpp`** — Remove guard around `<PsychicHttp.h>` include and `registerRoutes()` body. The PsychicHttp stub provides the types; the route registration body can execute against the mock server (no-op).
- [x] **`TimeConfigGate.hpp/.cpp`** — Check for any guards and remove.
- [x] **Verify**: `pio test -e native` and `pio run -e esp32s3nano`

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.

## Status Update
**Completed:** 2026-07-01
- Removed all `NATIVE_BUILD` guards from `TimeManager.hpp` and `TimeManager.cpp`.
- `TimeConfigGate` had no such guards.
- Expanded native stubs (`esp_sntp.h`, `LittleFS.h`, `Arduino String write`) to support the real `TimeManager` implementation in the native test environment.
- Fixed `test_timeManager` unit tests to advance the real state machine (`tick(0)`).
- Validated with `pio test -e native` and `pio run -e esp32s3nano` via `platformio` MCP tools.
- Output distilled.
- Committed changes to current branch.
