# Migrating to `fetchUpdate(const FetchContext&)`

> ESP32 App Foundation (c) 2026 CB Labs
>
> A comprehensive guide for migrating custom application modules, downstream repositories, and test mocks to the breaking `IDataSource::fetchUpdate()` and `DataManager::queueDataJob()` APIs introduced in PR #128.

---

## 1. Executive Summary

In ESP32 App Foundation v0.19.0+, the `IDataSource` interface and `DataManager` job queues received breaking API changes to resolve watchdog timeout crashes and device reboots caused by stalled or failing external data sources (Issue #128).

### The Breaking Changes

1. **`IDataSource::fetchUpdate()`**:
   - **Before**: `virtual UpdateStatus fetchUpdate() = 0;`
   - **After**: `virtual UpdateStatus fetchUpdate(const FetchContext& ctx) = 0;`
2. **`DataManager::queueDataJob()`**:
   - **Before**: `bool queueDataJob(std::function<void()> job, ...);`
   - **After**: `bool queueDataJob(std::function<void(const FetchContext&)> job, ...);`

### Why This Changed

- **Watchdog Protection**: Long fetches now run with cooperative cancellation (`DeadlineToken`) and dynamic FreeRTOS Task Watchdog Timer (TWDT) widening.
- **Centralized Security**: The framework now supplies a pre-configured `WiFiClientSecure` configured with the ESP-IDF certificate bundle (`ctx.client`). Consumers no longer need to allocate their own TLS clients or bypass TLS verification (`setInsecure()`).
- **Resilience**: Failures degrade the source with automatic exponential backoff (30s base, 15m cap) rather than causing watchdog resets.

---

## 2. Migration Options

Downstream consumers have two migration paths:

### Path A: Minimum Viable Migration (5 minutes)

If you have existing code and want to compile immediately without refactoring internal fetch logic:

1. Update your module header (`.hpp`):
   ```cpp
   // Before
   UpdateStatus fetchUpdate() override;

   // After
   UpdateStatus fetchUpdate(const FetchContext& ctx) override;
   ```
2. Update your implementation (`.cpp`):
   ```cpp
   // Before
   UpdateStatus MyModule::fetchUpdate() {
       // ... existing code ...
   }

   // After
   UpdateStatus MyModule::fetchUpdate(const FetchContext& ctx) {
       (void)ctx; // Suppress unused parameter warning
       // ... existing code unchanged ...
   }
   ```

> [!WARNING]
> **Path A is transitional. Without polling ctx.deadline.isExpired(), your source relies on the framework's transport timeouts (~15s). Migrate to Path B as soon as possible.**

### Plain-HTTP Consumers

`ctx.client` is a `WiFiClientSecure*` pre-configured for TLS. If your source requires plain HTTP (e.g. communicating with local unencrypted network devices), you must retain your own `WiFiClient` allocation. However, you should still poll `ctx.deadline.isExpired()` during parsing loops to participate in the cooperative timeout, and use `resolveHostBounded` for DNS if doing raw socket work.

---

### Path B: Recommended Full Migration

The recommended migration unlocks full resilience, reduces DRAM consumption, and ensures robust TLS validation.

#### Step 1: Update Class Header

Ensure `#include "AppFoundation.h"` is present. Update the override in your class definition:

```cpp
class MyWeatherSource : public ITickable, public IDataSource {
public:
    UpdateStatus fetchUpdate(const FetchContext& ctx) override;
    // ...
};
```

#### Step 2: Adopt Framework TLS Client (`ctx.client`)

Delete locally instantiated `WiFiClientSecure` instances and use `*ctx.client` provided in `FetchContext`:

```cpp
// ❌ BEFORE: Local client allocation and insecurity
WiFiClientSecure client;
client.setInsecure(); // Unsafe!
HTTPClient http;
http.begin(client, url);

// ✅ AFTER: Use framework-provided secure client
#ifdef ESP_PLATFORM
if (!ctx.client) {
    return UpdateStatus::FetchError;
}
HTTPClient http;
if (!http.begin(*ctx.client, url)) {
    return UpdateStatus::FetchError;
}
#endif
```

#### Step 3: Add Deadline Checks in Streaming / Parse Loops

If your module reads chunked payloads or uses streaming JSON parsers (`JsonStreamingParser`, `ArduinoJson`), check `ctx.deadline.isExpired()` during the loop:

```cpp
WiFiClient* stream = http.getStreamPtr();
uint8_t buff[128];

while (http.connected() && (len > 0 || len == -1)) {
    // Check cooperative deadline
    if (ctx.deadline.isExpired()) {
        ESP_LOGE(TAG, "Fetch deadline expired — aborting");
        http.end();
        return UpdateStatus::FetchError;
    }

    size_t size = stream->available();
    if (size) {
        int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
        for (int i = 0; i < c; i++) {
            parser.parse(buff[i]);
        }
        if (len > 0) len -= c;
    }
    vTaskDelay(pdMS_TO_TICKS(1)); // Yield to other tasks
}
```

#### Step 4: Delete Custom Scheduling & Backoff Logic

Remove any custom retry counters, backoff timers, or sleep loops in your module. The framework's `DataManager` automatically tracks consecutive failures and applies exponential backoff (30s doubling up to 15m).

#### Step 5: What to Delete

Review your module and remove the following legacy workarounds:
- 🗑️ `client.setInsecure()` calls
- 🗑️ Custom `esp_crt_bundle_attach()` calls in consumer code
- 🗑️ Stack or class-member `WiFiClientSecure` allocations
- 🗑️ Custom retry / backoff state variables (`_failureCount`, `_nextRetryMs`)

---

## 3. Reference Example: Before & After

Here is a side-by-side comparison based on the reference `WeatherModule` implementation:

### Before (v0.18.0 and earlier)

```cpp
UpdateStatus WeatherModule::fetchUpdate() {
    std::string apiKey = _keyManager ? _keyManager->getKeyById("owm_api_key") : "";
    if (apiKey.empty()) return UpdateStatus::AuthError;

    char url[256];
    snprintf(url, sizeof(url), "https://api.openweathermap.org/data/2.5/weather?lat=%f&lon=%f&appid=%s",
             _lat, _lon, apiKey.c_str());

    // Legacy: Stack allocation with setInsecure or custom cert bundle attachment
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    if (!http.begin(client, url)) {
        return UpdateStatus::FetchError;
    }

    int httpCode = http.GET();
    if (httpCode != 200) {
        http.end();
        return (httpCode == 429) ? UpdateStatus::RateLimited : UpdateStatus::FetchError;
    }

    WiFiClient* stream = http.getStreamPtr();
    JsonStreamingParser parser;
    parser.setListener(this);
    
    // Potentially blocks loopTask or worker if network stalls
    while (http.connected()) {
        if (stream->available()) {
            parser.parse(stream->read());
        }
    }
    http.end();
    return UpdateStatus::Success;
}
```

### After (v0.19.0+)

```cpp
UpdateStatus WeatherModule::fetchUpdate(const FetchContext& ctx) {
    std::string apiKey = _keyManager ? _keyManager->getKeyById("owm_api_key") : "";
    if (apiKey.empty()) return UpdateStatus::AuthError;

    char url[256];
    snprintf(url, sizeof(url), "https://api.openweathermap.org/data/2.5/weather?lat=%f&lon=%f&appid=%s",
             _lat, _lon, apiKey.c_str());

#ifdef ESP_PLATFORM
    if (!ctx.client) {
        ESP_LOGE("WeatherModule", "FetchContext provided null client");
        return UpdateStatus::FetchError;
    }
#endif

    HTTPClient http;
#ifdef ESP_PLATFORM
    if (!http.begin(*ctx.client, url)) {
        return UpdateStatus::FetchError;
    }
#else
    if (false) {}
#endif

    int httpCode = http.GET();
    if (httpCode != 200) {
        http.end();
        if (httpCode == 401 || httpCode == 403) return UpdateStatus::AuthError;
        if (httpCode == 429) return UpdateStatus::RateLimited;
        return UpdateStatus::FetchError;
    }

    WiFiClient* stream = http.getStreamPtr();
    JsonStreamingParser parser;
    parser.setListener(this);

    uint8_t buff[128];
    while (http.connected()) {
        // Cooperative deadline check
        if (ctx.deadline.isExpired()) {
            ESP_LOGE("WeatherModule", "Fetch deadline expired — aborting parse");
            http.end();
            return UpdateStatus::FetchError;
        }

        size_t size = stream->available();
        if (size) {
            int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
            for (int i = 0; i < c; i++) {
                parser.parse(buff[i]);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    http.end();
    return UpdateStatus::Success;
}
```

---

## 4. Migrating Queued Data Jobs (`queueDataJob`)

If your module dispatches asynchronous tasks using `DataManager::queueDataJob()`, update the lambda signature to accept `const FetchContext& ctx`:

### Before

```cpp
dataManager.queueDataJob([this]() {
    HTTPClient http;
    http.begin("https://api.mycloud.com/v1/telemetry");
    http.POST(payload);
    http.end();
}, "cloudTelemetry");
```

### After

```cpp
dataManager.queueDataJob([this](const FetchContext& ctx) {
#ifdef ESP_PLATFORM
    if (!ctx.client) return;
    HTTPClient http;
    if (http.begin(*ctx.client, "https://api.mycloud.com/v1/telemetry")) {
        http.POST(payload);
        http.end();
    }
#endif
}, "cloudTelemetry");
```

---

## 5. Updating Test Mocks

In native test suites (`test/test_unit/` or host GoogleTest runners):

### Mock Data Source

Update mock implementations of `IDataSource` to accept `const FetchContext&`:

```cpp
class MockDataSource : public IDataSource {
public:
    UpdateStatus fetchResult = UpdateStatus::Success;
    uint32_t fetchCallCount = 0;

    UpdateStatus fetchUpdate(const FetchContext& ctx) override {
        (void)ctx;
        fetchCallCount++;
        return fetchResult;
    }

    uint32_t getUpdateIntervalMs() const override { return 1000; }
    const char* getSourceName() const override { return "MockSource"; }
    PriorityTier getPriorityTier() const override { return PriorityTier::Normal; }
};
```

### Calling `fetchUpdate` Directly in Tests

When constructing a `FetchContext` manually in a unit test:

```cpp
#include "types/FetchContext.hpp"

TEST(DataManagerTest, ManualFetchInvocation) {
    DeadlineToken token(millis() + FetchContext::FETCH_TOTAL_TIMEOUT_MS);
    FetchContext ctx{token};

    MockDataSource source;
    UpdateStatus status = source.fetchUpdate(ctx);
    EXPECT_EQ(status, UpdateStatus::Success);
}
```
