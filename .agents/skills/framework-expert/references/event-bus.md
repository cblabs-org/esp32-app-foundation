# Event Bus Reference

## EventBus C++ API Signatures

The `EventBus` provides synchronous, in-memory pub-sub decoupling between modules.

### Callback Definition
Subscribers must register a static function or free function matching the callback signature:
```cpp
typedef void (*EventCallback)(const char* topic, const void* payload, void* ctx);
```
*   `topic`: The exact string topic published.
*   `payload`: A generic `const void*` pointer (must cast to expected type).
*   `ctx`: A user-supplied class context pointer (usually `this`).

### Method Signatures

*   **Initialize:** `void begin();`
*   **Subscribe:** `bool subscribe(const char* topic, EventCallback callback, void* ctx = nullptr);`
    Returns `true` if successful; `false` if the subscription table is full.
*   **Unsubscribe:** `bool unsubscribe(const char* topic, EventCallback callback, void* ctx = nullptr);`
    Returns `true` if found and unsubscribed.
*   **Publish:** `int publish(const char* topic, const void* payload = nullptr);`
    Dispatches synchronously. Returns the number of subscribers that received the event.

## Payload Lifetime Rule
> [!WARNING]
> **Strict Synchronous Lifetime:** Payloads passed via `void* payload` are **only valid for the duration of the synchronous callback execution**.
> 
> Once `publish()` returns, the memory occupied by the payload may be reclaimed or overwritten (especially if allocated on the publisher's stack). If a subscriber needs payload data after the callback returns, it **MUST** copy the payload into its own persistent member variables during the callback.

## Topic Conventions & Custom Topic Declarations

To prevent namespace collisions, declare topic strings within a dedicated C++ namespace using uppercase snake case constants:

```cpp
// Header file (e.g. myApp.hpp)
namespace MyAppEvents {
    // lowercase dot-separated: <module_namespace>.<event_name>
    constexpr const char* LOG_TRIGGERED = "myapp.log_triggered";
}

// Payload struct definition
struct LogTriggeredPayload {
    uint32_t timestamp;
    char message[64];
};
```

## Publishing Events

Populate the payload on the stack and publish it:
```cpp
void MyApp::triggerLog(const char* msg) {
    LogTriggeredPayload payload;
    payload.timestamp = millis();
    strncpy(payload.message, msg, sizeof(payload.message) - 1);
    
    app.getEventBus().publish(MyAppEvents::LOG_TRIGGERED, &payload);
}
```

## Subscribing to Events

Register a static function and delegate to a non-static instance method using `ctx` (`this`):

```cpp
// Header declaration
class OtherModule {
public:
    void init() {
        app.getEventBus().subscribe(MyAppEvents::LOG_TRIGGERED, _onLogTriggered, this);
    }
private:
    static void _onLogTriggered(const char* topic, const void* payload, void* ctx) {
        if (!ctx || !payload) return;
        auto* self = static_cast<OtherModule*>(ctx);
        const auto* data = static_cast<const LogTriggeredPayload*>(payload);
        self->handleLog(*data);
    }
    void handleLog(const LogTriggeredPayload& data) {
        // Safe instance logic here
    }
};
```

## Framework-Published Topics Reference

| Topic Constant | String Value | Publisher | Payload Type | Description |
| :--- | :--- | :--- | :--- | :--- |
| `ApplicationEvents::SYSTEM_HEAP` | `"system.heap"` | `Application` | `const uint32_t*` | Free heap memory in bytes. |
| `ApplicationEvents::APP_RUNNING` | `"app.running"` | `Application` | `nullptr` | Emitted on entering RUNNING state. |
| `ApplicationEvents::APP_CONFIGURING` | `"app.configuring"` | `Application` | `const AppConfiguringPayload*` | Emitted on entering CONFIGURING state. |
| `OTAUpdateManagerEvents::OTA_PROGRESS` | `"ota.progress"` | `OTAUpdateManager` | `const OTAProgressEvent*` | Active firmware download progress. |
| `U8G2DisplayEvents::DISPLAY_SLEEP` | `"display.sleep"` | `U8G2DisplayModule` | `nullptr` | Screen saver triggered (prevent burn-in). |
| `U8G2DisplayEvents::DISPLAY_WAKE` | `"display.wake"` | `U8G2DisplayModule` | `nullptr` | User interaction woke screen up. |
| `U8G2DisplayEvents::DISPLAY_BOARD_CHANGED` | `"display.board.changed"` | `U8G2DisplayModule` | `const DisplayBoardChangedPayload*` | Telemetry board changed. |
| `U8G2DisplayEvents::DISPLAY_MESSAGE` | `"display.message"` | *Any Module* | `const DisplayMessagePayload*` | Post scrolling message overlays. |
