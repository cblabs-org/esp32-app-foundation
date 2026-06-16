# IDataSource

Contract for background data-fetch workers managed by `DataManager`.

## Overview

Unlike `ITickable` which runs on the main core with strict time budgets, `IDataSource` fetches execute in a dedicated FreeRTOS task, making them suitable for long-running blocking operations like HTTP requests.

```cpp
class IDataSource {
public:
    virtual ~IDataSource() = default;
    virtual UpdateStatus fetchUpdate() = 0;
    virtual uint32_t getUpdateIntervalMs() const = 0;
    virtual const char* getSourceName() const = 0;
    virtual PriorityTier getPriorityTier() const = 0;
};
```

## Enums

### `UpdateStatus`

Returned by `fetchUpdate()` to indicate the outcome:
- `Success`: Fetched and parsed successfully.
- `NoChange`: Server returned 304 or data unchanged.
- `RateLimited`: Server returned 429 or local backoff applied.
- `FetchError`: Network or TLS failure.
- `ParseError`: Response received but JSON parsing failed.
- `AuthError`: API key rejected.

### `PriorityTier`

Determines scheduling grouping:
- `High`: ~60 seconds
- `Normal`: ~300 seconds (5 mins)
- `Low`: ~1800 seconds (30 mins)

## Threading Model & Stack Placement

`fetchUpdate()` executes in a FreeRTOS task outside the main `loop()`. 

> [!CAUTION]
> You must ensure thread safety when sharing data between `fetchUpdate()` (the FreeRTOS task) and other framework interfaces like `IWebContributor` or `ITickable` (the main task). Use `std::atomic`, mutexes, or `EventBus` payloads to cross thread boundaries safely.

**ADR-14 (WiFiClientSecure placement)**: `WiFiClientSecure` objects allocate massive internal state (MbedTLS context). Do NOT place them as class members, or you will bloat the global heap. Always instantiate them locally within `fetchUpdate()` so they are allocated on the FreeRTOS task stack, and destroyed when the function returns.

```cpp
UpdateStatus MySource::fetchUpdate() {
    // Instantiate locally so it uses the FreeRTOS task stack (ADR-14)
    WiFiClientSecure client;
    client.setInsecure(); // Or provide cert
    
    HTTPClient http;
    http.begin(client, "https://api.example.com/data");
    
    // ... fetch and parse ...
    
    return UpdateStatus::Success;
}
```
