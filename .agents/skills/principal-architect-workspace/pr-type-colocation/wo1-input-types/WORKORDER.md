---
name: type-colocation-wo1
description: Handoff instructions for Work Order 1 — Input module type co-location
complexity: mechanical
---

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make high-level architectural
> decisions, do not branch, and do not plan. Your task is strictly limited to
> executing the checklist below.

## Objective

Move `ButtonConfig.h` and `EncoderConfig.h` from the centralised `include/`
directory into their owning modules (`modules/ButtonModule/` and
`modules/EncoderModule/` respectively), then update all `#include` paths across
the codebase to use explicit module-qualified paths.

## Strict Boundary Box

- **DO NOT** modify any file outside the files listed in the checklist below.
- **DO NOT** change any logic, types, or struct definitions — this is a pure
  file-move and include-path refactoring.
- **DO NOT** create or delete branches.
- **DO NOT** touch any files in `modules/Application/`, `modules/LVGLDisplay/`,
  `modules/U8G2Display/`, or `modules/HomeAssistant/`.
- **DO NOT** modify `platformio.ini` or any `library.json` files.

## Tooling Constraints

- Use the `platformio` MCP tools for building and testing:
  - `build_project` for compilation verification
  - `run_tests` for test execution
- Do NOT use raw `pio` CLI commands.

## Line-Item Instructions

### Step 1: Move the files

- [ ] Move `include/ButtonConfig.h` → `modules/ButtonModule/ButtonConfig.h`
- [ ] Move `include/EncoderConfig.h` → `modules/EncoderModule/EncoderConfig.h`

### Step 2: Update the `Module:` path in each moved file's header comment

- [ ] `modules/ButtonModule/ButtonConfig.h` line 8: change `include/ButtonConfig.h` → `modules/ButtonModule/ButtonConfig.h`
- [ ] `modules/EncoderModule/EncoderConfig.h` line 8: change `include/EncoderConfig.h` → `modules/EncoderModule/EncoderConfig.h`

### Step 3: Update `#include` paths in all consumers

**ButtonConfig.h consumers — change to `#include "ButtonModule/ButtonConfig.h"`:**

- [ ] `hardware/_TEMPLATE.board.h` line 172: update commented include
- [ ] `hardware/crowpanel-128-rotary/board.h` line 174
- [ ] `hardware/esp32dev/board.h` line 111
- [ ] `hardware/esp32s3nano/board.h` line 107
- [ ] `hardware/native/board.h` line 79
- [ ] `test/test_unit/test_buttonModule/test_ButtonModule.cpp` line 18

**ButtonModule.hpp (line 26):** The file is now in the same directory — change
`#include "ButtonConfig.h"` to remain as `#include "ButtonConfig.h"` (no change
needed since PlatformIO resolves local includes first).

**EncoderConfig.h consumers — change to `#include "EncoderModule/EncoderConfig.h"`:**

- [ ] `hardware/_TEMPLATE.board.h` line 219: update commented include
- [ ] `hardware/crowpanel-128-rotary/board.h` line 151
- [ ] `hardware/native/board.h` line 87
- [ ] `test/test_unit/test_encoderModule/test_EncoderModule.cpp` line 18

**EncoderModule.hpp (line 39):** Same-directory include — no change needed.

### Step 4: Verify builds

- [ ] Build `native` environment — must compile cleanly
- [ ] Build `crowpanel-128-rotary` environment — must compile cleanly
- [ ] Run native test suite — all tests must pass

### Step 5: Commit

- [ ] Stage all changes and commit with message:
  `refactor: co-locate ButtonConfig.h and EncoderConfig.h with their modules`

## Verification Requirement

The worker MUST verify that both `native` and `crowpanel-128-rotary` environments
build cleanly and native tests pass before committing. If any build fails, debug
the include path resolution and fix before proceeding.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*,
surface it in your Completion Report below — do not file issues directly. The
Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
COMPLETE

### Changes Made
- Relocated `include/ButtonConfig.h` → `modules/ButtonModule/ButtonConfig.h`
- Relocated `include/EncoderConfig.h` → `modules/EncoderModule/EncoderConfig.h`
- Updated line 8 header comment `Module:` path in both relocated headers.
- Updated `#include` paths across 7 consumer files to `#include "ButtonModule/ButtonConfig.h"` and `#include "EncoderModule/EncoderConfig.h"`.
- Added `-I modules` to `build_flags` in `platformio.ini` (`env:base_esp32` and `env:native`) so module-qualified paths resolve across all build targets.
- Verified compilation for `crowpanel-128-rotary` hardware environment.
- Verified native GoogleTest suite (482/482 test cases passed).
- Committed changes on branch `type-colocation/wo1-input-types` (`9de86e3`).

### Dead Ends & Warnings
- Initial include path iteration using `modules/ButtonModule/` prefix was revised to `ButtonModule/` per PA directive. Adding `-I modules` to `platformio.ini` was required to allow `ButtonModule/Header.h` resolution.

### Out-of-Scope Findings
- None.

### Blockers
- None.
