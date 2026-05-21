# ADR: Board Expiry Semantics

## Context

Each `IDisplayBoard` has an `isExpired()` method that the carousel engine uses to decide whether to evict the board from the pool. The three WO3 boards needed distinct expiry policies based on their use-cases.

## Decisions

### OtaProgressBoard
- **`isExpired()` returns `_complete && !_failed`**
- Successful OTA updates auto-dismiss so the carousel can return to normal boards.
- Failed OTA updates persist (`_failed = true`) so the user can see the error state and take manual action. The board can be recycled via `reset()`.

### ErrorBoard
- **`isExpired()` returns `_dismissed`**
- Both fatal and non-fatal errors start as not-dismissed (visible).
- The `fatal` flag does NOT affect `isExpired()` — it only controls the "System halted" UI indicator.
- The carousel or parent module must call `dismiss()` explicitly to expire the board.
- `showError()` resets `_dismissed = false`, enabling error cycling (show → dismiss → show new error).

### ScreensaverBoard
- **`isExpired()` returns `!_active`**
- The screensaver starts as expired (not active) by default, so it doesn't interfere with the carousel on startup.
- `activate()` sets `_active = true` (not expired), `deactivate()` sets `_active = false` (expired).
- This means the screensaver board will be evicted from the pool when deactivated, which is the correct behavior since it's shown via the override mechanism (`showBoard()`), not the carousel.

## Rationale

The asymmetric expiry model ensures that:
1. Transient success states clean themselves up automatically.
2. Error/failure states persist for user visibility and require explicit dismissal.
3. The screensaver follows activation lifecycle rather than event-driven expiry.
