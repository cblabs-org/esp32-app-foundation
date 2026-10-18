---
name: type-colocation-wo3
description: Handoff instructions for Work Order 3 — LVGL, HomeAssistant, and otaSecrets cleanup
complexity: mechanical
---

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make high-level architectural
> decisions, do not branch, and do not plan. Your task is strictly limited to
> executing the checklist below.

## Objective

1. Move `ILVGLSystemScreens.hpp` from `include/interfaces/` into `modules/LVGLDisplay/`.
2. Move `homeassistant_js.hpp` from `include/portal_assets/` into `modules/HomeAssistant/`.
3. Delete the dead `otaSecrets.hpp` file (Issue #94 — zero consumers, live PAT).
4. Update stale documentation paths in `docs/developer/custom-branding.md` that
   reference old locations from WO2's moves.

Update all `#include` paths to match the convention established in WO1/WO2.

## Strict Boundary Box

- **DO NOT** modify any file outside the files listed in the checklist below.
- **DO NOT** change any logic, types, or interface definitions — this is a pure
  file-move, include-path refactoring, and dead-code deletion.
- **DO NOT** create or delete branches.
- **DO NOT** modify `platformio.ini` or any `library.json` files.
- **DO NOT** touch any files in `modules/ButtonModule/`, `modules/EncoderModule/`,
  or `modules/U8G2Display/`.

## Include Path Convention (established in WO1)

- **External consumers** (outside the owning module): use module-qualified
  paths like `#include "LVGLDisplay/ILVGLSystemScreens.hpp"` — no `modules/` prefix.
- **Internal files** (inside the owning module): use bare local paths like
  `#include "ILVGLSystemScreens.hpp"` since the file is now co-located.

## Tooling Constraints

- Use the `platformio` MCP tools for building and testing:
  - `build_project` for compilation verification
  - `run_tests` for test execution
- Do NOT use raw `pio` CLI commands.

## Line-Item Instructions

### Step 1: Move the files

- [ ] Move `include/interfaces/ILVGLSystemScreens.hpp` → `modules/LVGLDisplay/ILVGLSystemScreens.hpp`
- [ ] Move `include/portal_assets/homeassistant_js.hpp` → `modules/HomeAssistant/homeassistant_js.hpp`

### Step 2: Delete otaSecrets.hpp

- [ ] Delete `include/otaSecrets.hpp` (Issue #94 — dead code, zero `#include` references)

### Step 3: Update the `Module:` path in each moved file's header comment

- [ ] `modules/LVGLDisplay/ILVGLSystemScreens.hpp` — update Module path
- [ ] `modules/HomeAssistant/homeassistant_js.hpp` — update Module path (if it has one)

### Step 4: Update `#include` paths in all consumers

**ILVGLSystemScreens.hpp consumers:**

Internal to LVGLDisplay (change to bare local `"ILVGLSystemScreens.hpp"`):
- [ ] `modules/LVGLDisplay/LVGLDisplayModule.hpp` L62: `"interfaces/ILVGLSystemScreens.hpp"` → `"ILVGLSystemScreens.hpp"`
- [ ] `modules/LVGLDisplay/screens/SLSSystemScreens.hpp` L21: `"interfaces/ILVGLSystemScreens.hpp"` → `"ILVGLSystemScreens.hpp"`

External consumers (change to module-qualified):
- [ ] `test/test_unit/test_lvglDisplay/test_screens.cpp` L19: `"interfaces/ILVGLSystemScreens.hpp"` → `"LVGLDisplay/ILVGLSystemScreens.hpp"`
- [ ] `test/test_unit/test_lvgl_stubs/test_lvgl_stubs.cpp` L17: `"interfaces/ILVGLSystemScreens.hpp"` → `"LVGLDisplay/ILVGLSystemScreens.hpp"`

**homeassistant_js.hpp consumer:**

Internal to HomeAssistant (change to bare local):
- [ ] `modules/HomeAssistant/HomeAssistantModule.cpp` L26: `"portal_assets/homeassistant_js.hpp"` → `"homeassistant_js.hpp"`

### Step 5: Update stale documentation paths

- [ ] `docs/developer/custom-branding.md` — find and update any references to:
  - `include/types/U8G2BootLogo.h` → `modules/U8G2Display/U8G2BootLogo.h`
  - `include/interfaces/IU8G2SystemScreens.hpp` → `modules/U8G2Display/IU8G2SystemScreens.hpp`
  - `include/interfaces/IDisplayBoard.hpp` → `modules/U8G2Display/IDisplayBoard.hpp`
  - `include/interfaces/ILVGLSystemScreens.hpp` → `modules/LVGLDisplay/ILVGLSystemScreens.hpp`
  - Any other stale include paths from WO1/WO2 moves

### Step 6: Verify builds

- [ ] Build `native` environment — must compile cleanly
- [ ] Build `crowpanel-128-rotary` environment — must compile cleanly
- [ ] Run native test suite — all tests must pass

### Step 7: Commit

- [ ] Stage all changes and commit with message:
  `refactor: co-locate ILVGLSystemScreens and homeassistant_js with their modules; delete dead otaSecrets.hpp`

## Verification Requirement

The worker MUST verify that both `native` and `crowpanel-128-rotary` environments
build cleanly and native tests pass before committing.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*,
surface it in your Completion Report below — do not file issues directly. The
Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete — Commit `835e683`

### Changes Made
- Moved `include/interfaces/ILVGLSystemScreens.hpp` → `modules/LVGLDisplay/ILVGLSystemScreens.hpp` and updated header comment.
- Moved `include/portal_assets/homeassistant_js.hpp` → `modules/HomeAssistant/homeassistant_js.hpp` and updated header comment.
- Deleted dead `include/otaSecrets.hpp` file (Issue #94).
- Updated internal include paths in `modules/LVGLDisplay/LVGLDisplayModule.hpp` and `modules/LVGLDisplay/screens/SLSSystemScreens.hpp` to `"ILVGLSystemScreens.hpp"`.
- Updated external include paths in `test/test_unit/test_lvglDisplay/test_screens.cpp` and `test/test_unit/test_lvgl_stubs/test_lvgl_stubs.cpp` to `"LVGLDisplay/ILVGLSystemScreens.hpp"`.
- Updated internal include path in `modules/HomeAssistant/HomeAssistantModule.cpp` to `"homeassistant_js.hpp"`.
- Updated stale documentation paths in `docs/developer/custom-branding.md` and `modules/LVGLDisplay/README.md`.
- Verified native test suite (482/482 passed) and `crowpanel-128-rotary` build (clean).
- Committed changes as commit `835e683`.

### Dead Ends & Warnings
None.

### Out-of-Scope Findings
None.

### Blockers
None.
