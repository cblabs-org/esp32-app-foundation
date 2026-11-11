---
name: encoder-recovery
description: Recovery workorder — cherry-pick orphaned EncoderModule commits onto current main
complexity: standard
---

# Encoder Recovery — Cherry-Pick Orphaned PR #56 onto Main

You are an expert senior software engineer and execution-focused worker agent. **DO NOT trigger or act as the principal-architect.**

## Objective

The EncoderModule (PR #56) was merged into `input-module/wo3-tests-docs` 4½ hours AFTER that branch had already been merged to main via PR #53. The encoder code is orphaned — it exists on `origin/input-module/wo3-tests-docs` but never reached `main`. Your task is to bring it forward onto current main.

## Context

- **Branch:** `encoder-recovery` (already checked out, based on current `main`)
- **Orphaned commits** (in order, on `origin/input-module/wo3-tests-docs`):
  - `19e8cb8` feat(encoder): WO1 — EncoderModule framework core
  - `b430795` feat(encoder): WO2 — board wiring, build flags, sample app integration
  - `c07c561` test+docs(encoder): WO3 — unit tests (25 cases), README, PCNT stub
  - `602718a` fix(encoder): address PR #56 review findings
  - `b43090b` fix(encoder): add multi-encoder support respecting SOC_PCNT_UNITS_PER_GROUP
- **Skip:** `f1c7007` (PA workspace churn) and `9885e02` (GitHub merge commit)
- **45 commits of drift** on main since the orphan point — conflicts expected

## Strict Boundary Box

- Do NOT modify any files EXCEPT those needed for the encoder integration
- Do NOT refactor or redesign the EncoderModule — bring it forward as-is with minimal conflict resolution
- Do NOT touch U8G2Display, LVGLDisplay, or other unrelated modules
- Do NOT attempt to flash — hardware not attached
- Do NOT create new branches beyond the one already checked out
- Do NOT modify `.agents/plugins/principal-architect-workspace/` files

## Tooling Constraints

- Read the `pio-manager` skill (`.agents/skills/pio-manager/SKILL.md`) before any PlatformIO operations
- Use the `platformio` MCP server for builds: `build_project` and `run_tests`

## Conflict Resolution Guide

The encoder code itself (`modules/EncoderModule/`, `include/EncoderConfig.h`) is **net-new** — no conflicts. Conflicts will be in integration touchpoints:

### `platformio.ini`
- The encoder adds `ESP32Encoder` to `lib_deps` and `EncoderModule` to `lib_ignore` on native
- Main has been heavily modified (microkernel, hardware-config, LVGL envs)
- **Resolution:** manually apply the encoder's changes to the current `platformio.ini` rather than fighting merge conflicts

### `src/main.cpp`
- The encoder adds `#include "EncoderModule.hpp"` and `registerModule()` calls
- Main's `main.cpp` was restructured by the microkernel refactor
- **Resolution:** manually add the encoder includes and registration under `#ifdef WITH_ENCODER`

### `hardware/*/board.h`
- The encoder adds `BoardEncoder` descriptors to board headers
- PR-51 rewrote all `board.h` files with the new `board.h` contract
- **Resolution:** add the encoder descriptors to the current `board.h` format — check `hardware/crowpanel-128/board.h` and `hardware/esp32s3nano/board.h` for the current pattern, then add `WITH_ENCODER` sections

### `test/stubs/platform/`
- Encoder adds PCNT stubs — likely clean since the stub directory has grown but PCNT is new
- Check for any conflicts with recent stub additions

## Checklist

### 1. Cherry-pick with conflict resolution

Try cherry-picking each commit in order. For each conflict:
- Read BOTH versions carefully
- Apply the encoder's intent to the current codebase structure
- Do NOT lose any existing code that was added after the orphan point

```
git cherry-pick 19e8cb8   # WO1 core
git cherry-pick b430795   # WO2 board wiring
git cherry-pick c07c561   # WO3 tests+docs
git cherry-pick 602718a   # review findings
git cherry-pick b43090b   # multi-encoder fix
```

If conflicts are too tangled for clean cherry-picks, fall back to:
- Manually copying `modules/EncoderModule/` and `include/EncoderConfig.h` from the orphaned branch
- Manually applying the integration changes to current `platformio.ini`, `main.cpp`, and `board.h` files
- Squashing into a single recovery commit

### 2. Build verification

- [ ] `esp32dev` builds
- [ ] `esp32s3nano` builds  
- [ ] `crowpanel-128-rotary` builds
- [ ] `native` tests pass (including the 25 encoder tests)

### 3. Commit

If squashed: `feat(encoder): recover EncoderModule from orphaned PR #56 branch`

## Verification Requirement

After all builds pass, **STOP and send a message back to the PA** with build results. Do NOT distill.

## Provenance

Recovery of PR #56 (EncoderModule), originally part of PR-52 InputModule project. Lost-merge discovered 2026-08-05. User approved recovery.
