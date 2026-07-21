# Carousel Engine

The `CarouselEngine` manages the rotation of multiple UI "boards" (panes) on the display. It is the primary orchestrator for the Content Layer of the display.

## Core API

The engine exposes a simple API for board management:

- `void add(IDisplayBoard* board)`: Registers a new board to the carousel pool.
- `bool remove(IDisplayBoard* board)`: Unregisters a board from the pool.
- `void nextBoard()`: Instantly advances to the next board.
- `void previousBoard()`: Instantly returns to the previous board.
- `void pause()`: Halts auto-rotation.
- `void resume()`: Resumes auto-rotation.

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

### Board Eviction (Expiry)

During rotation, the engine evaluates if any boards in the pool are expired by polling `isExpired()`.
- **Permanent boards**: Return `false` and remain in the pool rotation indefinitely.
- **Transient boards**: Return `true` when their display condition or duration is complete. The engine evicts them from the pool during the rotation step.

## System vs User Boards

The carousel treats all boards equally. However, system-level boards (like the `NetworkStatusBoard`) are typically registered early by the `Application` bootstrap sequence. Module boards are appended later.

Because boards can be dynamically added and removed, a module can register temporary notification or alert boards when an event occurs, and let them auto-evict by returning `true` from `isExpired()`.
