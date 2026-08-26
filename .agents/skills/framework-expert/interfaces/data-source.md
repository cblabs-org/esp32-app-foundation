# IDataSource

Contract for background data-fetch workers managed by `DataManager`.

## Overview

Unlike `ITickable` which runs on the main core with strict time budgets (10ms tick budget), `IDataSource` fetches execute in a dedicated FreeRTOS worker task (Core 0, 12KB stack), making them suitable for long-running blocking operations like HTTP requests.

```cpp
class IDataSource {
public:
    virtual ~IDataSource() = default;
    virtual UpdateStatus fetchUpdate(const FetchContext& ctx) = 0;
    virtual uint32_t getUpdateIntervalMs() const = 0;
    virtual const char* getSourceName() const = 0;
    virtual PriorityTier getPriorityTier() const = 0;
    virtual bool fetchesWhileConfiguring() const { return false; }
};
```

---

## 1. The `FetchContext` Contract

`fetchUpdate()` receives a `const FetchContext& ctx` from `DataManager`:

```cpp
struct FetchContext {
    DeadlineToken& deadline;

    static constexpr uint32_t FETCH_TOTAL_TIMEOUT_MS = 15000; ///< 15s total budget
    static constexpr uint32_t CONNECT_TIMEOUT_MS     = 5000;  ///< 5s connection budget
    static constexpr uint32_t DNS_TIMEOUT_MS         = 5000;  ///< 5s DNS resolution budget
    static constexpr uint32_t FAR_OUTER_BACKSTOP_S   = 120;   ///< 120s TWDT backstop watchdog

#ifdef ESP_PLATFORM
    WiFiClientSecure* client = nullptr; ///< Framework-owned TLS client with cert bundle
#endif
};
```

### `DeadlineToken` API

- `ctx.deadline.isExpired()`: Check periodically during streaming I/O or JSON parsing loops.
- `ctx.deadline.isCancelled()`: Check if execution was cancelled.
- `ctx.deadline.remainingMs()`: Milliseconds remaining before deadline.

### Ownership Split

- **Framework owns scheduling**: When to call `fetchUpdate()`, backoff timers, health transitions, TWDT watchdog supervision, and priority fetching.
- **Consumer owns transport**: What happens inside one fetch. Consumers MUST NOT build custom scheduling backoff or sleep loops.

---

## 2. Enums & Types

### `UpdateStatus`

Returned by `fetchUpdate()` to indicate the outcome:
- `Success`: Fetched and parsed successfully.
- `NoChange`: Server returned 304 or data unchanged.
- `RateLimited`: Server returned 429 or local backoff applied.
- `FetchError`: Network drop, DNS failure, TLS handshake error, or deadline expired.
- `ParseError`: Response received but JSON/payload parsing failed.
- `AuthError`: API key rejected (HTTP 401/403) or missing credentials.

### `PriorityTier`

Determines scheduling grouping:
- `High`: ~60 seconds
- `Normal`: ~300 seconds (5 mins)
- `Low`: ~1800 seconds (30 mins)

### `SourceHealthState`

- `Unknown` (0): Registered but never attempted, or freshly reset.
- `Healthy` (1): Last fetch succeeded.
- `Degraded` (2): Failing and backing off (30s base doubling up to 15m). Retries automatically.
- `Quarantined` (3): At the 15-minute backoff cap. Still probes every 15 minutes to allow automatic self-healing.
- `Suspended` (4): Hard failure (`AuthError`). Automatic scheduling stops. Exit via `DataManager::resetSourceHealth()` or priority fetch.

---

## 3. Threading Model & Stack Placement

`fetchUpdate()` executes in a FreeRTOS task outside the main `loop()`.

> [!CAUTION]
> You must ensure thread safety when sharing data between `fetchUpdate()` (the FreeRTOS task) and other framework interfaces like `IWebContributor` or `ITickable` (the main task). Use `std::atomic`, mutexes, or `EventBus` payloads to cross thread boundaries safely.

> [!WARNING]
> **Streaming parser callbacks run on the background task.** If your `IDataSource`
> implementation also implements a streaming parser callback interface (e.g.,
> `JsonListener`), the parser callbacks (`key()`, `value()`, `endDocument()`) will
> execute inside `fetchUpdate()` on the DataManager's background task. Any class
> members written by these callbacks and read by main-thread interfaces (`ITickable`,
> `IWebContributor`, `IDisplayBoard`) require thread-safe access patterns.
>
> For numeric values, use `std::atomic<float>` or `std::atomic<int>`. For strings,
> use a fixed `char[]` buffer protected by a FreeRTOS mutex (`xSemaphoreCreateMutex()`).
> See [thread-safety.md](../embedded-safety/thread-safety.md) for recommended approaches.

### TLS Client Placement (ADR-14 & PR #128)

The framework supplies a pre-configured, ESP-IDF certificate bundle-attached `WiFiClientSecure` via `ctx.client`.
Consumers do **NOT** instantiate local `WiFiClientSecure` objects and MUST NOT call `setInsecure()`.

```cpp
UpdateStatus MySource::fetchUpdate(const FetchContext& ctx) {
#ifdef ESP_PLATFORM
    if (!ctx.client) {
        return UpdateStatus::FetchError;
    }
    HTTPClient http;
    if (!http.begin(*ctx.client, "https://api.example.com/data")) {
        return UpdateStatus::FetchError;
    }
#endif

    int httpCode = http.GET();
    if (httpCode != 200) {
        http.end();
        if (httpCode == 401 || httpCode == 403) return UpdateStatus::AuthError;
        if (httpCode == 429) return UpdateStatus::RateLimited;
        return UpdateStatus::FetchError;
    }

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buff[128];
    while (http.connected()) {
        if (ctx.deadline.isExpired()) {
            http.end();
            return UpdateStatus::FetchError;
        }
        // ... parse stream chunks ...
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    http.end();
    return UpdateStatus::Success;
}
```

---

## 4. Multi-Surface Health Surfacing

DataManager surfaces source health across five channels:
1. **EventBus**: `ApplicationEvents::DATA_SOURCE_HEALTH` (`"data.source.health"`) with payload `const DataSourceHealthPayload*`.
2. **REST API**: `GET /api/data/sources` and `POST /api/data/sources/reset`.
3. **Web Portal**: Data Sources card on Status tab with live state badges and reset button.
4. **Display**: Deferred subscription in `U8G2DisplayModule` and `LVGLDisplayModule`.
5. **MCP Server**: `get_data_sources` tool for AI agents and diagnostics.

---

## 5. Queuing Background Data Jobs

To execute one-off asynchronous network operations on the DataManager worker task:

```cpp
app.getDataManager().queueDataJob([this](const FetchContext& ctx) {
#ifdef ESP_PLATFORM
    if (!ctx.client) return;
    HTTPClient http;
    if (http.begin(*ctx.client, "https://api.example.com/command")) {
        http.POST("{\"cmd\":\"run\"}");
        http.end();
    }
#endif
}, "runCommand");
```
