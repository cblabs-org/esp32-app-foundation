# Scheduling Data Fetches

> ESP32 App Foundation (c) 2026 CB Labs
>
> Learn how to use the `IDataSource` interface and `DataManager` to schedule background data fetches on a dedicated FreeRTOS task.

---

This guide covers the framework's background data-fetching subsystem. If your module needs to periodically pull data from an external API, poll a sensor on a slow bus, or perform any I/O that would violate the [10ms tick budget](creating-a-module.md#the-tick-loop--injected-time), the `IDataSource` + `DataManager` pattern is the correct approach.

---

## 1. The `IDataSource` Interface

The [IDataSource](../../include/interfaces/IDataSource.hpp) interface defines the contract for any module that provides periodic background data. It is a pure virtual class with four required methods:

```cpp
class IDataSource {
public:
    virtual ~IDataSource() = default;

    virtual UpdateStatus  fetchUpdate() = 0;
    virtual uint32_t      getUpdateIntervalMs() const = 0;
    virtual const char*   getSourceName() const = 0;
    virtual PriorityTier  getPriorityTier() const = 0;
};
```

### Method Signatures

| Method | Returns | Purpose |
|:---|:---|:---|
| `fetchUpdate()` | `UpdateStatus` | Execute one full data-fetch cycle. Called by the DataManager on its own FreeRTOS task. |
| `getUpdateIntervalMs()` | `uint32_t` | Return the desired polling interval in milliseconds between successive `fetchUpdate()` calls. |
| `getSourceName()` | `const char*` | Return a human-readable identifier used in log output and EventBus topic names. |
| `getPriorityTier()` | `PriorityTier` | Return the scheduling priority tier used by the DataManager to group sources. |

### UpdateStatus Return Codes

Your `fetchUpdate()` implementation must return one of these result codes:

| Code | Value | Meaning |
|:---|:---|:---|
| `Success` | 0 | Data fetched and parsed successfully |
| `NoChange` | 1 | Server returned 304 or data is unchanged |
| `RateLimited` | 2 | Server returned 429 or local back-off triggered |
| `FetchError` | 3 | Network or TLS connection failure |
| `ParseError` | 4 | Response received but JSON/payload parsing failed |
| `AuthError` | 5 | API key rejected or credentials missing |

### PriorityTier Scheduling Tiers

| Tier | Approximate Refresh Rate |
|:---|:---|
| `PriorityTier::High` | Every ~60 seconds |
| `PriorityTier::Normal` | Every ~300 seconds (5 minutes) |
| `PriorityTier::Low` | Every ~1800 seconds (30 minutes) |

---

## 2. The DataManager

The [DataManager](../../modules/DataManager/DataManager.hpp) is the framework's background scheduler that manages all registered `IDataSource` instances. It runs its own dedicated FreeRTOS task, separate from the main tick loop.

### Public API

| Method | Description |
|:---|:---|
| `begin()` | Initialize the FreeRTOS task and start the scheduling loop. Called by `Application::begin()`. |
| `registerSource(IDataSource* source)` | Add a data source to the polling loop. |
| `unregisterSource(IDataSource* source)` | Remove a data source from the polling loop. |
| `requestPriorityFetch(IDataSource* source)` | Mark a source for immediate out-of-schedule refresh. |
| `setAppRunning(bool running)` | Enable or disable data fetching. When `false`, the worker task idles. |
| `processNext()` | (Test only) Synchronous tick for unit testing — simulates one iteration of the worker task. |

### Gating Behaviour

The `DataManager` only performs fetches when the application is in the `RUNNING` state:

- `setAppRunning(true)` — called automatically when the application transitions to `RUNNING`
- `setAppRunning(false)` — called when the application regresses to `CONFIGURING`

When gated, registered sources remain in the registry but `fetchUpdate()` is never called.

---

## 3. Registration

There are two ways to register a data source with the DataManager:

### Option A — Via `registerModule()` (Recommended)

If your module implements `IDataSource` alongside other interfaces, the universal `registerModule()` call auto-detects it:

```cpp
// In setup()
app.registerModule(&myWeatherModule);
// The framework's registerModule<T>() uses:
//   if constexpr (std::is_base_of_v<IDataSource, T>) { registerDataSource(module); }
```

This is the preferred approach because it keeps all registration in one place.

### Option B — Direct DataManager Registration

For cases where you want to register a standalone `IDataSource` that is not a full module:

```cpp
app.getDataManager().registerSource(&myDataSource);
```

> **Note:** Both registration methods must be called in `setup()` before `app.begin()`. The registration registry is compacted and locked during the boot sequence.

---

## 4. Threading Model

> [!IMPORTANT]
> **`fetchUpdate()` executes on the DataManager's FreeRTOS task, NOT the main tick loop.** This is the most important thing to understand about the data-fetching subsystem.

### Architecture

```
┌───────────────────────────┐     ┌───────────────────────────┐
│     Main Loop (Core 1)    │     │   DataManager Task        │
│                           │     │   (Core 0, 8KB stack)     │
│  app.tick()               │     │                           │
│    ├── myModule.tick()    │     │  executeLoop()            │
│    ├── display.tick()     │     │    ├── source1.fetchUpdate()
│    └── ...                │     │    ├── source2.fetchUpdate()
│                           │     │    └── ...                │
└───────────────────────────┘     └───────────────────────────┘
        ▲                                    │
        │       Shared State (⚠️ mutex)       │
        └────────────────────────────────────┘
```

### Thread Safety Implications

Because `fetchUpdate()` runs on a different task from `tick()`:

1. **Shared member variables** accessed by both `tick()` and `fetchUpdate()` must be protected with a mutex, or use `std::atomic` for simple types.
2. **Do not call Arduino APIs** (like `Serial.print`) from `fetchUpdate()` — use `ESP_LOGI` / `ESP_LOGW` for thread-safe logging.
3. **Heap allocation is safe** inside `fetchUpdate()` — the DataManager task has its own stack (8KB default). Avoid allocating large buffers that could overflow the stack.
4. **Network operations are safe** — `fetchUpdate()` is specifically designed for blocking I/O operations like HTTP requests, which would violate the 10ms tick budget if done in the main loop.

> [!NOTE]
> See the [Embedded Safety](embedded-safety.md) guide for detailed thread safety rules, ISR constraints, and FreeRTOS task best practices.

### `WiFiClientSecure` Placement (ADR-14)

`WiFiClientSecure` objects allocate massive internal state (MbedTLS context, ~15–40KB). Careless placement causes heap bloat or stack overflow.

**For simple data fetchers** (like weather or sensor API calls), instantiate `WiFiClientSecure` locally within `fetchUpdate()` so it is allocated on the DataManager's FreeRTOS task stack and automatically destroyed when the function returns:

```cpp
UpdateStatus MySource::fetchUpdate() {
    // ADR-14: Instantiate locally — allocated on the FreeRTOS task stack
    WiFiClientSecure client;
    client.setInsecure(); // Or provide CA certificate

    HTTPClient http;
    http.begin(client, "https://api.example.com/data");
    // ... fetch and parse ...
    return UpdateStatus::Success;
}
```

**For complex scenarios** (like OTA, where the client must outlive the function scope or be shared across multiple HTTP requests), allocate on the heap via `std::unique_ptr` with `std::nothrow` to handle allocation failure gracefully:

```cpp
std::unique_ptr<WiFiClientSecure> client(new (std::nothrow) WiFiClientSecure());
if (!client) {
    ESP_LOGE(TAG, "Failed to allocate WiFiClientSecure on heap");
    return UpdateStatus::FetchError;
}
```

> [!WARNING]
> **Never** place `WiFiClientSecure` as a class member variable. Its MbedTLS context will permanently consume heap for the lifetime of the module, even when no TLS connection is active.

### `std::atomic` Patterns for Cross-Task Data

For simple values shared between the FreeRTOS data task and the main tick loop, use `std::atomic` instead of mutexes. This is significantly faster and avoids deadlock risks:

```cpp
#include <atomic>

class MyFetcher : public ITickable, public IDataSource {
private:
    // Written by fetchUpdate() on DataManager task,
    // read by tick() on main loop — safe via std::atomic
    std::atomic<float>  _temperature{0.0f};
    std::atomic<bool>   _dataReady{false};
    std::atomic<int>    _errorCount{0};

public:
    void tick(uint32_t currentMillis) override {
        if (_dataReady.load()) {
            float temp = _temperature.load();
            // Update display, publish to EventBus, etc.
        }
    }

    UpdateStatus fetchUpdate() override {
        // ... fetch from API ...
        _temperature.store(parsedTemp);
        _dataReady.store(true);
        return UpdateStatus::Success;
    }
};
```

For complex shared state (structs, strings, arrays), use a FreeRTOS mutex:

```cpp
SemaphoreHandle_t _dataMutex = xSemaphoreCreateMutex();

// In fetchUpdate() (DataManager task):
if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    _complexData = newData;
    xSemaphoreGive(_dataMutex);
}

// In tick() (main loop):
if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    auto snapshot = _complexData;
    xSemaphoreGive(_dataMutex);
    // Use snapshot safely
}
```

---

## 5. Example — Skeleton IDataSource Implementation

Here is a complete, minimal `IDataSource` implementation that fetches JSON data from an HTTP API:

```cpp
// modules/WeatherFetcher/WeatherFetcher.hpp
#pragma once

#include "interfaces/ITickable.hpp"
#include "interfaces/IDataSource.hpp"
#include <cstdint>
#include <atomic>

#ifndef NATIVE_BUILD
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#endif

class WeatherFetcher : public ITickable, public IDataSource {
public:
    // ITickable — read cached data, update display
    void tick(uint32_t currentMillis) override;

    // IDataSource — background HTTP fetch
    UpdateStatus  fetchUpdate() override;
    uint32_t      getUpdateIntervalMs() const override { return 300000; } // 5 minutes
    const char*   getSourceName() const override { return "weather"; }
    PriorityTier  getPriorityTier() const override { return PriorityTier::Normal; }

    // Accessors for cached data (thread-safe via atomic)
    float getTemperature() const { return _temperature.load(); }

private:
    // Atomic for cross-task access: written by fetchUpdate(), read by tick()
    std::atomic<float> _temperature{0.0f};

    // Tick timing
    uint32_t _lastDisplayUpdateMs = 0;
};
```

```cpp
// modules/WeatherFetcher/WeatherFetcher.cpp
#include "WeatherFetcher.hpp"

#ifndef NATIVE_BUILD
#include <HTTPClient.h>
#include <esp_log.h>

static const char* TAG = "WeatherFetcher";
#endif

void WeatherFetcher::tick(uint32_t currentMillis) {
    // Read cached data on the main loop — safe via std::atomic
    if (currentMillis - _lastDisplayUpdateMs >= 1000) {
        _lastDisplayUpdateMs = currentMillis;
        float temp = _temperature.load();
        // Update display or publish to EventBus...
    }
}

#ifndef NATIVE_BUILD
UpdateStatus WeatherFetcher::fetchUpdate() {
    // This runs on the DataManager's FreeRTOS task — blocking I/O is OK
    HTTPClient http;
    http.begin("https://api.example.com/weather?city=london");

    int httpCode = http.GET();
    if (httpCode != 200) {
        ESP_LOGW(TAG, "HTTP %d", httpCode);
        http.end();
        return (httpCode == 429) ? UpdateStatus::RateLimited
                                 : UpdateStatus::FetchError;
    }

    // Parse JSON response
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();

    if (err) {
        ESP_LOGW(TAG, "JSON parse error: %s", err.c_str());
        return UpdateStatus::ParseError;
    }

    // Store result atomically for main-loop access
    _temperature.store(doc["temp"].as<float>());

    ESP_LOGI(TAG, "Fetched temperature: %.1f°C", _temperature.load());
    return UpdateStatus::Success;
}
#else
UpdateStatus WeatherFetcher::fetchUpdate() {
    // No-op for native test builds
    return UpdateStatus::Success;
}
#endif
```

### Registration

```cpp
// In main.cpp
#include "AppFoundation.h"
#include "myApp.hpp"
#include "WeatherFetcher.hpp"

Application app;
MyApp myApp;
WeatherFetcher weather;

void setup() {
    app.registerModule(&myApp);
    app.registerModule(&weather);  // Auto-registers as ITickable + IDataSource

    FrameworkConfig config;
    app.begin(config);
}
```

### Requesting an Immediate Refresh

If your module needs fresh data outside of the normal schedule (e.g., user pressed a "refresh" button on the web portal):

```cpp
// From a REST handler or MCP tool
app.getDataManager().requestPriorityFetch(&weather);
```

This sends a wake event to the DataManager's task, causing it to evaluate `weather.fetchUpdate()` immediately.

---

## 6. Developer Checklist

- [ ] **Implement all four `IDataSource` methods** — the compiler will enforce this via pure virtual
- [ ] **Return accurate `UpdateStatus` codes** — the DataManager may use these for back-off or retry logic
- [ ] **Protect shared state** — use `std::atomic` for simple types or a mutex for complex data shared between `tick()` and `fetchUpdate()`
- [ ] **Use `ESP_LOGI` / `ESP_LOGW`** for logging inside `fetchUpdate()` — not `Serial.print`
- [ ] **Guard hardware code** with `#ifndef NATIVE_BUILD` — see [Using Optional Modules](using-optional-modules.md)
- [ ] **Register via `registerModule()`** for automatic subsystem detection
- [ ] **Keep stack usage under 8KB** in `fetchUpdate()` — avoid large local buffers
