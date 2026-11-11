---
name: pr-17-wo1
description: Handoff instructions for Work Order 1 — Extract CarouselEngine from U8G2DisplayModule
---

# PR-17 / WO1 — Extract `CarouselEngine`

You are an expert senior software engineer and execution-focused worker agent. You are NOT
the Principal Architect. Do not make high-level architectural decisions, do not branch, and
do not plan. Your task is strictly limited to executing the checklist below.

---

## Objective

Extract all carousel pool and rotation logic from `U8G2DisplayModule` into two new files:
`modules/U8G2Display/CarouselEngine.hpp` and `modules/U8G2Display/CarouselEngine.cpp`.
Wire the new engine back into `U8G2DisplayModule` as a member variable so the module
compiles and behaves identically to before — no external API changes are visible in this WO.

---

## Git Context

You are working on branch **`pr-17-wo1-carousel-engine`**. Check it out before making any changes:

```bash
git checkout pr-17-wo1-carousel-engine
```

Do NOT commit to any other branch. Do NOT merge. The Principal Architect handles all merges.

---

## Strict Boundary Box

| ✅ You MAY touch | ❌ You MUST NOT touch |
|---|---|
| `modules/U8G2Display/CarouselEngine.hpp` (NEW) | `U8G2DisplayModule`'s public API surface |
| `modules/U8G2Display/CarouselEngine.cpp` (NEW) | Any board `.hpp` / `.cpp` files |
| `modules/U8G2Display/U8G2DisplayModule.hpp` (add member, delegate) | `Application.cpp` or `main.cpp` |
| `modules/U8G2Display/U8G2DisplayModule.cpp` (delegate carousel calls) | `ScreensaverBoard`, sleep/wake logic |
| House style header blocks | Any WO2 or WO3 scope (BoardCatalog, ScreensaverController) |

> **Key constraint**: After this WO, callers of `U8G2DisplayModule` see NO API differences.
> `addBoard()`, `showBoard()`, `getBoardCount()`, `getActiveBoard()` remain on the root module
> as thin delegators. The sub-object `display.carousel` is NOT yet exposed publicly — that is WO4.

---

## Pre-Work Reading (Mandatory)

Before writing a single line of code, read these files in order:

1. **Design spec §7.8** — `docs/design_specification.md` lines ~749–865  
   Focus on §7.8.1 (sub-object hierarchy) and §7.8.2 (canonical API)
2. **Existing module header** — `modules/U8G2Display/U8G2DisplayModule.hpp`  
   Understand all carousel-related members and methods before extracting them
3. **Existing module implementation** — `modules/U8G2Display/U8G2DisplayModule.cpp`  
   Identify every method body that references carousel state (`_boards`, `_activeBoardIndex`,
   `_lastRotationMs`, `_rotationIntervalMs`, `_advanceCarousel()`)
4. **House style reference** — `.agents/skills/house-style-docs/references/cpp-embedded.md`  
   Apply the header template, Doxygen comments, and naming conventions to all new files

---

## Implementation Checklist

### Step 1 — Create `CarouselEngine.hpp`

- [ ] Create `modules/U8G2Display/CarouselEngine.hpp`
- [ ] Apply the full house-style file header block (project attribution, license, module path,
      description, exported API summary)
- [ ] Add `#pragma once`
- [ ] Include only: `<vector>`, `<cstdint>`, `"interfaces/IDisplayBoard.hpp"`
- [ ] Forward-declare nothing extra (no `Application`, no `U8G2`)
- [ ] Define `class CarouselEngine` with:

  **Public API** (all documented with Doxygen `@brief` + `@param`/`@return`):
  ```cpp
  void add(IDisplayBoard* board);         // Register a board into the pool (pre-lock only)
  void lock();                            // Lock the pool — no further add() calls accepted
  void show(IDisplayBoard* board);        // Force-show a specific pool board (reset rotation timer)
  void suspend();                         // Pause automatic rotation (timer frozen)
  void resume();                          // Resume automatic rotation from current board
  void setInterval(unsigned long ms);     // Set the rotation interval in milliseconds
  void tick(unsigned long currentMs);     // Advance carousel if interval elapsed; call each tick
  IDisplayBoard* currentBoard() const;    // Return pointer to the active board, or nullptr
  int boardCount() const;                 // Return number of boards in the pool
  ```

  **Private members** (all documented with `///<` inline comments):
  ```cpp
  std::vector<IDisplayBoard*> _pool;       ///< Carousel board pool — populated before lock()
  int _activeIndex = 0;                    ///< Index of the currently rendering board
  unsigned long _lastRotationMs = 0;       ///< Timestamp of last carousel rotation
  unsigned long _intervalMs = 8000;        ///< Rotation interval (default 8 seconds)
  bool _locked = false;                    ///< True after lock() — blocks add()
  bool _suspended = false;                 ///< True when automatic rotation is paused
  ```

  **Private method** (documented):
  ```cpp
  void _advance();  // Advance to next non-expired board; removes expired boards from pool
  ```

### Step 2 — Create `CarouselEngine.cpp`

- [ ] Create `modules/U8G2Display/CarouselEngine.cpp`
- [ ] Apply the full house-style file header block
- [ ] Include `"CarouselEngine.hpp"` (relative, no path prefix needed since same dir)
- [ ] Implement all public methods, porting logic verbatim from `U8G2DisplayModule.cpp`:
  - `add()`: Log error if locked, otherwise `_pool.push_back(board)`
  - `lock()`: Set `_locked = true`; log pool size
  - `show()`: Search `_pool` for `board`, set `_activeIndex`, reset `_lastRotationMs`
  - `suspend()`: Set `_suspended = true`
  - `resume()`: Set `_suspended = false`
  - `setInterval()`: Set `_intervalMs`
  - `tick()`: If not suspended and `(currentMs - _lastRotationMs) >= _intervalMs`, call `_advance()`
  - `currentBoard()`: Return `_pool.empty() ? nullptr : _pool[_activeIndex]`
  - `boardCount()`: Return `(int)_pool.size()`
  - `_advance()`: Port the `_advanceCarousel()` logic — remove expired boards, wrap index

  > **Note on logging**: Use `ESP_LOGI` / `ESP_LOGW` with a `TAG` constant
  > `static const char* TAG = "CarouselEngine";`. Guard hardware-only log calls
  > with `#ifndef NATIVE_BUILD` only if the log macros are not available on native.
  > The project uses ESP-IDF's `esp_log.h` — check how other modules in the codebase import it.

### Step 3 — Modify `U8G2DisplayModule.hpp`

- [ ] Add `#include "CarouselEngine.hpp"` near the other local includes
- [ ] Add `CarouselEngine _carousel;` as a **private** member variable (with `///<` comment)
- [ ] Remove the following private member variables (they now live in `CarouselEngine`):
  - `std::vector<IDisplayBoard*> _boards`
  - `int _activeBoardIndex`
  - `unsigned long _lastRotationMs`
  - `unsigned long _rotationIntervalMs`
  - `bool _locked`
- [ ] Update the file-header "Exported API" block to note the new `_carousel` member
- [ ] Do NOT remove or change any public methods yet (`addBoard`, `showBoard`, `getBoardCount`,
      `getActiveBoard`, `setRotationInterval` all remain on the root — they just delegate)

### Step 4 — Modify `U8G2DisplayModule.cpp`

- [ ] Delegate `addBoard(board)` → `_carousel.add(board)`
- [ ] Delegate `showBoard(board)` → `_carousel.show(board)`
- [ ] Delegate `getBoardCount()` → `return _carousel.boardCount()`
- [ ] Delegate `getActiveBoard()` — if modal active, return modal; else return `_carousel.currentBoard()`
- [ ] Delegate `setRotationInterval(ms)` → `_carousel.setInterval(ms)`
- [ ] In `begin()`: replace `_compactAndLock()` / pool-lock code → `_carousel.lock()`
- [ ] In `tick()`: replace `_advanceCarousel()` call → `_carousel.tick(currentMs)`
- [ ] Remove the `_advanceCarousel()` method body (now in `CarouselEngine`)
- [ ] Anywhere `_boards[_activeBoardIndex]` was used for the base carousel board → `_carousel.currentBoard()`
- [ ] Anywhere `_rotationIntervalMs` was read → delegate to `_carousel` (use `_carousel.setInterval()`)
- [ ] Anywhere `_locked` was checked → remove (no longer a member of this class)

---

## Verification Requirement

After implementation, run both build targets and confirm zero errors:

```bash
# Native unit test build
pio run -e native

# Hardware build
pio run -e esp32dev
```

If the project has more build environments, run all of them.

Additionally, confirm with grep that no direct references to the extracted members remain
in the display module files (outside of `CarouselEngine.cpp`):

```bash
git grep -n "_activeBoardIndex\|_lastRotationMs\|_rotationIntervalMs\|_advanceCarousel\|_locked" \
    modules/U8G2Display/U8G2DisplayModule.cpp \
    modules/U8G2Display/U8G2DisplayModule.hpp
```

Expected result: zero hits.

---

## Completion Protocol

1. Verify both builds pass and the grep check returns zero hits
2. Run any existing display-related unit tests: `pio test -e native`
3. **STOP. Send a message back to the Principal Architect** with:
   - Build pass confirmation (native + hardware)
   - Test results
   - Any deviations from the checklist (with justification)
   - A request for approval before wrapping up

> Do NOT distill, do NOT commit, do NOT merge until the Principal Architect explicitly approves.

Once the Principal Architect approves:
- Invoke the `@distillery` skill, passing
  `.agents/plugins/principal-architect-workspace/pr-17-display-refactor/wo1-carousel-engine/`
  as the target directory
- Append a status update block to the bottom of this WORKORDER.md
- Terminate your session

---

## Status

_(To be filled in by worker agent on completion)_
