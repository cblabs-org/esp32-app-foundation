# Event Bus

> ESP32 App Foundation (c) 2026 CB Labs
>
> Learn how to use the framework's EventBus for decoupled, event-driven cross-module communication, including topic conventions, payload lifetimes, and framework topics.

---

The ESP32 App Foundation features a synchronous, in-memory publish-subscribe [EventBus](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/modules/EventBus/EventBus.hpp) that allows optional modules to communicate without direct dependencies.

For architectural decisions and structure details, see the [Inter-Module Communication](../design_specification.md#11-inter-module-communication) section in the Design Specification.

---

## 1. EventBus C++ API

The `EventBus` class provides a simple API for subscribing, unsubscribing, and publishing events.

### Callback Definition
Subscribers must register a static function pointer (or free function) matching the `EventCallback` signature:
```cpp
typedef void (*EventCallback)(const char* topic, const void* payload, void* ctx);
```
- **`topic`:** The exact string topic that was published.
- **`payload`:** A generic `const void*` pointer. Cast this to the expected struct or type.
- **`ctx`:** A user-supplied pointer passed back to your callback function (often used to pass the calling class instance `this`).

### Method Signatures:

#### `begin()`
Initializes the event bus and clears any active subscriptions.
```cpp
void begin();
```

#### `subscribe()`
Registers a callback function to listen to a specific topic string.
```cpp
bool subscribe(const char* topic, EventCallback callback, void* ctx = nullptr);
```
- **Returns:** `true` if subscription succeeded; `false` if the subscription table is full or the topic name is invalid.

#### `unsubscribe()`
Removes a registered callback.
```cpp
bool unsubscribe(const char* topic, EventCallback callback, void* ctx = nullptr);
```
- **Returns:** `true` if the subscription was successfully found and removed; `false` otherwise.

#### `publish()`
Dispatches a payload synchronously to all matching subscribers in the order they subscribed.
```cpp
int publish(const char* topic, const void* payload = nullptr);
```
- **Returns:** The number of active subscribers that received the event.

---

## 2. Payload Lifetime Rule

> [!WARNING]
> **Strict Synchronous Lifetime:** 
> Payloads passed via the EventBus (`void* payload`) are **only valid for the duration of the synchronous callback execution**. 
>
> Once `publish()` returns, the memory occupied by the payload may be reclaimed or overwritten (especially if it was allocated on the publisher's stack). If a subscriber needs to use the payload data after the callback returns (e.g. deferring work to a loop or another thread), the subscriber **MUST** copy the data into its own persistent member variables.

---

## 3. Module-Owned Topic Conventions

To prevent naming collisions across independent modules, all topics are declared using string constants grouped inside a descriptive events namespace. 

### Topic Namespace Pattern:
```cpp
namespace ModuleNameEvents {
    constexpr const char* EVENT_NAME = "modulename.event_name";
}
```
- **Namespace Name:** Ends with `Events` (e.g., `WifiEvents`, `U8G2DisplayEvents`).
- **Constant Names:** UPPERCASE snake case.
- **String Value:** Lowercase dot-separated format, starting with the module namespace (e.g., `"wifi.connected"`).

---

## 4. Framework-Published Topics Reference

The framework core modules publish several system events that custom modules can subscribe to:

| Topic Constant | String Value | Publisher | Payload Type | Description |
| :--- | :--- | :--- | :--- | :--- |
| `ApplicationEvents::SYSTEM_HEAP` | `"system.heap"` | [Application](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/modules/Application/Application.hpp) | `const uint32_t*` | Emitted periodically containing the current free heap memory in bytes. |
| `ApplicationEvents::APP_RUNNING` | `"app.running"` | [Application](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/modules/Application/Application.hpp) | `nullptr` | Emitted when all configuration gates are satisfied and the system enters the RUNNING state. |
| `ApplicationEvents::APP_CONFIGURING` | `"app.configuring"` | [Application](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/modules/Application/Application.hpp) | `const AppConfiguringPayload*` | Emitted when entering the CONFIGURING state. Payload contains `gateName`, `helpText`, and `isRegression` flag (see [AppConfiguringPayload.hpp](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/include/events/AppConfiguringPayload.hpp)). |
| `OTAUpdateManagerEvents::OTA_PROGRESS` | `"ota.progress"` | [OTAUpdateManager](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/modules/OTAUpdateManager/OTAUpdateManager.hpp) | `const OTAProgressEvent*` | Emitted during an active firmware download. Payload contains `progress` (0.0–1.0), `status` string, `complete` flag, and `success` flag. |
| `U8G2DisplayEvents::DISPLAY_SLEEP` | `"display.sleep"` | [U8G2DisplayModule](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/modules/U8G2Display/U8G2DisplayModule.hpp) | `nullptr` | Published when the screen saver kicks in and the display shuts down to prevent OLED burn-in. |
| `U8G2DisplayEvents::DISPLAY_WAKE` | `"display.wake"` | [U8G2DisplayModule](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/modules/U8G2Display/U8G2DisplayModule.hpp) | `nullptr` | Published when user interaction wakes the screen back up. |
| `U8G2DisplayEvents::DISPLAY_BOARD_CHANGED` | `"display.board.changed"` | [U8G2DisplayModule](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/modules/U8G2Display/U8G2DisplayModule.hpp) | `const DisplayBoardChangedPayload*` | Emitted when the visible board on the U8G2 carousel changes. Payload contains `boardName` and `boardIndex`. |
| `U8G2DisplayEvents::DISPLAY_MESSAGE` | `"display.message"` | *Any Module* | `const DisplayMessagePayload*` | Subscribed to by the display module. Any module can publish to this topic to display a scrolling message overlay. Payload contains `message` and `durationMs`. |

---

## 5. Declaring and Publishing Custom Events

Here is how you can declare and publish your own application events inside a custom module:

### Step 1: Declare the Events Namespace
In your module's header file (e.g. `myApp.hpp`), define your namespace and events:

```cpp
namespace MyAppEvents {
    // String topic name matching 'module.event' convention
    constexpr const char* FEATURE_TRIGGERED = "myapp.feature_triggered";
}

// Struct to hold payload data
struct FeatureTriggeredPayload {
    int mode;
    uint32_t durationMs;
};
```

### Step 2: Publish the Event
Inside your module's C++ code, populate the payload struct and publish it using the EventBus instance (accessible from the `Application` hub):

```cpp
#include "AppFoundation.h"
#include "myApp.hpp"

extern Application app; // Reference to global Application hub

void MyApp::triggerFeature(int mode, uint32_t duration) {
    // Allocate payload on the stack (cheap and safe for synchronous delivery)
    FeatureTriggeredPayload payload;
    payload.mode = mode;
    payload.durationMs = duration;

    // Dispatch synchronously to all subscribers
    app.getEventBus().publish(MyAppEvents::FEATURE_TRIGGERED, &payload);
}
```

---

## 6. Subscribing to Events

To listen to events, you must implement a static callback function and subscribe to the EventBus.

### Step 1: Declare the Static Callback Function
In your receiver class:
```cpp
class OtherModule {
public:
    void setupSubscription();
    
private:
    // Callback must be static to match EventCallback function pointer signature
    static void _onFeatureTriggered(const char* topic, const void* payload, void* ctx);
    
    // Non-static method to handle actual logic
    void handleFeature(const FeatureTriggeredPayload& data);
};
```

### Step 2: Subscribe and Pass Class Instance Context
Register your callback in the setup function, passing `this` as the context pointer so your static callback can call back into your non-static methods:
```cpp
void OtherModule::setupSubscription() {
    app.getEventBus().subscribe(
        MyAppEvents::FEATURE_TRIGGERED, 
        _onFeatureTriggered, 
        this // Passed as ctx to callback
    );
}
```

### Step 3: Implement the Static Callback
Cast the `ctx` pointer back to your class type, cast the `payload` pointer back to the expected event payload struct, copy the values if you need them later, and delegate:
```cpp
void OtherModule::_onFeatureTriggered(const char* topic, const void* payload, void* ctx) {
    if (!ctx || !payload) return;

    // 1. Cast context back to class instance pointer
    auto* self = static_cast<OtherModule*>(ctx);

    // 2. Cast payload to expected struct pointer
    const auto* data = static_cast<const FeatureTriggeredPayload*>(payload);

    // 3. Delegate to non-static member function
    self->handleFeature(*data);
}

void OtherModule::handleFeature(const FeatureTriggeredPayload& data) {
    // Real logic runs here safely within your class instance scope
    ESP_LOGI("OtherModule", "Feature triggered: mode=%d duration=%dms", 
             data.mode, data.durationMs);
}
```

---

## 7. GRAVE-05: Why `std::function` Was Rejected

You will notice that `EventCallback` is a C-style function pointer rather than a `std::function<void(const char*, const void*, void*)>`. This is a deliberate architectural decision documented in **GRAVE-05**.

`std::function` uses **type erasure**, which frequently allocates memory on the heap. In an embedded environment like the ESP32, heavy heap fragmentation from dynamic callback allocations leads to out-of-memory panics during steady-state execution.

> [!IMPORTANT]
> **Static function pointers only.** The `EventBus` will never accept `std::function`, lambdas with captures, or `std::bind` results. This is a permanent constraint, not a temporary limitation.

To bridge the gap between a static callback and an instance method, use the `ctx` parameter pattern:

```cpp
// In MyModule.h — declare a static callback
static void onHeapReport(const char* topic, const void* payload, void* ctx);

// In MyModule.cpp — subscribe, passing 'this' as context
void MyModule::begin() {
    app.getEventBus().subscribe("system.heap", MyModule::onHeapReport, this);
}

// The static callback casts ctx back to the instance pointer
void MyModule::onHeapReport(const char* topic, const void* payload, void* ctx) {
    auto* self = static_cast<MyModule*>(ctx);
    const auto* freeHeap = static_cast<const uint32_t*>(payload);
    self->_lastHeapValue = *freeHeap;
}
```

This pattern provides full object-oriented dispatch with zero heap allocation overhead.

---

## 8. Custom Topic Conventions

Module-owned topics use a `<module>.<event>` dot-separated format to prevent naming collisions across independent modules:

- **Correct:** `sensor.reading_ready`, `weather.updated`, `alarm.triggered`
- **Incorrect:** `reading_ready`, `weather`, `update`

Always define topics as `constexpr` string constants inside a `<ModuleName>Events` namespace:

```cpp
namespace WeatherEvents {
    constexpr const char* UPDATED = "weather.updated";
    constexpr const char* FORECAST_READY = "weather.forecast_ready";
}
```

Payload structs should be defined in the publisher's header file so subscribers can cast the `void*` payload correctly. Keep payload structs small — they are allocated on the publisher's stack.
