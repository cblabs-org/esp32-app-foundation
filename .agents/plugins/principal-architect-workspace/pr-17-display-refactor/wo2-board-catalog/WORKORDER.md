---
name: pr-17-wo2
description: Handoff instructions for Work Order 2 — Create BoardCatalog thin accessor struct
---

# PR-17 / WO2 — Create `BoardCatalog`

You are an expert senior software engineer and execution-focused worker agent. You are NOT
the Principal Architect. Do not make high-level architectural decisions, do not branch, and
do not plan. Your task is strictly limited to executing the checklist below.

---

## Objective

Create a new thin accessor struct `BoardCatalog` in `modules/U8G2Display/BoardCatalog.hpp`
that holds typed **references** to the user-surfaced system boards owned by `U8G2DisplayModule`.
Wire it into the module as a member variable. No external caller API changes are visible in
this WO — the existing `getBootBoard()`, `getWifiWizard()`, etc. methods remain as delegators.

---

## Git Context

You are working on branch **`pr-17-wo2-board-catalog`**. Check it out before making any changes:

```bash
git checkout pr-17-wo2-board-catalog
```

Do NOT commit to any other branch. Do NOT merge. The Principal Architect handles all merges.

---

## Strict Boundary Box

| ✅ You MAY touch | ❌ You MUST NOT touch |
|---|---|
| `modules/U8G2Display/BoardCatalog.hpp` (NEW — header only) | Carousel logic (WO1 scope) |
| `modules/U8G2Display/U8G2DisplayModule.hpp` (add member) | `ScreensaverController` (WO3 scope) |
| `modules/U8G2Display/U8G2DisplayModule.cpp` (wire constructor) | `Application.cpp` or `main.cpp` |
| House style header block | Any sleep/wake or modal logic |

> **Key constraint**: `BoardCatalog` is a **header-only** struct (no `.cpp` needed).
> Boards remain **owned** by `U8G2DisplayModule` as member variables — `BoardCatalog` holds
> only references, not values or pointers. This is a pure accessor layer.

---

## Pre-Work Reading (Mandatory)

Before writing a single line of code, read these files in order:

1. **Design spec §7.8** — `docs/design_specification.md` lines ~749–865  
   Focus on §7.8.1 (sub-object hierarchy) and §7.8.4 (board taxonomy table)
2. **Existing module header** — `modules/U8G2Display/U8G2DisplayModule.hpp`  
   Understand which board members are owned and their types
3. **Board headers** (read each, skim only for the class name and constructor signature):
   - `modules/U8G2Display/boards/BootBoard.hpp`
   - `modules/U8G2Display/boards/WiFiWizardBoard.hpp`
   - `modules/U8G2Display/boards/OTAProgressBoard.hpp`
   - `modules/U8G2Display/boards/ErrorBoard.hpp`
4. **House style reference** — `.agents/skills/house-style-docs/references/cpp-embedded.md`  
   Apply the header template and Doxygen naming conventions

---

## Board Taxonomy — What Goes in `BoardCatalog`

Per the design spec §7.8.4, ONLY user-surfaced boards appear in `display.boards`.
System-managed boards are excluded:

| Board | In `BoardCatalog`? | Reason |
|---|---|---|
| `BootBoard` | ❌ No | System-managed (driven via `reportStatus()`) |
| `ConfiguringStatusBoard` | ❌ No | System-managed (driven via `showBase()`) |
| `ScreensaverBoard` | ❌ No | Display-intrinsic (managed by `ScreensaverController`) |
| `WiFiWizardBoard` | ✅ Yes | Caller-driven (app calls `display.boards.wifiWizard.configure()`) |
| `OTAProgressBoard` | ✅ Yes | Caller-driven (OTA module calls `display.boards.otaProgress.setProgress()`) |
| `ErrorBoard` | ✅ Yes | Caller-driven (any module calls `display.boards.error`) |

---

## Implementation Checklist

### Step 1 — Create `BoardCatalog.hpp`

- [ ] Create `modules/U8G2Display/BoardCatalog.hpp`
- [ ] Apply the full house-style file header block (project attribution, license, module path,
      description, exported API summary)
- [ ] Add `#pragma once`
- [ ] Include the three board headers:
  ```cpp
  #include "boards/WiFiWizardBoard.hpp"
  #include "boards/OTAProgressBoard.hpp"
  #include "boards/ErrorBoard.hpp"
  ```
- [ ] Define `struct BoardCatalog` with three **reference** members:
  ```cpp
  struct BoardCatalog {
      WiFiWizardBoard&  wifiWizard;    ///< WiFi setup wizard screen (CONFIGURING phase)
      OTAProgressBoard& otaProgress;   ///< OTA firmware update progress screen
      ErrorBoard&       error;         ///< Error message display screen

      /**
       * @brief Construct a BoardCatalog bound to the owning module's board instances.
       *
       * All three parameters are non-const lvalue references. The BoardCatalog does
       * not own these objects — they remain owned by U8G2DisplayModule.
       *
       * @param wifiWizard   Reference to the module's owned WiFiWizardBoard.
       * @param otaProgress  Reference to the module's owned OTAProgressBoard.
       * @param error        Reference to the module's owned ErrorBoard.
       */
      BoardCatalog(WiFiWizardBoard& wifiWizard,
                   OTAProgressBoard& otaProgress,
                   ErrorBoard& error)
          : wifiWizard(wifiWizard)
          , otaProgress(otaProgress)
          , error(error) {}
  };
  ```

  > **Important**: Because `BoardCatalog` holds references, it is **not default-constructible**
  > and **not copy-assignable**. This is intentional — it must be initialized in the
  > `U8G2DisplayModule` member-initializer list. Do not attempt to add a default constructor.

### Step 2 — Modify `U8G2DisplayModule.hpp`

- [ ] Add `#include "BoardCatalog.hpp"` alongside the other local board includes
- [ ] Add `BoardCatalog _boards;` as a **private** member variable in the member variables
      section, with a `///<` comment:
  ```cpp
  BoardCatalog _boards{_wifiWizard, _otaProgress, _error};  ///< Typed references to user-surfaced boards
  ```
  
  > **Initialization order constraint**: `BoardCatalog _boards` must be declared AFTER
  > `_wifiWizard`, `_otaProgress`, and `_error` in the class body. C++ initializes members
  > in declaration order, so `_boards` must come last in the member variables section.

- [ ] Update the file-header "Exported API" block to mention `_boards` (BoardCatalog member)
- [ ] Do NOT remove the existing `getWifiWizard()`, `getOtaProgress()`, `getError()` methods yet.
      They remain as delegators — their removal is WO4's job.

### Step 3 — Verify `U8G2DisplayModule.cpp` needs no changes

- [ ] Confirm that no `.cpp` changes are required — the `BoardCatalog` is initialized purely
      via the member-initializer pattern in the class body (brace-initialization of the member).
      If the module's constructor has an explicit initializer list that would conflict, resolve
      it there instead, but do not add unnecessary code.

---

## Verification Requirement

After implementation, run both build targets and confirm zero errors:

```bash
# Native unit test build
pio run -e native

# Hardware build
pio run -e esp32dev
```

Confirm `BoardCatalog` is accessible from `U8G2DisplayModule` by checking the symbol:

```bash
git grep -n "BoardCatalog" modules/U8G2Display/
```

Expected: appears in `BoardCatalog.hpp` (definition) and `U8G2DisplayModule.hpp` (include + member).

---

## Completion Protocol

1. Verify both builds pass
2. Run any existing display-related unit tests: `pio test -e native`
3. **STOP. Send a message back to the Principal Architect** with:
   - Build pass confirmation (native + hardware)
   - Test results
   - Any deviations from the checklist (with justification)
   - A request for approval before wrapping up

> Do NOT distill, do NOT commit, do NOT merge until the Principal Architect explicitly approves.

Once the Principal Architect approves:
- Invoke the `@distillery` skill, passing
  `.agents/skills/principal-architect-workspace/pr-17-display-refactor/wo2-board-catalog/`
  as the target directory
- Append a status update block to the bottom of this WORKORDER.md
- Terminate your session

---

## Status

_(To be filled in by worker agent on completion)_
