# IDisplayBoard

Contract for a U8G2 carousel display board. 

## Overview

The `u8g2Display` module maintains a rotation of "boards". Each board is a full-screen rendering unit.

```cpp
class IDisplayBoard {
public:
    virtual ~IDisplayBoard() = default;
    virtual void render(U8G2& u8g2) = 0;
    virtual bool isExpired() const = 0;
    virtual const char* getBoardName() const = 0;
    virtual void onActivate() {}
    virtual void onDeactivate() {}
    virtual void tick(uint32_t currentMillis) {}
};
```

## Lifecycle

The `CarouselEngine` manages the transition between boards. The lifecycle is:

1. **Activate**: `onActivate()` is called when the board becomes the active target. Use this to reset scroll positions, initialize animations, or snapshot telemetry.
2. **Tick**: `tick()` is called on every main loop iteration while the board is active. Use this for per-frame timing (advancing scroll counters).
3. **Render**: `render()` is called within the U8G2 draw loop. Use the `u8g2` reference to draw primitives and text.
4. **Deactivate**: `onDeactivate()` is called when the board goes off-screen.
5. **Expiry**: `isExpired()` is polled. If it returns true, the board is evicted from the carousel pool entirely. (System boards typically never expire; dynamic notification boards do).

## Implementation Example

```cpp
void MyBoard::onActivate() {
    _scrollPos = 0;
}

void MyBoard::tick(uint32_t currentMillis) {
    if (currentMillis - _lastTick > 100) {
        _scrollPos++;
        _lastTick = currentMillis;
    }
}

void MyBoard::render(U8G2& u8g2) {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 10 - _scrollPos, "Scrolling Text...");
}
```

> [!NOTE]
> `render()` is executing synchronously on the main thread and MUST be fast. Do not perform any heavy calculation or network I/O during rendering.
