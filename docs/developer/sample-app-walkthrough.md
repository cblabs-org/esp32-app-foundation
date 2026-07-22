# Sample App Walkthrough

> ESP32 App Foundation (c) 2026 CB Labs
>
> End-to-end guided tour of the Weather Station sample app. Learn the framework by reading real, working code — starting from a minimal skeleton and building up to a production module.

---

## Introduction

The `sample-app/` directory contains a **complete, functional Weather Station** built on the ESP32 App Foundation. It fetches current weather data from OpenWeatherMap, displays it on an OLED screen, exposes a full web portal tab with configuration UI, publishes Home Assistant sensor entities, and broadcasts events to other modules.

This guide walks through the sample app in two acts:

1. **Act 1 — The Skeleton:** The simple `MyApp` module teaches you the framework's core interfaces with minimal, readable stub code.
2. **Act 2 — The Real Thing:** The production `WeatherModule` shows what those same patterns look like at scale — with threading, streaming parsers, embedded JavaScript, and real API integration.

By the end, you'll understand how to build your own application on the framework.

### Prerequisites

- [PlatformIO](https://platformio.org/) installed (CLI or VS Code extension)
- An ESP32 development board (ESP32 DevKit or ESP32-S3 Nano)
- A free [OpenWeatherMap API key](https://openweathermap.org/appid) (for the weather features)
- Basic C++ and Arduino familiarity

### Getting Your Project Running

See [sample-app/README.md](../../sample-app/README.md) for step-by-step project setup — creating your repo, adding the framework as a Git submodule, copying the sample files, and adjusting `platformio.ini` paths.

---

## Project Structure Tour

The sample app lives alongside the framework in the same repository:

```
esp32-app-foundation/
├── include/                  # Framework public headers (AppFoundation.h)
├── modules/                  # Framework kernel modules (Application, ConfigManager, etc.)
├── lib/                      # Framework support libraries
├── sample-app/               # ← This is what we're walking through
│   ├── src/
│   │   ├── main.cpp          # Entry point — wiring and registration
│   │   ├── myApp.hpp/cpp     # Skeleton module (5 interfaces, stub logic)
│   │   ├── WeatherModule.hpp/cpp  # Production module (weather data)
│   │   ├── WeatherClockBoard.hpp/cpp  # OLED display board
│   │   └── WeatherWidget.hpp/cpp  # Display rendering helper
│   ├── data/                 # LittleFS filesystem assets
│   ├── platformio.ini        # Build configuration
│   └── README.md             # Setup instructions
```

### How the Framework Is Referenced

The sample app's `platformio.ini` uses relative paths because it lives *inside* the framework repo:

```ini
build_flags =
    -I ../include              # Framework public headers
lib_extra_dirs =
    ../modules                 # Framework kernel modules
    ../lib                     # Framework support libraries
lib_deps =
    Application                # The single framework entry point library
```

When you create your own downstream project using the Git submodule approach, these paths become `lib/esp32-app-foundation/include`, etc. — see [sample-app/README.md](../../sample-app/README.md) for details.

### Build Environments

The project defines three PlatformIO environments:

| Environment | Board | Purpose |
|---|---|---|
| `esp32dev` | ESP32 DevKit V1 | Primary development target |
| `esp32s3nano` | ESP32-S3 Nano | Secondary target with PSRAM |
| `native` | Host machine | Unit testing (no hardware) |

The `native` environment uses the `-DNATIVE_BUILD` flag, which is critical for the compilation guard pattern described later.

---

## The Entry Point: `main.cpp`

Every ESP32 App Foundation application starts in `main.cpp`. Let's walk through the sample app's entry point line by line.

### Global Module Instantiation

```cpp
#include "AppFoundation.h"          // Single framework façade — all interfaces included
#include "publicKey.hpp"            // OTA signing public key
#include "WeatherModule.hpp"
#include "WeatherClockBoard.hpp"

Application app;                    // The framework hub
WeatherModule weatherModule;        // Our main application module
WeatherClockBoard weatherClockBoard(&weatherModule);  // Display board
```

> [!IMPORTANT]
> **ADR-16: Static Scope Requirement.** All module instances MUST be declared at global or `static` scope. The ESP32 has no MMU — stack-allocated modules in `setup()` will create dangling pointers that silently corrupt memory when the framework calls `tick()` after `setup()` returns.

### The `setup()` Function

```cpp
void setup() {
    Serial.begin(115200);

    app.disableDefaultAppTab();         // We provide our own web tabs
    app.setAppName("Weather Station");  // Customise the portal title
```

**Dependency wiring** — the framework's kernel services are injected into the module via setter methods:

```cpp
    weatherModule.setHomeAssistant(&app.getHomeAssistant());
    weatherModule.setKeyManager(&app.getKeyManager());
    weatherModule.setEventBus(&app.getEventBus());
    weatherModule.setConfigManager(&app.getConfigManager());
    weatherModule.setDataManager(&app.getDataManager());
```

This is manual dependency injection — the module declares what services it needs, and `main.cpp` wires them up from the `Application` hub. More on this pattern in [§8 — Dependency Wiring Patterns](#8--dependency-wiring-patterns).

**Module registration** — a single call auto-detects all implemented interfaces:

```cpp
    app.registerModule(&weatherModule);
```

Behind the scenes, this uses `if constexpr` and `std::is_base_of_v` to detect that `WeatherModule` implements `ITickable`, `IConfigurable`, `IWebContributor`, and `IDataSource`, and registers it with each corresponding subsystem. See [Architecture Overview](architecture-overview.md) for the full registration mechanism.

**Framework configuration and boot:**

```cpp
    FrameworkConfig config;
    config.otaManifestUrl = "https://raw.githubusercontent.com/.../manifest.json";
    config.otaPublicKey = (const char*)public_key_pem;

    app.getWifiManager().setDefaultHostname("WeatherStation");

    app.getDisplay().carousel.add(&weatherClockBoard);  // Display board

    app.begin(config);  // Boot the framework
}
```

Note that the display board is registered **separately** via `app.getDisplay().carousel.add()` — it does not go through `registerModule()`. This is because `IDisplayBoard` is managed by the carousel engine, not the main registration pipeline.

### The `loop()` Function

```cpp
void loop() {
    app.tick();
}
```

That's it. The framework dispatches `tick()` to all registered `ITickable` modules, manages the display carousel, processes web requests, and runs background data fetches — all from this single call.

---

## Act 1: Learning the Framework — The `MyApp` Skeleton

The `MyApp` class in `template-app/src/myApp.hpp` is a **standalone, independently buildable starting point** — a minimal module that demonstrates all five primary interfaces with clear, stub logic. It lives in its own `template-app/` directory with its own `main.cpp` and `platformio.ini`, so you can copy the entire directory to begin your project.

> **Ready to start?** The `template-app/` directory includes its own `main.cpp` that registers `MyApp` — it builds and runs out of the box. Copy it as your starting point.

### Interface Overview

```cpp
class MyApp : public ITickable,
              public IConfigurable,
              public IWebContributor,
              public IMCPContributor,
              public IConfigGate {
```

Five interfaces, each adding a specific capability. Let's walk through them.

---

### 4.1 — Periodic Logic: `ITickable`

The simplest interface. Your module receives a `tick()` call on every main-loop iteration.

```cpp
void MyApp::tick(uint32_t currentMillis) {
    if (currentMillis - _lastActionMs >= (uint32_t)_refreshIntervalMs) {
        _lastActionMs = currentMillis;
        // Your periodic logic here
    }

    if (!_gateOpen) {
        _gateOpen = true;  // Open the config gate after first tick
    }
}
```

**Key patterns:**
- **Unsigned subtraction** (`currentMillis - _lastActionMs`) — correctly handles `millis()` overflow without special-casing.
- **10ms execution budget** — `tick()` MUST return within 10ms. The Task Watchdog Timer (TWDT) will panic and reboot the ESP32 if the main loop blocks for more than 5 seconds. For slow operations, use `IDataSource` instead.
- **Never use `delay()`** — it blocks the entire thread, halting WiFi, display, web server, and all other modules.

*Reference:* [Creating a Module — The Tick Loop](creating-a-module.md#3-implementing-the-module-logic)

---

### 4.2 — Saving Settings: `IConfigurable`

Persist module settings to flash (LittleFS) via the ConfigManager.

```cpp
const char* MyApp::getConfigNamespace() const { return "myapp"; }
int MyApp::getSchemaVersion() const { return 1; }

void MyApp::applyConfig(const JsonObject& config) {
    _refreshIntervalMs = config["refreshMs"] | 5000;    // Default: 5 seconds
    _featureEnabled = config["enabled"] | true;
}

void MyApp::serializeConfig(JsonObject& config) const {
    config["refreshMs"] = _refreshIntervalMs;
    config["enabled"] = _featureEnabled;
}

void MyApp::resetConfig() {
    _refreshIntervalMs = 5000;
    _featureEnabled = true;
}
```

**The contract:**

| Method | Purpose |
|---|---|
| `getConfigNamespace()` | Unique identifier — config stored at `/mod/myapp.json` |
| `getSchemaVersion()` | Version number for config migrations |
| `applyConfig()` | Called at boot with the loaded JSON object |
| `serializeConfig()` | Serialise current state for saving |
| `resetConfig()` | Restore factory defaults |

The `|` operator in `config["refreshMs"] | 5000` is ArduinoJson v7's "default value" syntax — if the key is missing or null, use the fallback.

*Reference:* [Configuration System](config-system.md)

---

### 4.3 — Adding a Web Tab: `IWebContributor`

Expose REST API endpoints and add a tab to the device's web portal.

```cpp
void MyApp::registerRoutes(PsychicHttpServer& server) {
    server.on("/api/myapp/status", HTTP_GET,
        [this](PsychicRequest* request, PsychicResponse* response) {
            JsonDocument doc;
            doc["status"] = "ok";
            doc["refreshMs"] = _refreshIntervalMs;
            doc["enabled"] = _featureEnabled;

            String output;
            serializeJson(doc, output);
            return response->send(200, "application/json", output.c_str());
        });
}

const char* MyApp::getTabName() const { return "MyApp"; }
int MyApp::getTabOrder() const { return 100; }
```

**Key points:**
- `registerRoutes()` is called **once** during boot — register all your routes here.
- Route handlers take `(PsychicRequest*, PsychicResponse*)` — use `response->send()` to reply.
- Auth enforcement for `/api/` paths is handled centrally — your handlers don't need to check authentication.
- Wrap the implementation in `#ifndef NATIVE_BUILD` / `#else` guards for host testing:

```cpp
#ifndef NATIVE_BUILD
void MyApp::registerRoutes(PsychicHttpServer& server) {
    // Real implementation
}
#else
void MyApp::registerRoutes(PsychicHttpServer& server) {
    (void)server;  // No-op for native builds
}
#endif
```

*Reference:* [Extending the Web Portal](extending-the-web-portal.md)

---

### 4.4 — Exposing MCP Tools: `IMCPContributor`

Register JSON-RPC tools that external clients (AI agents, scripts) can call.

```cpp
void MyApp::registerMCPTools(MCPToolRegistry& registry) {
    registry.addTool(
        "myapp_status",                            // Tool name
        "Get status and configuration of MyApp",   // Description
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* self = static_cast<MyApp*>(ctx);
            (void)params;
            result["refreshMs"] = self->_refreshIntervalMs;
            result["enabled"] = self->_featureEnabled;
            result["gateOpen"] = self->_gateOpen;
        },
        this,   // Context pointer — recovered via static_cast
        false   // mutatesState = false (read-only tool)
    );
}
```

**Key points:**
- Handlers are **static function pointers** (not `std::function` — see embedded safety constraints). Module state is passed through the opaque `void* ctx` parameter.
- Set `mutatesState = true` for tools that modify device state. The framework respects a read-only mode toggle that blocks mutating tools.

*Reference:* [MCP Server](mcp-server.md)

---

### 4.5 — Gating Boot: `IConfigGate`

Block the framework from entering the `RUNNING` state until your module is ready.

```cpp
const char* MyApp::getGateName() const { return "MyApp Ready"; }

bool MyApp::isSatisfied() const { return _gateOpen; }
```

While any gate returns `false`, the framework stays in `CONFIGURING` state — the WiFi AP captive portal remains active so the user can complete setup. Once all gates are satisfied, the framework transitions to `RUNNING`.

Real-world use cases: "WiFi credentials not yet configured", "API key not set", "sensor calibration incomplete".

---

### 5 — Registering Your Module

One call does everything:

```cpp
app.registerModule(&myApp);
```

The framework's template mechanism expands this to:

```cpp
if constexpr (std::is_base_of_v<ITickable, MyApp>)       → register as tickable
if constexpr (std::is_base_of_v<IConfigurable, MyApp>)   → register as configurable
if constexpr (std::is_base_of_v<IWebContributor, MyApp>) → register as web contributor
if constexpr (std::is_base_of_v<IMCPContributor, MyApp>) → register as MCP contributor
if constexpr (std::is_base_of_v<IConfigGate, MyApp>)     → register as config gate
// ... and so on for all 9 interfaces
```

**Critical rules:**
- Call `registerModule()` **before** `app.begin()`. The registration window closes during boot — late registration triggers a fatal panic.
- All modules at global/static scope (ADR-16).
- Don't call both `registerModule()` and individual registration methods — `registerModule()` does all the work.

---

## Act 2: The Real Thing — `WeatherModule` at Scale

Now let's see what a production module looks like. `WeatherModule` uses the same interfaces from Act 1, but adds real data fetching, thread safety, streaming JSON parsing, and an embedded web UI.

### Interface Inventory

```cpp
class WeatherModule : public ITickable,
                      public IConfigurable,
                      public IWebContributor,
                      public IDataSource,        // Background data fetching
                      public IMCPContributor,     // MCP tool registration
                      public IConfigGate,         // Boot readiness gating
                      public JsonListener {       // Streaming JSON parser (external library)
```

Notice what's **different** from `MyApp`:
- **Adds `IDataSource`** — for background network requests (OpenWeatherMap API)
- **Adds `JsonListener`** — not a framework interface; it's from the `JsonStreamingParser` library
- **All 7 app-facing interfaces demonstrated** — `WeatherModule` now implements every framework interface including `IMCPContributor` and `IConfigGate`

---

### 6.1 — Background Data Fetching: `IDataSource`

This is the most important difference from `MyApp`. While `ITickable::tick()` runs on the main loop and must return in 10ms, `IDataSource::fetchUpdate()` runs in a **dedicated FreeRTOS background task** — it's safe to block, make HTTP requests, and do heavy parsing.

```cpp
// IDataSource contract
UpdateStatus fetchUpdate() override;
uint32_t getUpdateIntervalMs() const override { return _pollingIntervalMs; }
const char* getSourceName() const override { return "OpenWeatherMap"; }
PriorityTier getPriorityTier() const override { return PriorityTier::Normal; }
```

The DataManager calls `fetchUpdate()` at the interval specified by `getUpdateIntervalMs()` from within its own task. Here's a simplified view of the implementation:

```cpp
UpdateStatus WeatherModule::fetchUpdate() {
    // 1. Check prerequisites
    std::string apiKey = _keyManager->getKeyById("owm_api_key");
    if (apiKey.empty()) return UpdateStatus::AuthError;
    if (_lat == 0.0 && _lon == 0.0) return UpdateStatus::NoChange;

    // 2. Create a secure HTTPS client (heap-safe allocation)
    std::unique_ptr<SecureClient> client(new (std::nothrow) SecureClient());
    if (!client) return UpdateStatus::FetchError;
    client->useDefaultCertBundle();  // Use ESP-IDF cert bundle, NOT setInsecure()

    // 3. Stream and parse the response (memory-efficient)
    HTTPClient http;
    http.begin(*client, url);
    int httpCode = http.GET();

    JsonStreamingParser parser;
    parser.setListener(this);  // WeatherModule implements JsonListener

    // Stream bytes through parser — no full JSON in RAM
    while (http.connected() && len > 0) {
        int c = stream->readBytes(buff, size);
        for (int i = 0; i < c; i++) parser.parse(buff[i]);
        vTaskDelay(pdMS_TO_TICKS(1));  // Yield to other tasks (not delay()!)
    }

    return UpdateStatus::Success;
}
```

**Production patterns demonstrated:**
- `new (std::nothrow)` — returns `nullptr` on allocation failure instead of throwing (ESP32 has no exception support)
- Certificate bundles — uses the ESP-IDF built-in certificate bundle instead of `setInsecure()` (which disables TLS verification entirely)
- `vTaskDelay()` — yields to other FreeRTOS tasks (never use `delay()` in a task context)
- Streaming JSON — parses the response byte-by-byte using `JsonStreamingParser`, avoiding loading the entire JSON response into RAM

**Thread-safe data exchange:**

Since `fetchUpdate()` runs in a background task but `tick()` and `render()` run on the main loop, data must be shared safely:

```cpp
// Numeric values — use std::atomic (lock-free, no mutex needed)
std::atomic<float> _temperature{0.0f};
std::atomic<float> _windSpeed{0.0f};
std::atomic<int>   _humidity{0};

// String values — use fixed char buffers protected by a FreeRTOS mutex
char _condition[32] = "";
SemaphoreHandle_t _dataMutex = nullptr;
```

Writing (in the background task):
```cpp
_temperature.store(temp, std::memory_order_release);

if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    strlcpy(_condition, parsed.c_str(), sizeof(_condition));
    xSemaphoreGive(_dataMutex);
}
```

Reading (on the main loop):
```cpp
float getTemperature() const {
    return _temperature.load(std::memory_order_acquire);
}

std::string getCondition() const {
    std::string result;
    if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        result = _condition;
        xSemaphoreGive(_dataMutex);
    }
    return result;
}
```

*Reference:* [Scheduling Data Fetches](scheduling-data-fetches.md) | [Embedded Safety](embedded-safety.md)

---

### 6.2 — The Web Portal Tab

`WeatherModule`'s `IWebContributor` implementation is more sophisticated than `MyApp`'s:

```cpp
const char* getTabName() const override { return "Weather"; }
int getTabOrder() const override { return 50; }  // Appears before MyApp's 100

// Custom SVG icon for the bottom navigation bar
const char* getTabIcon() const override {
    return "<path d=\"M17.5 19H9a7 7 0 1 1 6.71-9h1.79a4.5 4.5 0 1 1 0 9Z\"/>";
}
```

It registers four routes:
- `GET /api/weather/config` — returns config + API key status
- `POST /api/weather/config` — saves settings, stores API key via `KeyManager`, triggers immediate background fetch
- `GET /api/weather/status` — returns all current weather data
- `GET /weather.js` — serves a **complete embedded JavaScript web UI** (~300 lines)

The JavaScript UI is served from a raw string literal in the C++ source. It includes a weather summary card, configuration form, password-masked API key input with eye toggle, status badges, a diagnostics drawer, and auto-polling after save. This pattern — embedding the JS directly in the route handler — keeps everything self-contained without requiring separate web asset files.

---

### 6.3 — Home Assistant Integration & Events

`WeatherModule` publishes sensor data to Home Assistant and broadcasts events via the EventBus:

```cpp
// H2: Static member allocation — sensors are direct members, no `new`
HaSensor _tempSensor{"weather_temp", "Temperature"};
HaSensor _windSensor{"weather_wind", "Wind Speed"};
HaSensor _condSensor{"weather_condition", "Condition"};
```

After a successful fetch, the parsed data is published:

```cpp
_tempSensor.setState(tempString);
if (_ha) _ha->publishState(&_tempSensor);

// Notify other modules via EventBus
if (_eventBus) {
    _eventBus->publish("weather.updated");
}
```

Other modules (like the display board) can subscribe to `"weather.updated"` to refresh their UI when new data arrives.

*Reference:* [Event Bus](event-bus.md)

---

### 6.4 — MCP Tool Registration (`IMCPContributor`)

`WeatherModule` exposes a `weather_status` tool that AI agents and scripts can query via the MCP JSON-RPC transport:

```cpp
void WeatherModule::registerMCPTools(MCPToolRegistry& registry) {
    registry.addTool(
        "weather_status",
        "Get current weather data including temperature, condition, humidity, "
        "wind speed, pressure, and location",
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* self = static_cast<WeatherModule*>(ctx);
            (void)params;
            result["hasData"] = self->hasData();
            if (self->hasData()) {
                result["temperature"] = self->getTemperature();
                result["condition"] = self->getCondition();
                result["humidity"] = self->getHumidity();
                result["windSpeed"] = self->getWindSpeed();
                result["city"] = self->getCityName();
            }
            result["units"] = self->_units;
        },
        this,
        false  // read-only
    );
}
```

This exposes the same data as `GET /api/weather/status` but via the MCP JSON-RPC transport — queryable by AI agents, automation scripts, and the device MCP client skill.

*Reference:* [MCP Server](mcp-server.md)

---

### 6.5 — Boot Gating (`IConfigGate`)

The weather module gates the CONFIGURING → RUNNING transition on having an API key configured:

```cpp
const char* WeatherModule::getGateName() const {
    return "Weather API Key";
}

bool WeatherModule::isSatisfied() const {
    if (!_keyManager) return true;  // No KeyManager wired — don't block boot
    return !_keyManager->getKeyById("owm_api_key").empty();
}

const char* WeatherModule::getHelpText() const {
    return "Enter your OpenWeatherMap API key via the Weather tab";
}
```

On first flash, the device stays in `CONFIGURING` mode with the captive portal active until the user enters an OWM API key via the Weather tab. The `getHelpText()` message is displayed on the system status board to guide the user. Once the key is stored, the gate opens and the framework transitions to `RUNNING`.

The defensive `if (!_keyManager) return true` ensures the module never blocks boot if the dependency wasn't wired — fail-open, not fail-closed.

---

## 7 — Display Boards: `WeatherClockBoard`

The sample app includes a custom OLED display board that renders weather data on the physical screen carousel.

### The `IDisplayBoard` Contract

```cpp
class WeatherClockBoard : public IDisplayBoard {
private:
    WeatherModule* _weatherModule;

public:
    explicit WeatherClockBoard(WeatherModule* weatherModule);

    const char* getBoardName() const override { return "WeatherClock"; }
    bool isExpired() const override { return false; }  // Permanent board

    void render(U8G2& u8g2) override;
};
```

**Contract methods:**
- `render(U8G2&)` — draw a full-screen frame into the display buffer
- `getBoardName()` — human-readable identifier for logging
- `isExpired()` — return `true` to remove this board from the carousel (transient boards); `false` for permanent boards

**Additional lifecycle hooks** (default no-op, override as needed):
- `onActivate()` — called when this board becomes the active display
- `onDeactivate()` — called when this board rotates off-screen
- `tick(uint32_t)` — per-frame timing for animations

### Rendering

The board reads data from `WeatherModule` via thread-safe getters and delegates drawing to a stateless utility:

```cpp
void WeatherClockBoard::render(U8G2& u8g2) {
    std::string tempStr;
    std::string condStr;

    if (_weatherModule) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", _weatherModule->getTemperature());
        tempStr = buf;
        condStr = _weatherModule->getCondition();
    }

    WeatherWidget::draw(u8g2, 10, 40, tempStr, condStr);
}
```

The `WeatherWidget` maps condition strings to weather icon glyphs and renders the temperature and condition text.

### Registration

Display boards are registered via the carousel engine, **not** via `registerModule()`:

```cpp
// In main.cpp setup()
app.getDisplay().carousel.add(&weatherClockBoard);
```

This is wrapped in `#ifndef NATIVE_BUILD` because the display subsystem is hardware-only.

*Reference:* [Using Optional Modules](using-optional-modules.md)

---

## 8 — Dependency Wiring Patterns

The sample app demonstrates two distinct wiring mechanisms:

### Interface Registration (automatic)

```cpp
app.registerModule(&weatherModule);
```

This auto-detects which *interfaces* the module implements and registers it with the corresponding framework subsystems (tick loop, config manager, web server, data manager, etc.).

### Service Injection (manual)

```cpp
weatherModule.setHomeAssistant(&app.getHomeAssistant());
weatherModule.setKeyManager(&app.getKeyManager());
weatherModule.setEventBus(&app.getEventBus());
weatherModule.setConfigManager(&app.getConfigManager());
weatherModule.setDataManager(&app.getDataManager());
```

The module declares setter methods for the framework services it needs. `main.cpp` wires them up from the `Application` hub.

**Available framework services:**

| Getter | Service |
|---|---|
| `app.getEventBus()` | Pub/sub event system |
| `app.getKeyManager()` | Secure credential storage |
| `app.getConfigManager()` | Config persistence |
| `app.getDataManager()` | Background data fetch scheduling |
| `app.getHomeAssistant()` | Home Assistant MQTT integration |
| `app.getWifiManager()` | WiFi configuration |
| `app.getDisplay()` | Display subsystem (carousel, screensaver) |

### Why Two Mechanisms?

`registerModule()` handles *contracts* — "this module is tickable, configurable, web-contributing." But it doesn't know *which* framework services your module needs at runtime. A weather module needs `KeyManager` for API key storage; a sensor module might not. The setter pattern keeps dependencies explicit and avoidable.

---

## 9 — Building & Flashing

### Build

```bash
# Build for both default environments
pio run

# Build for a specific target
pio run -e esp32dev

# Run native unit tests
pio test -e native
```

### Upload Web Portal Assets

The web portal SPA lives on the LittleFS filesystem:

```bash
pio run -t uploadfs -e esp32dev
```

### Flash Firmware

```bash
pio run -t upload -e esp32dev
```

*Reference:* [Build System](build-system.md)

---

## 10 — What to Build Next

### Start From the Skeleton

1. Copy the entire `template-app/` directory as your project starting point
2. Rename the `MyApp` class to match your application
3. Remove the interfaces you don't need
4. Add your domain logic

### Progressive Capability Model

Start simple and add interfaces as your module grows:

| Stage | Interface | What It Adds |
|---|---|---|
| 1 | `ITickable` | Periodic main-loop logic |
| 2 | `IConfigurable` | Persistent settings |
| 3 | `IWebContributor` | Web portal tab and REST API |
| 4 | `IDataSource` | Background network requests |
| 5 | `IDisplayBoard` | OLED display pane |
| 6 | `IMCPContributor` | Device MCP tools |
| 7 | `IConfigGate` | Boot-time readiness gating |

### Developer Checklist

Before shipping, verify:

- [ ] All modules declared at global/static scope (ADR-16)
- [ ] All `registerModule()` calls happen before `app.begin()`
- [ ] `tick()` completes within 10ms — no `delay()`, no blocking I/O
- [ ] Hardware code wrapped in `#ifndef NATIVE_BUILD`
- [ ] No `new` without `std::nothrow` — use `std::unique_ptr` for RAII
- [ ] Thread-safe data exchange for `IDataSource` modules
- [ ] Config namespace is unique and lowercase
- [ ] All public methods have Doxygen comments

### Further Reading

Continue with the deep-dive guides in order:

- [Creating a Module](creating-a-module.md) — detailed interface-by-interface tutorial
- [Coding Patterns](coding-patterns.md) — SOLID principles, house style, anti-patterns
- [Configuration System](config-system.md) — migrations, schema versioning, bulk export
- [Extending the Web Portal](extending-the-web-portal.md) — HTML fragments, JavaScript patterns, authentication
- [Event Bus](event-bus.md) — pub/sub, custom topics, framework events
- [Scheduling Data Fetches](scheduling-data-fetches.md) — threading model, WiFiClientSecure placement
- [Embedded Safety](embedded-safety.md) — thread safety, memory rules, design graveyard

---

## 11 — Framework-Expert Skill: Known Discrepancies

> [!WARNING]
> The `framework-expert` AI agent skill (`.agents/skills/framework-expert/`) contains code examples that conflict with the actual codebase. If you are using AI agents to build on the framework, be aware of these discrepancies. They are flagged for correction in a follow-up update.

| # | Skill Location | Claim | Actual Code | Severity |
|---|---------------|-------|-------------|----------|
| 1 | `app-developer-quickstart.md` §4 | Uses `registerWebEndpoints(PsychicHttpServer* server)` | Actual method is `registerRoutes(PsychicHttpServer& server)` | 🔴 Breaking — wrong name and pointer vs reference |
| 2 | `app-developer-quickstart.md` §4 | Uses `req->reply(200, ...)` | Handler takes `(PsychicRequest*, PsychicResponse*)`, uses `response->send()` | 🔴 Breaking — wrong handler signature |
| 3 | `app-developer-quickstart.md` §5 | Uses `performUpdate()` | Actual method is `fetchUpdate()` | 🔴 Breaking — wrong method name |
| 4 | `app-developer-quickstart.md` §6 | `CarouselEngine::get().registerBoard()` | Actual: `app.getDisplay().carousel.add(&board)` | 🟡 Misleading API |
| 5 | `references/architecture.md` | App code at `template/src/` | Directory is now `sample-app/src/` | 🟡 Stale path |
| 6 | SKILL.md interface table | Correct `registerRoutes` name | But the quickstart example contradicts this | 🟡 Internal inconsistency |
| 7 | Sample app `myApp.hpp` | Includes individual interface headers | Skill says "always use `#include "AppFoundation.h"`" | 🟡 Code violates own advice |
| 8 | `extending-the-web-portal.md` | `StaticJsonDocument<1024>` | ✅ Fixed — uses `JsonDocument` (ArduinoJson v7) | ✅ Resolved |
