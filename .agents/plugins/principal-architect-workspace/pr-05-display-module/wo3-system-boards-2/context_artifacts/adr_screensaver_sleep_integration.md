# ADR: Screensaver / Sleep Integration

## Context

The work order specified that `_sleep()` should call `_screensaver.activate()` and `showBoard(&_screensaver)`, and `_wake()` should call `_screensaver.deactivate()`.

## Decision

The screensaver was wired directly into the module's private `_sleep()` and `_wake()` methods:

### `_sleep()` — Order of operations:
1. Set `_sleeping = true`
2. Call `_screensaver.activate()` — initialise bounce position, set `_active = true`
3. Call `showBoard(&_screensaver)` — register as override board for rendering
4. Call `_u8g2.setPowerSave(1)` (hardware guard)

### `_wake()` — Order of operations:
1. Set `_sleeping = false`
2. Call `_screensaver.deactivate()` — set `_active = false`
3. Call `_u8g2.setPowerSave(0)` and restore contrast (hardware guard)

## Important Note for WO4

The screensaver is shown via the `showBoard()` override mechanism (sets `_activeBoardIndex = -1` and `_overrideBoard = &_screensaver`). When `_wake()` is called, `resetActivity()` is invoked within `showBoard()` during sleep entry, but wake does NOT call `showBoard()` to restore the previous board. The `tick()` method's early-exit on `_sleeping` means the carousel resumes from wherever `_activeBoardIndex` was before sleep. This should be verified in WO4 integration testing.

## No Dead-Ends

This integration was clean — no failed approaches. The pattern matches `SleepingBoard` from `departures-board` closely but simplified (no `showClock` toggle, no `dimmedBrightness`, no `oledOff` flag — those can be added in future WOs if needed).
