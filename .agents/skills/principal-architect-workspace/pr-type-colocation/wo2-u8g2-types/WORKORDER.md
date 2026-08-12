---
name: type-colocation-wo2
description: Handoff instructions for Work Order 2 — U8G2 display module type co-location
complexity: mechanical
---

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make high-level architectural
> decisions, do not branch, and do not plan. Your task is strictly limited to
> executing the checklist below.

## Objective

Move three U8G2-specific headers from the centralised `include/` directory into
`modules/U8G2Display/`, then update all `#include` paths. These headers are
display-technology-specific types and interfaces that belong with the module
that defines them.

## Strict Boundary Box

- **DO NOT** modify any file outside the files listed in the checklist below.
- **DO NOT** change any logic, types, or interface definitions — this is a pure
  file-move and include-path refactoring.
- **DO NOT** create or delete branches.
- **DO NOT** modify `platformio.ini` or any `library.json` files. (WO1 already
  added `-I modules` to the build flags — module-qualified includes resolve.)
- **DO NOT** touch any files in `modules/ButtonModule/`, `modules/EncoderModule/`,
  `modules/LVGLDisplay/`, or `modules/HomeAssistant/`.

## Include Path Convention (established in WO1)

- **External consumers** (outside `modules/U8G2Display/`): use module-qualified
  paths like `#include "U8G2Display/IDisplayBoard.hpp"` — no `modules/` prefix.
- **Internal files** (inside `modules/U8G2Display/`): use bare local paths like
  `#include "IDisplayBoard.hpp"` since the file is now co-located.

## Tooling Constraints

- Use the `platformio` MCP tools for building and testing:
  - `build_project` for compilation verification
  - `run_tests` for test execution
- Do NOT use raw `pio` CLI commands.

## Line-Item Instructions

### Step 1: Move the files

- [ ] Move `include/types/U8G2BootLogo.h` → `modules/U8G2Display/U8G2BootLogo.h`
- [ ] Move `include/interfaces/IU8G2SystemScreens.hpp` → `modules/U8G2Display/IU8G2SystemScreens.hpp`
- [ ] Move `include/interfaces/IDisplayBoard.hpp` → `modules/U8G2Display/IDisplayBoard.hpp`

### Step 2: Update the `Module:` path in each moved file's header comment

- [ ] `modules/U8G2Display/U8G2BootLogo.h` — update Module path
- [ ] `modules/U8G2Display/IU8G2SystemScreens.hpp` — update Module path
- [ ] `modules/U8G2Display/IDisplayBoard.hpp` — update Module path

### Step 3: Update `#include` paths in all consumers

**U8G2BootLogo.h consumers:**

- [ ] `hardware/_TEMPLATE.board.h` L200: change `<types/U8G2BootLogo.h>` (commented) → `"U8G2Display/U8G2BootLogo.h"`
- [ ] `modules/U8G2Display/U8G2DisplayModule.hpp` L110: change `<types/U8G2BootLogo.h>` → `"U8G2BootLogo.h"` (now local)
- [ ] `modules/U8G2Display/boards/BootBoard.hpp` L30: change `<types/U8G2BootLogo.h>` → `"U8G2BootLogo.h"` (now local)

**IU8G2SystemScreens.hpp consumers:**

- [ ] `modules/U8G2Display/U8G2DisplayModule.hpp` L111: change `<interfaces/IU8G2SystemScreens.hpp>` → `"IU8G2SystemScreens.hpp"` (now local)
- [ ] `test/test_unit/test_u8g2Display/test_eventbus_integration.cpp` L212: change `"interfaces/IU8G2SystemScreens.hpp"` → `"U8G2Display/IU8G2SystemScreens.hpp"`
- [ ] `test/test_unit/test_u8g2Display/test_module.cpp` L18: change `"interfaces/IU8G2SystemScreens.hpp"` → `"U8G2Display/IU8G2SystemScreens.hpp"`

**IDisplayBoard.hpp consumers:**

Internal to U8G2Display (change to bare local `"IDisplayBoard.hpp"`):
- [ ] `modules/U8G2Display/U8G2DisplayModule.hpp` L95: `"interfaces/IDisplayBoard.hpp"` → `"IDisplayBoard.hpp"`
- [ ] `modules/U8G2Display/CarouselEngine.hpp` L62: `"interfaces/IDisplayBoard.hpp"` → `"IDisplayBoard.hpp"`
- [ ] `modules/U8G2Display/boards/BootBoard.hpp` L26: `"interfaces/IDisplayBoard.hpp"` → `"IDisplayBoard.hpp"`
- [ ] `modules/U8G2Display/boards/ConfiguringStatusBoard.hpp` L27: `"interfaces/IDisplayBoard.hpp"` → `"IDisplayBoard.hpp"`
- [ ] `modules/U8G2Display/boards/DiagnosticsBoard.hpp` L31: `"interfaces/IDisplayBoard.hpp"` → `"IDisplayBoard.hpp"`
- [ ] `modules/U8G2Display/boards/MessageBoardBase.hpp` L23: `"interfaces/IDisplayBoard.hpp"` → `"IDisplayBoard.hpp"`
- [ ] `modules/U8G2Display/boards/OTAProgressBoard.hpp` L27: `"interfaces/IDisplayBoard.hpp"` → `"IDisplayBoard.hpp"`
- [ ] `modules/U8G2Display/boards/ScreensaverBoard.hpp` L26: `"interfaces/IDisplayBoard.hpp"` → `"IDisplayBoard.hpp"`

Also update the moved IU8G2SystemScreens.hpp's own include of IDisplayBoard:
- [ ] `modules/U8G2Display/IU8G2SystemScreens.hpp` L22: `"interfaces/IDisplayBoard.hpp"` → `"IDisplayBoard.hpp"` (now co-local)

External consumers (change to module-qualified `"U8G2Display/IDisplayBoard.hpp"`):
- [ ] `include/AppFoundation.h` L47: `"interfaces/IDisplayBoard.hpp"` → `"U8G2Display/IDisplayBoard.hpp"`
- [ ] `test/test_unit/test_u8g2Display/test_module.cpp` L19: `"interfaces/IDisplayBoard.hpp"` → `"U8G2Display/IDisplayBoard.hpp"`

### Step 4: Verify builds

- [ ] Build `native` environment — must compile cleanly
- [ ] Build `crowpanel-128-rotary` environment — must compile cleanly
- [ ] Run native test suite — all tests must pass

### Step 5: Commit

- [ ] Stage all changes and commit with message:
  `refactor: co-locate U8G2BootLogo, IU8G2SystemScreens, and IDisplayBoard with U8G2Display module`

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
Complete

### Changes Made
- Moved `include/types/U8G2BootLogo.h` → `modules/U8G2Display/U8G2BootLogo.h`
- Moved `include/interfaces/IU8G2SystemScreens.hpp` → `modules/U8G2Display/IU8G2SystemScreens.hpp`
- Moved `include/interfaces/IDisplayBoard.hpp` → `modules/U8G2Display/IDisplayBoard.hpp`
- Updated header top comments (`Module:` path) in all 3 moved files.
- Refactored `#include` paths across internal `modules/U8G2Display/` headers and boards (`CarouselEngine.hpp`, `U8G2DisplayModule.hpp`, `BootBoard.hpp`, `ConfiguringStatusBoard.hpp`, `DiagnosticsBoard.hpp`, `MessageBoardBase.hpp`, `OTAProgressBoard.hpp`, `ScreensaverBoard.hpp`, `IU8G2SystemScreens.hpp`) to bare local filenames.
- Refactored external `#include` paths (`include/AppFoundation.h`, `hardware/_TEMPLATE.board.h`, `test/test_unit/test_u8g2Display/test_module.cpp`, `test/test_unit/test_u8g2Display/test_eventbus_integration.cpp`) to module-qualified paths (`"U8G2Display/..."`).
- Verified build and test pass: `native` environment passed 482/482 tests; `crowpanel-128-rotary` environment built cleanly.
- Committed changes on branch `type-colocation/wo2-u8g2-types`: `f06119a`.

### Dead Ends & Warnings
- None. All file moves and include updates compiled and passed tests cleanly without side effects.

### Out-of-Scope Findings
- Documentation file `docs/developer/custom-branding.md` (lines 77, 108, 157, 181) contains references to old header paths (`include/types/U8G2BootLogo.h` and `include/interfaces/IU8G2SystemScreens.hpp`). Per Strict Boundary Box rules, external docs were not modified and are surfaced for PA doc updates.

### Blockers
- None.
