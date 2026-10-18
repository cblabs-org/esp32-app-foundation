# ADR: Screensaver Activation via pushModal()

## Context

The original `_sleep()` method powered off the OLED via `setPowerSave(1)`, set `_sleeping = true`, and published a `display.sleep` event. The screensaver (a fully implemented bouncing clock board) was never activated — it was dead code.

## Decision

Replace hard OLED power-off with screensaver activation using `pushModal(&_screensaver)`.

## Rationale

### Why `pushModal()` instead of `showBoard()`?

`showBoard()` searches the pool for the board. Since `_screensaver` is a system board that is NOT registered in the pool, `showBoard()` would not find it — `_activeBoardIndex` wouldn't update, and `_renderCurrentBoard()` would continue rendering `_boards[_activeBoardIndex]` instead of the screensaver.

`pushModal()` places the screensaver on the modal layer, which `_renderCurrentBoard()` checks first (line ~1082: `if (_modal != nullptr)`). This guarantees the screensaver renders on top of everything.

### Why `_sleeping` is NOT set to `true`?

The tick loop must continue running to:
1. Call `_modal->tick()` to animate the bouncing clock
2. Call `_renderCurrentBoard()` to render each frame
3. Check `_modal->isExpired()` for auto-clear after `deactivate()`

If `_sleeping = true`, the tick loop would short-circuit before reaching these calls.

### Why no `display.sleep` event?

The display isn't actually sleeping — it's showing the screensaver. The OLED stays powered on. Publishing `display.sleep` would mislead subscribers into thinking the display is off.

### Dismissal Flow

`resetActivity()` checks `if (_modal == &_screensaver)` and calls `_screensaver.deactivate()`. This makes `isExpired()` return `true`, and the modal auto-clears on the next tick cycle. The `_wake()` method is left unchanged as a safety net (reachable via direct calls, but `resetActivity()` no longer triggers it since `_sleeping` is never set).

### Re-entry Prevention

After `_sleep()` pushes the screensaver modal, `_checkSleep()` won't fire again because with `_modal != nullptr`, the tick loop routes to `_checkDim()` instead of `_checkSleep()`.
