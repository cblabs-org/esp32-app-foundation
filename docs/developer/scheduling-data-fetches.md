# Scheduling Data Fetches

> ESP32 App Foundation (c) 2026 CB Labs
>
> Learn how to use the `IDataSource` interface, `FetchContext`, and `DataManager` to schedule background data fetches and asynchronous jobs on a dedicated FreeRTOS task.

---

This guide covers the framework's background data-fetching subsystem. If your module needs to periodically pull data from an external API, poll a sensor on a slow bus, or perform any I/O that would violate the [10ms tick budget](creating-a-module.md#the-tick-loop--injected-time), the `IDataSource` + `DataManager` pattern is the correct approach.

> [!NOTE]
> For the in-depth resilience contract, deadline policies, and health state machine, see the [FetchContext Contract](fetch-context-contract.md). For migrating existing code to the `const FetchContext&` signature, see the [Migration Guide](migration-guide-fetchupdate.md).

---

## 1. The `IDataSource` Interface

The [IDataSource](../../include/interfaces/IDataSource.hpp) interface defines the contract for any module that provides periodic background data. It is a pure virtual class:

```cpp
class IDataSource {
public:
    virtual ~IDataSource() = default;

    virtual UpdateStatus  fetchUpdate(const FetchContext& ctx) = 0;
    virtual uint32_t      getUpdateIntervalMs() const = 0;
    virtual const char*   getSourceName() const = 0;
    virtual PriorityTier  getPriorityTier() const = 0;
    virtual bool          fetchesWhileConfiguring() const { return false; }
};
```

### Method Signatures

| Method | Returns | Purpose |
|:---|:---|:---|
| `fetchUpdate(ctx)` | `UpdateStatus` | Execute one full data-fetch cycle. Called by DataManager on its own FreeRTOS worker task. |
| `getUpdateIntervalMs()` | `uint32_t` | Return the desired polling interval in milliseconds between successive `fetchUpdate()` calls. |
| `getSourceName()` | `const char*` | Return a human-readable identifier used in log output and EventBus topic names. |
| `getPriorityTier()` | `PriorityTier` | Return the scheduling priority tier used by DataManager to group sources. |
| `fetchesWhileConfiguring()` | `bool` | Opt-in to allow fetching while in `CONFIGURING` state (for sources that back preconditions). Default: `false`. |

### UpdateStatus Return Codes

Your `fetchUpdate()` implementation must return one of these result codes:

| Code | Value | Meaning |
|:---|:---|:---|
| `Success` | 0 | Data fetched and parsed successfully |
| `NoChange` | 1 | Server returned 304 or data is unchanged |
| `RateLimited` | 2 | Server returned 429 or local back-off triggered |
| `FetchError` | 3 | Network, DNS, TLS failure, or deadline expired |
| `ParseError` | 4 | Response received but JSON/payload parsing failed |
| `AuthError` | 5 | API key rejected (HTTP 401/403) or credentials missing |

### PriorityTier Scheduling Tiers

| Tier | Approximate Refresh Rate |
|:---|:---|
| `PriorityTier::High` | Every ~60 seconds |
| `PriorityTier::Normal` | Every ~300 seconds (5 minutes) |
| `PriorityTier::Low` | Every ~1800 seconds (30 minutes) |

---

## 2. The DataManager

The [DataManager](../../modules/DataManager/DataManager.hpp) is the framework's background coordinator that manages all registered `IDataSource` instances and asynchronous queued jobs. It runs on its own dedicated FreeRTOS worker task (Core 0, 12KB stack).

### Public API

| Method | Description |
|:---|:---|
| `begin()` | Initialize the FreeRTOS task, queue, and watchdog supervision. Called by `Application::begin()`. |
| `registerSource(IDataSource* source)` | Add a data source to the polling loop. |
| `unregisterSource(IDataSource* source)` | Remove a data source from the polling loop. |
| `requestPriorityFetch(IDataSource* source)` | Mark a source for immediate out-of-schedule refresh. |
| `queueDataJob(job, options)` | Enqueue a callable to execute off-loop on the worker task. |
| `getSourceHealth(source, out)` | Read point-in-time health snapshot for a source. |
| `resetSourceHealth(source)` | Clear failure history and resume normal scheduling for a suspended source. |
| `processNext()` | (Test only) Synchronous tick for unit testing — simulates one iteration of the worker task. |

---

## 3. Registration

There are two ways to register a data source with the DataManager:

### Option A — Via `registerModule()` (Recommended)

If your module implements `IDataSource` alongside other interfaces, the universal `registerModule()` call auto-detects it:

```cpp
// In setup()
app.registerModule(&myWeatherModule);
```

This is the preferred approach because it keeps all registration in one place.

### Option B — Direct DataManager Registration

For cases where you want to register a standalone `IDataSource` that is not a full module:

```cpp
app.getDataManager().registerSource(&myDataSource);
```

> **Note:** Both registration methods must be called in `setup()` before `app.begin()`.

---

## 4. Threading Model & Security

> [!IMPORTANT]
> **`fetchUpdate()` executes on the DataManager's FreeRTOS task, NOT the main tick loop.**

### Architecture

```
┌───────────────────────────┐     ┌──────────────────────────────────────────┐
│     Main Loop (Core 1)    │     │         DataManager Task (Core 0)        │
│                           │     │                                          │
│  app.tick()               │     │  workerTaskLoop()                        │
│    ├── myModule.tick()    │     │    ├── drainJobs(urgentOnly=true)        │
│    ├── display.tick()     │     │    ├── executeLoop()                     │
│    └── webServer.tick()   │     │    │     └── source->fetchUpdate(ctx)    │
│                           │     │    └── drainJobs(urgentOnly=false)       │
└───────────────────────────┘     └──────────────────────────────────────────┘
        ▲                                               │
        │           Cross-Task Shared State             │
        └───────────────────────────────────────────────┘
```

### Thread Safety Best Practices

1. **Scalars**: Use `std::atomic<float>`, `std::atomic<int>`, or `std::atomic<bool>` for simple values shared between `fetchUpdate()` and `tick()`.
2. **Strings & Structs**: Protect complex state with a FreeRTOS mutex (`xSemaphoreCreateMutex()`).
3. **Logging**: Use `ESP_LOGI` / `ESP_LOGW` for thread-safe logging. Do not call Arduino `Serial.print`.

### TLS & Certificate Verification

The framework automatically manages TLS client lifecycle via `ctx.client` (`FrameworkSecureClient`), attaching the ESP-IDF root certificate bundle.

> [!CAUTION]
> Do NOT allocate local `WiFiClientSecure` objects and do NOT call `client.setInsecure()`. Always use `*ctx.client` provided by `FetchContext`.

---

## 5. Example — Skeleton IDataSource Implementation

Here is a complete, minimal `IDataSource` implementation:

```cpp
// modules/WeatherFetcher/WeatherFetcher.hpp
#pragma once

#include "interfaces/ITickable.hpp"
#include "interfaces/IDataSource.hpp"
#include "types/FetchContext.hpp"
#include <cstdint>
#include <atomic>

#ifndef NATIVE_BUILD
#include <ArduinoJson.h>
#endif

class WeatherFetcher : public ITickable, public IDataSource {
public:
    void tick(uint32_t currentMillis) override;

    UpdateStatus  fetchUpdate(const FetchContext& ctx) override;
    uint32_t      getUpdateIntervalMs() const override { return 300000; } // 5 minutes
    const char*   getSourceName() const override { return "weather"; }
    PriorityTier  getPriorityTier() const override { return PriorityTier::Normal; }

    float getTemperature() const { return _temperature.load(std::memory_order_acquire); }

private:
    std::atomic<float> _temperature{0.0f};
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
    if (currentMillis - _lastDisplayUpdateMs >= 1000) {
        _lastDisplayUpdateMs = currentMillis;
        float temp = _temperature.load(std::memory_order_acquire);
        // Update display or publish to EventBus...
    }
}

#ifndef NATIVE_BUILD
UpdateStatus WeatherFetcher::fetchUpdate(const FetchContext& ctx) {
    if (!ctx.client) {
        return UpdateStatus::FetchError;
    }

    HTTPClient http;
    if (!http.begin(*ctx.client, "https://api.example.com/weather?city=london")) {
        return UpdateStatus::FetchError;
    }

    int httpCode = http.GET();
    if (httpCode != 200) {
        ESP_LOGW(TAG, "HTTP %d", httpCode);
        http.end();
        if (httpCode == 401 || httpCode == 403) return UpdateStatus::AuthError;
        if (httpCode == 429) return UpdateStatus::RateLimited;
        return UpdateStatus::FetchError;
    }

    // Check deadline before starting parse
    if (ctx.deadline.isExpired()) {
        http.end();
        return UpdateStatus::FetchError;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();

    if (err) {
        ESP_LOGW(TAG, "JSON parse error: %s", err.c_str());
        return UpdateStatus::ParseError;
    }

    _temperature.store(doc["temp"].as<float>(), std::memory_order_release);
    ESP_LOGI(TAG, "Fetched temperature: %.1f°C", _temperature.load(std::memory_order_relaxed));
    return UpdateStatus::Success;
}
#else
UpdateStatus WeatherFetcher::fetchUpdate(const FetchContext& ctx) {
    (void)ctx;
    return UpdateStatus::Success;
}
#endif
```

---

## 6. Queuing Asynchronous Data Jobs

To perform off-loop network calls (like an HTTP POST/PUT triggered by a web portal action), enqueue a job on `DataManager`:

```cpp
void MyModule::onConfigSaved() {
    app.getDataManager().queueDataJob([this](const FetchContext& ctx) {
#ifdef ESP_PLATFORM
        if (!ctx.client) return;
        HTTPClient http;
        if (http.begin(*ctx.client, "https://hub.local/api/setpoint")) {
            http.addHeader("Content-Type", "application/json");
            http.POST("{\"temp\": 22}");
            http.end();
        }
#endif
    }, "setTemperature");
}
```
