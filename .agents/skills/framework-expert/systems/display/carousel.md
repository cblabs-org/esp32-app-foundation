# Carousel Engine

The `CarouselEngine` manages the rotation of multiple UI "boards" (panes) on the display. It is the primary orchestrator for the Content Layer of the display.

## Core API

The engine exposes a simple API for board management:

- `void add(IDisplayBoard* board)`: Registers a new board to the carousel pool.
- `bool remove(IDisplayBoard* board)`: Unregisters a board from the pool.
- `void show(IDisplayBoard* board, unsigned long currentMs)`: Force-shows a specific board and resets the rotation timer. Fires lifecycle hooks.
- `void nextBoard(unsigned long currentMs)`: Advances to the next board. Bypasses `canAdvance()` hold — intended for user-initiated navigation.
- `void previousBoard(unsigned long currentMs)`: Returns to the previous board. Bypasses `canAdvance()` hold — intended for user-initiated navigation.
- `void suspend()`: Halts auto-rotation (timer frozen).
- `void resume()`: Resumes auto-rotation.
- `void setMaxHoldMs(unsigned long ms)`: Sets the bounded ceiling for `canAdvance()` defers (default 30 000 ms).
- `unsigned long getMaxHoldMs() const`: Gets the bounded max-hold ceiling.

## Board Pool & Registration

Modules that implement [`IDisplayBoard`](../../interfaces/display-board.md) can inject themselves into the carousel. This is typically done during the module's `begin()` phase.

```cpp
void MyModule::begin() {
    // Register with the display's carousel engine
    app.getDisplay().carousel.add(&_myBoard);
}
```

The pool stores pointers to the boards. The engine iterates over this pool sequentially.

## Rotation Timing

Auto-rotation is driven by the framework's main loop calling `tick(currentMs)` on the `CarouselEngine`.

1. **Rotation Interval**: The carousel rotates between boards at a fixed interval, which can be customized via `setInterval(unsigned long ms)`. The default interval is 8 seconds.
2. **Tick Evaluation**: On every tick, the engine evaluates whether the rotation interval has elapsed.
3. **Transition**: When the interval elapses, the engine calls `onDeactivate()` on the active board, advances to the next board, calls `onActivate()` on the newly active board, and resets the rotation timer.

### Bounded Hold: canAdvance()

Once the rotation interval elapses, the engine does not rotate unconditionally — it first asks the active board's [`canAdvance()`](../../interfaces/display-board.md#pacing-canadvance):

- If `canAdvance()` returns `true` (the default, for boards that don't override it), rotation proceeds exactly as before.
- If it returns `false`, rotation is **deferred**. The engine re-queries `canAdvance()` on every subsequent tick and rotates as soon as it returns `true`.
- The hold is **bounded**: once the elapsed time since the last rotation exceeds `getInterval() + getMaxHoldMs()`, the engine forces rotation anyway and logs a warning. This guarantees a board with a bug (or a pathologically long-running animation) cannot freeze the display permanently.
- Eviction is unaffected by a hold — expired boards are still collected every tick, hold or no hold.

This is per-board and pull-based: the engine asks, the board answers. It does not require a board to reach into the carousel (unlike `suspend()`/`resume()`, which pause the whole pool globally).

### Board Eviction (Expiry)

During rotation, the engine evaluates if any boards in the pool are expired by polling `isExpired()`.
- **Permanent boards**: Return `false` and remain in the pool rotation indefinitely.
- **Transient boards**: Return `true` when their display condition or duration is complete. The engine evicts them from the pool during the rotation step.

## System vs User Boards

The carousel treats all boards equally. However, system-level boards (like the `NetworkStatusBoard`) are typically registered early by the `Application` bootstrap sequence. Module boards are appended later.

Because boards can be dynamically added and removed, a module can register temporary notification or alert boards when an event occurs, and let them auto-evict by returning `true` from `isExpired()`.
