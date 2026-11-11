---
name: mk03-wo2
description: Verify flag-off builds and update documentation for optional modules
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Ensure the framework builds successfully with HomeAssistant disabled (flag-off state). Update the documentation to reflect the new "honest main.cpp" composition pattern for optional modules.

# Strict Boundary Box
- Do not refactor other modules.
- Do not alter the core architecture.

# Tooling Constraints
Use the `platformio` MCP tools for building and testing. Run tests with `run_tests` and build with `build_project`.

# Line-Item Instructions
- [x] In `platformio.ini`, remove the `-D WITH_HOMEASSISTANT` flag from `[env:esp32dev]` and `[env:native]` to restore the default-off state (per MK-6).
- [x] In `platformio.ini`, create a new environment `[env:esp32dev_ha]` that `extends = env:esp32dev` and adds `-D WITH_HOMEASSISTANT` to its `build_flags` (use `${env:esp32dev.build_flags} -D WITH_HOMEASSISTANT`).
- [x] In `platformio.ini`, create a new environment `[env:native_ha]` that `extends = env:native` and adds `-D WITH_HOMEASSISTANT` to its `build_flags` (use `${env:native.build_flags} -D WITH_HOMEASSISTANT`).
- [x] Use `platformio` MCP to build `esp32dev` (flag-off) and `esp32dev_ha` (flag-on). Note the flash size of both to verify HomeAssistant is absent when flag is off.
- [x] Use `platformio` MCP to run tests for `native` (flag-off) and `native_ha` (flag-on). Both must pass.
- [x] In `docs/developer/using-optional-modules.md`, update the documentation to explain the new two-axis optionality model (availability flag + explicit composition). Explain that framework optional modules (like HomeAssistant) are no longer owned by `Application`; they must be instantiated in the user's `main.cpp` and registered via `registerModule()`, guarded by a feature flag (e.g., `WITH_HOMEASSISTANT`). Provide a small code snippet showing `HomeAssistantModule` instantiation in `main.cpp` (and how it gets dependencies like KeyManager and EventBus).
- [x] In `docs/developer/creating-a-module.md`, add a brief note or update existing text to clarify that both custom application modules AND framework optional modules are wired the exact same way (instantiated in `main.cpp` and passed to `app.registerModule()`).

# Verification Requirement
Verify that both flag-on and flag-off builds pass, and all native tests pass in both states.
Before distilling or committing, you MUST ask the user if they are happy. Only after approval should you invoke the `@distillery` skill to package the state and drop control.

## Status Update
- **Status**: ✅ **COMPLETED AND VERIFIED**
- **Date**: 2026-07-29
- **Build & Test Verification**:
  - `esp32dev` (flag-off): Built successfully (1,266,643 bytes flash | 53,748 bytes RAM)
  - `esp32dev_ha` (flag-on): Built successfully (1,317,695 bytes flash | 54,100 bytes RAM)
  - Flash Reduction: ~51 KB saved when flag is off.
  - `native` (flag-off): 467/467 test cases passed.
  - `native_ha` (flag-on): 491/491 test cases passed.
- **Documentation**: Updated `using-optional-modules.md` (two-axis optionality model + `HomeAssistantModule` injection example) and `creating-a-module.md` (unified composition model note).
- **Distillery**: Packaged into `.agents/plugins/principal-architect-workspace/microkernel-refactor/pr-mk03-module-composition/wo2-tests-docs/`.
