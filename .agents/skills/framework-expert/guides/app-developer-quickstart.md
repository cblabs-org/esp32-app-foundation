# App Developer Quickstart: Building a Custom Application

This guide walks you through building an application (like a weather station or sensor dashboard) on the ESP32 App Foundation. The framework uses a **Progressive Capability Model** — your module starts simple and gains capabilities by inheriting from specific interfaces.

> **Before you start**: Read the [Embedded Safety Constraints](../embedded-safety/memory-rules.md) to understand critical rules around memory allocation and thread safety.

## Step 1: The Basic Module Template

Every app starts as a basic module registered with the `Application` instance. There is no base `IModule` class. A module is defined by the interfaces it implements. Start with `ITickable` for periodic logic, then add more interfaces as your module's capabilities grow.

```cpp
#pragma once
#include "AppFoundation.h"

class MyApp : public ITickable {
public:
    void tick(uint32_t currentMillis) override {
        // Your periodic logic here
    }
};
```
*Next, register it in `src/main.cpp`:*
```cpp
Application app;
MyApp myApp;

void setup() {
    app.registerModule(&myApp);
    FrameworkConfig config;
    app.begin(config);
}

void loop() { app.tick(); }
```
*Read more: [Registration & Boot Sequence](../references/registration.md)*

## Step 2: Adding a Background Loop (`ITickable`)

To poll sensors or manage state, add `ITickable`.

```cpp
class WeatherStation : public ITickable {
public:
    void tick(uint32_t currentMillis) override {
        // Called every cycle. MUST NOT block for > 10ms!
    }
};
```
*Reference:* [`../interfaces/tickable.md`](../interfaces/tickable.md)

## Step 3: Saving User Settings (`IConfigurable`)

If your app needs API keys or preferences, implement `IConfigurable`.

```cpp
class WeatherStation : public IConfigurable {
public:
    void serializeConfig(JsonObject& section) const override {
        section["apiKey"] = m_apiKey;
    }
    void applyConfig(const JsonObject& section) override {
        m_apiKey = section["apiKey"] | "";
    }
};
```
*Reference:* [`../interfaces/configurable.md`](../interfaces/configurable.md)

## Step 4: Adding a Web UI (`IWebContributor`)

To let users view data or change settings on the portal, add a web tab.

```cpp
class WeatherStation : public IWebContributor {
public:
    void registerWebEndpoints(PsychicHttpServer* server) override {
        server->on("/api/weather", HTTP_GET, [this](PsychicRequest* req) {
            return req->reply(200, "application/json", "{\"temp\": 22}");
        });
    }
};
```
*Reference:* [`../interfaces/web-contributor.md`](../interfaces/web-contributor.md)

## Step 5: Background Fetching (`IDataSource`)

For slow network requests (like fetching a forecast), use `IDataSource` to offload work to a FreeRTOS background task.

```cpp
class WeatherStation : public IDataSource {
public:
    UpdateStatus performUpdate() override {
        // Runs in a background thread. Safe to block!
        return fetchForecast() ? UpdateStatus::SUCCESS : UpdateStatus::FAILED;
    }
};
```
*Reference:* [`../interfaces/data-source.md`](../interfaces/data-source.md) | *Thread Safety:* [`../embedded-safety/thread-safety.md`](../embedded-safety/thread-safety.md)

## Step 6: Showing Data on the Screen (`IDisplayBoard`)

To show a slide on the physical display carousel, implement `IDisplayBoard`.

```cpp
class WeatherBoard : public IDisplayBoard {
public:
    const char* getBoardName() const override { return "Weather"; }
    void render(U8G2& display) override {
        display.drawStr(0, 10, "22°C");
    }
};
// Register via CarouselEngine::get().registerBoard()
```
*Reference:* [`../interfaces/display-board.md`](../interfaces/display-board.md) | *System Guide:* [`../systems/display/custom-boards.md`](../systems/display/custom-boards.md)

## Step 7: Publishing System Events (`EventBus`)

Broadcast your data to other modules (like a screen saver or rules engine) via the EventBus.

```cpp
struct WeatherData { float temperature; };
WeatherData data{22.5f};
app.getEventBus().publish("weather.updated", &data);
```
*System Guide:* [`../systems/event-bus/custom-topics.md`](../systems/event-bus/custom-topics.md)

## What to Read Next

Once your prototype is working, review these deep dives before shipping:
- **[Performance & Optimization](../performance/overview.md)**: Ensure your app fits within the constrained ESP32 heap.
- **[Replacing the Display](../systems/display/replacing-the-display.md)**: Using TFTs or e-ink instead of OLED.
- **[Coding Patterns & Style Guide](../coding-patterns/overview.md)**: Ensure your app structure aligns with the framework's house style.
