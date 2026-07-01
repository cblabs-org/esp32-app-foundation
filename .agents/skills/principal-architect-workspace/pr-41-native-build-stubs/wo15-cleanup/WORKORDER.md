---
name: pr41-wo15
description: Handoff instructions for Work Order 15 — Production code cleanup
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Final cleanup pass after all module migrations are complete. Remove the `NATIVE_BUILD` preprocessor flag, delete obsolete platform shim files, and ensure no stale references remain.

## Strict Boundary Box

- **DO** modify `platformio.ini`
- **DO** modify/delete files in `include/platform/`
- **DO** search and fix any remaining `NATIVE_BUILD` references across the entire codebase
- **DO NOT** introduce new functionality

## Tooling Constraints

- Use `platformio` MCP tools for building and testing

## Line-Item Instructions

### Remove NATIVE_BUILD flag

- [ ] **`platformio.ini`** — Remove `-DNATIVE_BUILD` from `[env:native]` build flags. The stub headers are selected by include-path priority alone; no preprocessor flag should be needed.
- [ ] **Grep sweep**: Run `grep -r "NATIVE_BUILD" --include='*.cpp' --include='*.hpp' --include='*.h' --include='*.ini'` across the entire project. There should be ZERO references remaining in production code. If any remain (e.g., in comments, test files, or documentation), clean them up:
  - **Comments**: Remove or update references to `NATIVE_BUILD` in doc comments
  - **Test files**: If tests use `#if defined(NATIVE_BUILD)` for conditional `main()`, replace with `#if defined(UNIT_TEST)` or remove the guard entirely (tests only run in native env anyway)

### Clean up platform shims

- [ ] **`include/platform/NativeBuildStubs.hpp`** — DELETE this file. The Arduino `String` class is now in the `test/stubs/platform/Arduino.h` stub. Grep for any remaining includes of this file and remove them.
- [ ] **`include/platform/LogShim.h`** — Simplify: remove the `#ifdef NATIVE_BUILD` guard. In native builds, `esp_log.h` resolves to the stub which provides printf-based macros. In hardware builds, it resolves to the real ESP-IDF header. LogShim should just `#include "esp_log.h"` unconditionally.
- [ ] **`include/platform/FreeRTOSMutex.hpp`** — Remove the `#ifdef NATIVE_BUILD` guard. In native builds, the FreeRTOS stub headers provide the mutex types and no-op implementations. The FreeRTOSMutex wrapper should use the same API unconditionally.

### Remove UNIT_TEST flag if unused

- [ ] **Grep for `UNIT_TEST`**: Check if this flag is used anywhere in production code. If not, remove it from `platformio.ini`. If it's used in test files for conditional compilation, keep it.

### Verify

- [ ] `pio test -e native` — all tests pass
- [ ] `pio run -e esp32s3nano` — hardware build succeeds
- [ ] `grep -r "NATIVE_BUILD" .` — zero hits (excluding `.git/`, `test/stubs/`, and agent workspace files)

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.
