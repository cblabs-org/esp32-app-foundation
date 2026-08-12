---
name: type-colocation-wo4-fixups
description: Handoff instructions for Work Order 4 — PR review fixups
complexity: standard
---

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make high-level architectural
> decisions, do not branch, and do not plan. Your task is strictly limited to
> executing the checklist below.

## Objective

Address three findings from PR #95 code review:
1. Revert the `homeassistant_js.hpp` move (it's a build-generated artifact)
2. Remove `IDisplayBoard` from `AppFoundation.h` (technology-coupled interface doesn't belong in the technology-neutral façade)
3. Fix stale include guard in `ILVGLSystemScreens.hpp`

## Strict Boundary Box

- **DO NOT** modify any file outside the files listed in the checklist below.
- **DO NOT** modify `platformio.ini` or any `library.json` files.
- **DO NOT** create or delete branches.
- **DO NOT** touch any files in `modules/ButtonModule/`, `modules/EncoderModule/`,
  `modules/U8G2Display/` (except reading `IDisplayBoard.hpp` to confirm it has `#pragma once`).

## Tooling Constraints

- Use the `platformio` MCP tools for building and testing:
  - `build_project` for compilation verification
  - `run_tests` for test execution
- Do NOT use raw `pio` CLI commands.

## Line-Item Instructions

### Commit 1: Revert homeassistant_js.hpp move

The file `modules/HomeAssistant/homeassistant_js.hpp` is a build-generated
artifact (created by `scripts/portalBuilder.py` from `web/portal/homeassistant.js`).
It was incorrectly committed as tracked source in WO3. Revert to the original
include path.

- [x] Delete `modules/HomeAssistant/homeassistant_js.hpp` (the tracked copy)
- [x] In `modules/HomeAssistant/HomeAssistantModule.cpp`: change
  `#include "homeassistant_js.hpp"` back to `#include "portal_assets/homeassistant_js.hpp"`

Commit message: `fix: revert homeassistant_js.hpp move — build-generated artifact should not be tracked`

### Commit 2: Remove IDisplayBoard from AppFoundation.h

`IDisplayBoard::render(U8G2& u8g2)` takes a concrete U8G2 library type — it's
technology-coupled and doesn't belong in the technology-neutral framework façade.
Every other interface in AppFoundation.h is technology-agnostic.

- [x] In `include/AppFoundation.h`:
  - Remove line 47: `#include "U8G2Display/IDisplayBoard.hpp"`
  - Remove `IDisplayBoard` from the docblock exports list (line 24: `* - IDisplayBoard:     Interface — U8G2 carousel board render contract`)
- [x] In `sample-app/src/WeatherBoard.hpp`:
  - Add `#include "U8G2Display/IDisplayBoard.hpp"` after the existing `#include "AppFoundation.h"` line
  - Update the comment on the AppFoundation include to remove the "provides IDisplayBoard" mention
- [x] Check `template-app/src/` for any file that inherits from `IDisplayBoard` via `AppFoundation.h` and fix similarly. If no file uses `IDisplayBoard`, no change needed.

Commit message: `fix: remove IDisplayBoard from AppFoundation.h — U8G2-specific interface`

### Commit 3: Fix stale include guard

- [x] In `modules/LVGLDisplay/ILVGLSystemScreens.hpp`:
  - Change `#ifndef INCLUDE_INTERFACES_ILVGLSYSTEMSCREENS_HPP_` to `#ifndef LVGLDISPLAY_ILVGLSYSTEMSCREENS_HPP_`
  - Change `#define INCLUDE_INTERFACES_ILVGLSYSTEMSCREENS_HPP_` to `#define LVGLDISPLAY_ILVGLSYSTEMSCREENS_HPP_`
  - Change the closing `#endif // INCLUDE_INTERFACES_ILVGLSYSTEMSCREENS_HPP_` comment to `#endif // LVGLDISPLAY_ILVGLSYSTEMSCREENS_HPP_`

Commit message: `fix: update stale include guard in ILVGLSystemScreens.hpp`

### Step 4: Verify builds

- [x] Build `native` environment — must compile cleanly
- [x] Build `crowpanel-128-rotary` environment — must compile cleanly
- [x] Run native test suite — all tests must pass

## Verification Requirement

The worker MUST verify that both environments build cleanly and native tests
pass after ALL three commits are made (verification at the end is sufficient).

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*,
surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
COMPLETE

### Changes Made
- Reverted tracked build artifact `modules/HomeAssistant/homeassistant_js.hpp` (`git rm`) and updated `#include` in `modules/HomeAssistant/HomeAssistantModule.cpp` back to `"portal_assets/homeassistant_js.hpp"` (Commit `60cedb2b`).
- Removed `IDisplayBoard` docblock line and header include from `include/AppFoundation.h`. Updated `sample-app/src/WeatherBoard.hpp` to explicitly include `"U8G2Display/IDisplayBoard.hpp"` and updated its doc comment. Verified no references in `template-app/src/` needed modification (Commit `b220e1df`).
- Updated stale include guards in `modules/LVGLDisplay/ILVGLSystemScreens.hpp` to `LVGLDISPLAY_ILVGLSYSTEMSCREENS_HPP_` (Commit `56dab650`).

### Dead Ends & Warnings
None.

### Out-of-Scope Findings
None.

### Blockers
None.
