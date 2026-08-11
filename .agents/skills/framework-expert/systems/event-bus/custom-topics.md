# Custom Topics

Modules are encouraged to publish their own events to the `EventBus` to notify other modules of state changes. This enables decoupled architectures where modules don't need direct references to each other.

## Namespace Convention

To prevent topic collisions, all custom topics MUST be prefixed with the module's logical namespace, followed by a dot.

- **Correct**: `sensor.reading_ready`, `weather.updated`, `alarm.triggered`
- **Incorrect**: `reading_ready`, `weather`, `update`

## Payload Struct Design

When defining the payload struct for a custom topic:

1. **Header Placement**: Define the struct in the publisher's header file so subscribers can cast the `void*` payload correctly.
2. **Keep it Small**: The struct is allocated on the stack. Do not include massive arrays or deep object graphs.
3. **Use Pointers Carefully**: You may include pointers to strings or larger data structures, but remember the *Payload Lifetime Rule*. The subscriber must copy any necessary data before the callback returns. If a pointer points to heap memory owned by the publisher, the subscriber must not free it.

```cpp
// In WeatherModule.h
struct WeatherUpdatedEvent {
    float temperature;
    float humidity;
    const char* conditionString; // Pointer to static memory or publisher's buffer
};

// In WeatherModule.cpp
void WeatherModule::fetchData() {
    // ... fetch logic ...
    WeatherUpdatedEvent event = { _temp, _humidity, _condition };
    app->getEventBus()->publish("weather.updated", &event);
}
```

## Subscriber Casting

Subscribers must statically cast the `void*` payload back to the defined struct.

```cpp
void MyModule::onWeatherUpdated(const char* topic, const void* payload, void* ctx) {
    if (!payload) return; // Always check for null!
    
    const WeatherUpdatedEvent* event = static_cast<const WeatherUpdatedEvent*>(payload);
    
    // Copy data for later use
    instance->_currentTemp = event->temperature;
    
    // Safe to use event->conditionString here, but not after returning
    Serial.printf("Weather: %s\n", event->conditionString);
}
```
