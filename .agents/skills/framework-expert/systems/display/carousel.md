# Carousel Engine

The `CarouselEngine` manages the rotation of multiple UI "boards" (panes) on the display. It is the primary orchestrator for the Content Layer of the display.

## Core API

The engine exposes a simple API for board management:

- `void addBoard(IDisplayBoard* board)`: Registers a new board to the carousel pool.
- `void removeBoard(IDisplayBoard* board)`: Unregisters a board.
- `void nextBoard()`: Instantly advances to the next board.
- `void previousBoard()`: Instantly returns to the previous board.
- `void pause()`: Halts auto-rotation.
- `void resume()`: Resumes auto-rotation.

## Board Pool & Registration

Modules that implement [`IDisplayBoard`](../../interfaces/display-board.md) can inject themselves into the carousel. This is typically done during the module's `begin()` phase.

```cpp
void MyModule::begin() {
    // Assuming app->getDisplay() returns an interface exposing the carousel
    app->getDisplay()->getCarousel()->addBoard(this);
}
```

The pool stores pointers to the boards. The engine iterates over this pool sequentially. If a board returns `false` from `board->shouldDisplay()`, the engine skips it and immediately queries the next board.

## Rotation Timing

Auto-rotation is driven by the framework's main `tick()` loop. The `CarouselEngine` tracks the active time of the current board.

1. **Duration Request**: When a board becomes active, the engine queries `board->getDisplayDurationMs()`.
2. **Tick Evaluation**: On every tick, the engine compares the elapsed time against the requested duration.
3. **Transition**: If the duration is exceeded, the engine calls `board->onHide()` on the active board, selects the next valid board, calls `nextBoard->onShow()`, and resets the timer.

### Indefinite Display

If a board returns `0` from `getDisplayDurationMs()`, the carousel auto-rotation pauses while that board is active. It will not rotate until triggered manually (e.g., via physical button press calling `nextBoard()`) or until the board signals it is ready to yield (if implemented via an event mechanism).

## System vs User Boards

The carousel treats all boards equally. However, system-level boards (like the `NetworkStatusBoard`) are typically registered early by the `Application` bootstrap sequence. Module boards are appended later.

Because boards are queried dynamically via `shouldDisplay()`, a module can implement "alert" boards that only show when a specific condition is met, seamlessly entering and exiting the rotation pool.
