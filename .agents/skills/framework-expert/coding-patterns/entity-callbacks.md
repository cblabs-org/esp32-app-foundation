# Entity Callback Patterns

## Overview

Commandable Home Assistant entities (HaSwitch, HaLight, HaClimate, etc.) use
callbacks to notify the application when a command arrives via MQTT. This document
covers the correct callback pattern.

## Callback Registration Pattern

All entity callbacks MUST use static function pointers with `void* ctx` — NOT
`std::function`. See [memory-rules.md § 6](../embedded-safety/memory-rules.md)
for the rationale.

```cpp
// Entity definition
using CommandFn = void(*)(bool state, void* ctx);
void onCommand(CommandFn callback, void* ctx = nullptr);

// Application registration
haSwitch.onCommand([](bool state, void* ctx) {
    auto* self = static_cast<MyApp*>(ctx);
    self->handleSwitch(state);
}, this);
```

## Thread Safety

Entity command callbacks execute on the **esp-mqtt background task**, not the
main `loop()` task. This means:

1. Do NOT access non-thread-safe data structures directly in the callback
2. Use `std::atomic` for simple flags or numeric values
3. Use a FreeRTOS mutex for complex state updates
4. Or, set an atomic flag and handle the command in `tick()` on the main thread

**Recommended pattern — deferred processing:**
```cpp
std::atomic<bool> _pendingCommand{false};
bool _pendingState = false;

// In callback (esp-mqtt task):
_pendingState = state;
_pendingCommand.store(true, std::memory_order_release);

// In tick() (main task):
if (_pendingCommand.load(std::memory_order_acquire)) {
    _pendingCommand.store(false, std::memory_order_relaxed);
    // Safe to process _pendingState here
}
```

## Entity State Updates

When publishing state back to Home Assistant (`entity.setState()`), prefer
calling from the main thread (`tick()`). If called from a callback on the
esp-mqtt task, ensure the entity's internal state storage is thread-safe.
