# Graveyard: _overrideBoard → _modal Mapping

## The Problem

The WORKORDER.md was written against an older version of the display module that had an `_overrideBoard` member variable. PR-12 refactored the display to a Base+Modal architecture, replacing `_overrideBoard` with `_modal`.

## What Was Attempted

### Initial Fix 1 Snippet (from WORKORDER.md)
```cpp
// Clear stale override if it's an expired system board
if (_overrideBoard != nullptr && _overrideBoard->isExpired()) {
    _overrideBoard = nullptr;
}
```

This would not compile — `_overrideBoard` doesn't exist.

### Corrected Mapping
```cpp
// Clear stale modal if it's an expired system board
if (_modal != nullptr && _modal->isExpired()) {
    _modal = nullptr;
}
```

The fix concept was correct — the variable name just needed updating. `_modal` serves the same role as the old `_overrideBoard` (a pointer to a board that takes rendering priority over the pool).

## Initial Fix 2 Error: `showBoard()` vs `pushModal()`

The initial implementation of Fix 2 used `showBoard(&_screensaver)` — this was incorrect because `showBoard()` only searches the pool vector. Since `_screensaver` is a system board not in the pool, the `_activeBoardIndex` wouldn't update and the screensaver would never render. The parent architect caught this and directed the correction to `pushModal(&_screensaver)`.

## Pre-existing Test Failures

Two tests in `test_carousel.cpp` (lines 120-142) test `showBoard()` override behavior that assumes calling `showBoard()` with a non-pool board sets it as the active board. This behavior doesn't exist in the Base+Modal architecture — these tests need updating but are outside WO1 scope.
