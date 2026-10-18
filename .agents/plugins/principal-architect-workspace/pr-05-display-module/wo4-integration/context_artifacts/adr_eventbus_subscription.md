# ADR: EventBus Subscription via Static Callbacks

## Status: Accepted

## Context

The `U8g2DisplayModule` needs to subscribe to EventBus topics (`ota.progress`, `display.message`) during `begin()`. The `EventBus::subscribe()` API takes a C-style function pointer (`EventCallback`) and a `void* ctx` context parameter:

```cpp
using EventCallback = void (*)(const char* topic, const void* payload, void* ctx);
bool subscribe(const char* topic, EventCallback callback, void* ctx = nullptr);
```

This means we cannot use member function pointers or lambdas with captures directly.

## Decision

Use **static member functions** as callbacks, passing `this` as the `ctx` parameter:

```cpp
// In header — private static methods
static void _onOtaProgress(const char* topic, const void* payload, void* ctx);
static void _onDisplayMessage(const char* topic, const void* payload, void* ctx);

// In begin() — subscribe with this as context
_app->getEventBus().subscribe("ota.progress", _onOtaProgress, this);
_app->getEventBus().subscribe("display.message", _onDisplayMessage, this);

// In callback — cast ctx back to self
void U8g2DisplayModule::_onOtaProgress(const char* /*topic*/, const void* payload, void* ctx) {
    auto* self = static_cast<U8g2DisplayModule*>(ctx);
    // ... delegate to self->_otaProgress ...
}
```

## Payload Structs

Two payload structs are defined in `u8g2DisplayModule.hpp` for type-safe casting:

- `OtaProgressPayload` — `{float progress, const char* status, bool complete, bool success}`
- `DisplayMessagePayload` — `{const char* message, unsigned long durationMs}`

These are published by other modules and cast from `const void*` in the callbacks.

## Consequences

- Clean separation of concerns — the display module reacts to events without coupling to OTA or message producers
- The `void* ctx` pattern is idiomatic for C-style callback APIs on embedded systems
- Payload lifetimes: payloads are only valid during the synchronous callback (EventBus::publish is synchronous)
