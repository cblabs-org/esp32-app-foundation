# Architecture Decision Record: EventBus

## Context
We needed a lightweight publish/subscribe event bus system to decouple components within the ESP32 App Foundation architecture.

## Decisions
1. **Static Function Pointers vs `std::function`**: We strictly avoided `std::function` and dynamic lambda captures. Instead, we used a static function pointer table `void (*callback)(const char* topic, const void* payload, void* ctx)` alongside a `void* ctx` user context pointer.
    - **Why**: `std::function` can allocate heap memory and cause fragmentation over time on memory-constrained microcontrollers. Pre-allocating a static table of function pointers ensures deterministic memory footprints and avoids runtime heap exhaustion or leaks.
2. **Synchronous Publishing**: The `publish()` method iterates through subscribers and dispatches payloads sequentially in the same thread context.
    - **Why**: Avoids complex asynchronous queueing logic, lock contention, and message payload copying (lifetimes). Payloads are only guaranteed to be valid during the execution of the callback.
3. **Fixed Capacity limits**: 
    - 32 maximum subscribers.
    - 64 bytes maximum topic length.
    - **Why**: Keeps the EventBus instance memory footprint small and predictable.
