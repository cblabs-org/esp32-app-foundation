---
name: pr-43-wo2
description: Rewrite display system docs to match the real IDisplayBoard API
complexity: standard
---

# WO2 — Rewrite Display System Docs

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Rewrite `custom-boards.md` and `carousel.md` in the framework-expert skill so they match the real `IDisplayBoard` interface. The reference source of truth is `include/interfaces/IDisplayBoard.hpp` and `interfaces/display-board.md` (already correct). Closes GitHub Issue #19.

## Strict Boundary Box

- **DO NOT** modify any C++ source or header files.
- **DO NOT** modify `interfaces/display-board.md` — it is already correct and is your reference.
- **DO NOT** change any files outside the two listed below.

## Reference: The Real API

From `include/interfaces/IDisplayBoard.hpp`:

| Method | Type | Purpose |
|---|---|---|
| `render(U8G2&)` | pure virtual | Draw a full-screen frame |
| `isExpired() const` | pure virtual | Return true to be evicted from carousel |
| `getBoardName() const` | pure virtual | Human-readable name for logging |
| `onActivate()` | optional (no-op) | Called when board becomes active |
| `onDeactivate()` | optional (no-op) | Called when board goes off-screen |
| `tick(uint32_t currentMillis)` | optional (no-op) | Per-frame timing hook |

Registration: `carousel.add(&board)` (from `CarouselEngine.hpp:68`). No `addBoard()`.

Lifecycle: `add()` → [wait for rotation] → `onActivate()` → `tick()`/`render()` loop → `onDeactivate()` → `isExpired()` → remove.

## Checklist

### `.agents/skills/framework-expert/systems/display/custom-boards.md`

- [ ] **Rewrite lifecycle section (lines 9-13):** Replace the fabricated `shouldDisplay()` → `onShow()` → `draw()` → `onHide()` lifecycle with the real one documented above.
- [ ] **Rewrite TelemetryBoard example (lines 19-61):** Replace all wrong method signatures:
  - `getName()` → `getBoardName()`
  - `shouldDisplay()` → remove entirely
  - `getDisplayDurationMs()` → remove entirely
  - `onShow()` → `onActivate()`
  - `onHide()` → `onDeactivate()`
  - `draw()` → `render()`
  - **Add** `isExpired() const` override (pure virtual — return `false` for permanent board)
  - **Add** `tick(uint32_t currentMillis)` override (optional, show as example)
  - Fix include to `#include "interfaces/IDisplayBoard.hpp"` (not `framework/interfaces/IDisplayBoard.h`)
- [ ] **Fix registration section (lines 69-76):** Change `app->getDisplay()->getCarousel()->addBoard(this)` to the real pattern. Reference `interfaces/display-board.md:55` for the correct call.
- [ ] **Fix draw advice at bottom (line 71):** Change `draw()` to `render()`.

### `.agents/skills/framework-expert/systems/display/carousel.md`

- [ ] **Fix API section (lines 9-10):** `addBoard()` → `add()`, `removeBoard()` → `remove()`.
- [ ] **Fix registration snippet (lines 20-24):** Same fix as custom-boards.md.
- [ ] **Rewrite rotation section (lines 27-45):** Remove all references to `shouldDisplay()` and `getDisplayDurationMs()`. Replace with the actual model: fixed interval via `setInterval()`, board eviction via `isExpired()`, transitions via `onActivate()`/`onDeactivate()`.

## Verification

- [ ] Diff each changed file against `interfaces/display-board.md` — every method name, signature, and lifecycle step must match.
- [ ] No method name from the old API (`draw`, `getName`, `shouldDisplay`, `getDisplayDurationMs`, `onShow`, `onHide`, `addBoard`) appears anywhere in the updated files.
