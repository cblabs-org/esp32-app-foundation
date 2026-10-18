# ADR: tick() Wake Request Ordering

## Decision

In `ScreensaverController::tick()`, the `_wakeRequested` check is placed **before** both the `_dimTimeoutMs == 0` and `!_enabled` early-return guards.

## Context

The WO1 checklist specified: "Modify tick(): early-return if `!_enabled`". A naive implementation places the `!_enabled` guard at the top of `tick()`. However, `setEnabled(false)` sets `_wakeRequested = true` when the display is currently dimmed or blanked, so the display can restore immediately.

## Problem

If `!_enabled` returns before checking `_wakeRequested`, the wake is never serviced. The display stays visually stuck in dim/blank state even though the screensaver is logically disabled. The unit tests `DisableWhileDimmed_RequestsWake` and `DisableWhileBlanked_RequestsWake` caught this immediately.

## Resolution

```
tick() order:
  1. Service _wakeRequested (always — even if disabled)
  2. Guard: _dimTimeoutMs == 0 → return
  3. Guard: !_enabled → return
  4. Normal timeout state machine
```

This ensures that disabling the screensaver while active always restores the display, but prevents any new dim/blank transitions when disabled.
