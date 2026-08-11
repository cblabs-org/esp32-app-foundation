# Event Bus Overview

The `EventBus` provides a lightweight, synchronous publish/subscribe mechanism. It is the primary way modules communicate state changes without tightly coupling to each other.

## Pub/Sub API Signatures

The bus uses string-based topics.

### Subscribing

Modules subscribe to a topic by providing a callback function and an optional context pointer.

```cpp
// Signature: void subscribe(const char* topic, EventCallback callback, void* ctx = nullptr);
// EventCallback definition: typedef void (*EventCallback)(const char* topic, const void* payload, void* ctx);

app.getEventBus().subscribe("system.heap", MyModule::onHeapReport, this);
```

### Publishing

Modules publish events by providing the topic and a pointer to a payload struct.

```cpp
// Signature: void publish(const char* topic, const void* payload);

WeatherUpdatedEvent payload = { 22.5f, 60.0f };
app.getEventBus().publish("weather.updated", &payload);
```

## WARNING: Payload Lifetime Rule

The `EventBus` is synchronous. When `publish()` is called, the bus immediately iterates through all subscribers and executes their callbacks sequentially. 

**This means the payload pointer is only valid for the duration of the callback.**

If a subscriber needs to access the payload data later (e.g., in its next `tick()` cycle), it **must copy the data** out of the payload struct into its own memory space. Subscribing modules must never store the `const void* payload` pointer itself, as it points to memory allocated on the publisher's stack, which will be destroyed as soon as the `publish()` call returns.

## GRAVE-05: Static Function Pointer Rationale

You will notice that `EventCallback` is a C-style function pointer (`typedef void (*EventCallback)(const char* topic, const void* payload, void* ctx);`), rather than a `std::function`. This is a deliberate architectural decision documented in **GRAVE-05**.

`std::function` frequently allocates memory on the heap (type erasure overhead). In an embedded environment like the ESP32, heavy heap fragmentation from dynamic callback allocations leads to out-of-memory panics. 

Therefore, subscribers must provide static class methods or global functions. To bridge the gap back to an instance method, modules use the `ctx` parameter passed during subscription:

```cpp
// Static callback method — bridges to instance via ctx pointer
static void onEvent(const char* topic, const void* payload, void* ctx) {
    auto* self = static_cast<MyModule*>(ctx);
    // Access instance members via self->
}

// Registration — pass `this` as ctx
eventBus.subscribe("topic.name", MyModule::onEvent, this);
```
