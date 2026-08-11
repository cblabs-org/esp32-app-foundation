# ITickable

Contract for any component participating in the Application tick loop.

## Overview

Implementors receive a `tick()` call on every `Application::tick()` iteration. This is the primary mechanism for cooperative multitasking on the main core.

```cpp
class ITickable {
public:
    virtual ~ITickable() = default;

    /**
     * @brief Periodic update hook called by the Application hub on every loop iteration.
     * @param currentMillis Milliseconds since boot.
     */
    virtual void tick(uint32_t currentMillis) = 0;
};
```

## Performance Budget (ADR-17)

Implementations MUST complete within **10ms**. 

The main loop runs the Task Watchdog Timer (TWDT) and processes cooperative tasks. Any long I/O, heavy computation, or blocking call MUST be deferred to a dedicated FreeRTOS task. Exceeding the 10ms budget will cause jitter in other modules (like the display carousel) and may trigger a TWDT panic reset.

If you absolutely must do a slightly longer operation on the main thread, insert `yield()` calls to allow the ESP32 to service background tasks (though this is a code smell — prefer a FreeRTOS task).

## Throttle Pattern

Because `tick()` is called continuously, use a throttle pattern for operations that only need to happen periodically but don't warrant a full FreeRTOS task:

```cpp
void MyModule::tick(uint32_t currentMillis) {
    if (currentMillis - _lastUpdate >= 500) {
        _lastUpdate = currentMillis;
        // Do light work every 500ms
    }
}
```

> [!WARNING]
> Do NOT use `delay()` inside a `tick()` function under any circumstances. It blocks the entire application hub.

## Static Scope Constraint (ADR-16)

Due to memory fragmentation concerns (GRAVE-05), ITickable modules MUST be statically allocated at global scope or inside `main()`. Do not heap-allocate (`new`) modules and register them dynamically.

```cpp
// Correct:
static MyModule myModule;
app.registerModule(&myModule);

// Incorrect:
MyModule* myModule = new MyModule();
app.registerModule(myModule);
```
