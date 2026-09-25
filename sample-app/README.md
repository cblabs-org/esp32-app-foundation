# Weather Station — ESP32 App Foundation Canonical Sample App

A complete, production-grade weather station built on the **ESP32 App Foundation** framework. This sample demonstrates how to consume the framework's core interfaces, lifecycle patterns, and UI systems to build a robust, maintainable embedded application.

---

## Architecture Summary

The sample centres on `WeatherModule`, which models an external cloud API integration and exposes data across multiple framework layers:

```
                          ┌───────────────────────┐
                          │   OpenWeatherMap API  │
                          └───────────┬───────────┘
                                      │ HTTPS GET
                                      ▼
┌─────────────────────────────────────────────────────────────────────────┐
│ WeatherModule                                                           │
│                                                                         │
│  IDataSource ──────────────► DataManager (Background FreeRTOS task)     │
│                              • ArduinoJson DeserializationFilter        │
│                              • Thread-safe atomic / mutex storage       │
│                                                                         │
│  ITickable ────────────────► Main Loop (EventBus publish: weather.updated)
│                                                                         │
│  IConfigurable ────────────► ConfigManager (NVS / LittleFS persistence) │
│                                                                         │
│  IPreconditionContributor ──► PreconditionEngine                        │
│                              • Startup: "Weather API Key"               │
│                              • Startup: "Weather Location"              │
│                              • Operational: "Weather API"               │
│                                                                         │
│  IWebContributor ──────────► Web Portal (PsychicHttp REST + weather.js) │
│                                                                         │
│  IMCPContributor ──────────► Model Context Protocol (weather_status)    │
│                                                                         │
│  IWeatherDataProvider ─────┐ (Decoupled data access interface)          │
└────────────────────────────┼────────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────────┐
│ Display Subsystem                                                       │
│                                                                         │
│  WeatherBoard (IDisplayBoard)                                           │
│  └── WeatherWidget (Dynamic centering, icon glyphs, LabelWidgets)       │
└─────────────────────────────────────────────────────────────────────────┘
```

### Framework Interfaces Implemented

| Interface | Implementor | Role in Sample |
|---|---|---|
| `IDataSource` | `WeatherModule` | Periodic background HTTPS fetch via `DataManager`, using framework-managed TLS and deadlines. |
| `IConfigurable` | `WeatherModule` | Persisted configuration (`lat`, `lon`, `interval`, `units`) with namespace `"weather"`, versioning, and JSON serialization. |
| `IPreconditionContributor` | `WeatherModule` | Declares system dependencies: API key and coordinates as Startup gates, API connectivity as an Operational gate. |
| `IWebContributor` | `WeatherModule` | Exposes REST endpoints (`/api/weather/config`, `/api/weather/status`) and registers the "Weather" portal tab serving `weather.js`. |
| `IMCPContributor` | `WeatherModule` | Registers AI-callable MCP tools (`weather_status`) for inspection and diagnostics. |
| `ITickable` | `WeatherModule` | Main-thread update hook for dispatching deferred `EventBus` notifications. |
| `IWeatherDataProvider` | `WeatherModule` | Abstract interface providing thread-safe getters (`float` atomics, mutexed strings), cleanly decoupling the UI from data fetching. |
| `IDisplayBoard` | `WeatherBoard` | Carousel screen rendering current weather conditions onto OLED displays. |

---

## Preconditions Architecture

The sample demonstrates fine-grained precondition gating using the `PreconditionEngine`:

1. **Weather API Key** (`PreconditionRole::Startup`):
   - Depends on: `WiFiStation`
   - Gate: Requires an OpenWeatherMap API key stored in `KeyManager`.
   - Effect: Prevents boot completion to `RUNNING` if missing; prompts the user on the display and portal during `CONFIGURING`.
2. **Weather Location** (`PreconditionRole::Startup`):
   - Depends on: `WiFiStation`
   - Gate: Requires valid latitude and longitude coordinates in configuration.
   - Effect: Prevents boot completion to `RUNNING` if unconfigured.
3. **Weather API** (`PreconditionRole::Operational`):
   - Depends on: `"Weather API Key"` and `"Weather Location"`.
   - Gate: Verifies successful HTTP communication with OpenWeatherMap.
   - Effect: Degrades gracefully at runtime if network issues, rate limits, or bad keys occur, without rebooting the device.

---

## Data Flow

1. **Scheduling**: `DataManager` periodically invokes `WeatherModule::fetchUpdate()` on a dedicated FreeRTOS background worker task according to `getUpdateIntervalMs()`.
2. **Fetch**: `fetchUpdate()` requests current weather from `https://api.openweathermap.org/data/2.5/weather` using the `FetchContext`-provided TLS client and ESP-IDF certificate bundle.
3. **Selective Parsing**: The HTTP response stream is parsed in-place using **`ArduinoJson` with a `DeserializationFilter`**. Only required fields (temperature, humidity, wind, pressure, condition, icon) are decoded into the document, minimizing DRAM allocations and eliminating external streaming parser libraries.
4. **Thread-Safe Storage**:
   - Scalar numeric fields (temperature, wind speed, pressure, etc.) update `std::atomic` variables via `std::memory_order_release`.
   - String fields (condition name, icon code, city, country) are stored in fixed `char[]` buffers guarded by a FreeRTOS mutex (`_dataMutex`).
5. **Event Notification**: `fetchUpdate()` flags `_publishPending`. On the next main loop pass, `WeatherModule::tick()` publishes the `"weather.updated"` event via `EventBus`.
6. **Display Rendering**: `WeatherBoard` reads the data via `IWeatherDataProvider`'s thread-safe getters and delegates layout and drawing to `WeatherWidget` on the display carousel.
7. **Portal & MCP**: Web clients query `/api/weather/status` and AI agents call `weather_status` to inspect current metrics on demand.

---

## Configuration Workflow

1. **Get an API Key**: Obtain a free OpenWeatherMap API key at [openweathermap.org/appid](https://openweathermap.org/appid).
2. **Connect to WiFi**: Power on the device. On initial boot, connect to its SoftAP (or configure credentials via USB serial) to attach to your local network.
3. **Open the Portal**: Navigate to `http://weather-station.local` (or the device's IP address) in your web browser.
4. **Configure Weather**: Select the **Weather** tab:
   - Paste your **OpenWeatherMap API Key** (securely saved to encrypted flash via `KeyManager`).
   - Enter your **Latitude** and **Longitude**.
   - Select your preferred **Units** (`metric` or `imperial`).
5. **Save**: Click **Save**. The Startup preconditions are satisfied, the device transitions from `CONFIGURING` to `RUNNING`, and weather data begins polling automatically.

---

## Environments & Build Flags

The project configuration in `platformio.ini` provides multiple build targets and modular feature toggles.

### Target Environments

| Environment | Description | Board / Platform |
|---|---|---|
| `esp32dev` | Reference ESP32-WROOM-32 build (OLED display, buttons, MCP, HomeAssistant). | `esp32dev` |
| `esp32dev_all_on` | Extends `esp32dev` with all optional modules enabled (`WITH_OTA`, `WITH_TIME`). | `esp32dev` |
| `esp32dev_all_off` | Extends `esp32dev` with minimal features (`-UWITH_TIME`). | `esp32dev` |
| `esp32s3nano` | Waveshare ESP32-S3 Nano build (16MB flash, PSRAM, OLED, buttons, MCP, HomeAssistant). | `um_nanos3` |
| `esp32s3nano_ble` | Extends `esp32s3nano` with BLE client and service providers (`WITH_BLE`). | `um_nanos3` |
| `native` | Host-based build executing GoogleTest unit tests natively on your development machine. | `native` |

### Feature Flags (`WITH_*`)

| Flag | Purpose |
|---|---|
| `WITH_DISPLAY_U8G2` | Enables monochrome OLED carousel display subsystem using U8g2. |
| `WITH_DISPLAY_LVGL` | Enables full-colour touch display subsystem using LVGL (e.g., CrowPanel). |
| `WITH_BUTTONS` | Enables physical button input handling and `button.click` EventBus dispatch. |
| `WITH_ENCODER` | Enables rotary encoder input handling. |
| `WITH_MCP` | Enables the Model Context Protocol server over WebSockets/PsychicHttp for AI agents. |
| `WITH_HOMEASSISTANT` | Enables Home Assistant MQTT auto-discovery and sensor publishing. |
| `WITH_OTA` | Enables secure Over-The-Air firmware updates via `OTAUpdateManager`. |
| `WITH_TIME` | Enables NTP time synchronisation and POSIX timezone management via `TimeManager`. |
| `WITH_BLE` | Enables NimBLE-based Bluetooth Low Energy service and client providers. |

---

## File Inventory

```
sample-app/
├── platformio.ini                    — PlatformIO project configuration & environment definitions
├── README.md                         — Project architecture, build, and configuration documentation
├── data/
│   ├── config_manifest.json          — App configuration manifest (schema version, fields, secrets)
│   └── timezones.json                — POSIX timezone database for TimeManager NTP sync
├── src/
│   ├── main.cpp                      — Application entry point, module wiring & device identity setup
│   ├── WeatherModule.hpp             — Declaration of WeatherModule (6 framework interfaces + provider)
│   ├── WeatherModule.cpp             — WeatherModule implementation (fetch, parse, REST, MCP, preconditions)
│   ├── IWeatherDataProvider.hpp      — Pure abstract interface for thread-safe weather metric access
│   ├── WeatherBoard.hpp              — Carousel display board declaration (IDisplayBoard)
│   ├── WeatherBoard.cpp              — WeatherBoard implementation & EventBus subscription
│   ├── WeatherWidget.hpp             — Layout widget declaration composing LabelWidgets & weather icons
│   ├── WeatherWidget.cpp             — Dynamic layout measurement and rendering implementation
│   ├── ble/                          — Optional BLE sample providers (FTMS cycling, HR repeater/client)
│   ├── CMakeLists.txt                — ESP-IDF component registration for Arduino-as-component builds
│   └── idf_component.yml             — ESP-IDF component manager dependencies
├── web/
│   └── weather.js                    — Modern Web Portal JavaScript UI component (compiled to PROGMEM)
└── test/
    └── test_unit/
        └── test_weather/             — Host unit test suite (contracts, configuration, getters)
```

---

## Dependencies

- **`bblanchon/ArduinoJson@^7.4.3`**: Canonical framework JSON library used for config serialization, REST payloads, and filtered HTTP response parsing. No separate streaming JSON parser is required.
- **`hoeken/PsychicHttp@^3.0.0`**: High-performance asynchronous HTTP server with WebSocket support.
- **`olikraus/U8g2@2.36.18`**: Monochrome graphics display library powering the OLED carousel.
- **`h2zero/esp-nimble-cpp@^2.1.0`**: Lightweight BLE stack (used in BLE-enabled targets).

---

## Getting Started

### Building with PlatformIO MCP Tools

When working with agent tooling, build and flash operations must be dispatched through the `platformio` MCP server:

```
# Build project
mcp_platformio_build_project(projectDir="sample-app", environment="esp32dev")

# Upload firmware
mcp_platformio_upload_firmware(projectDir="sample-app", environment="esp32dev")
```

### CLI Building

For direct local development:

```bash
cd sample-app
pio run -e esp32dev       # Build ESP32-DevKit firmware
pio run -e esp32s3nano     # Build ESP32-S3 Nano firmware
```

### Native Testing

The sample includes native unit tests in `test/test_unit/test_weather/`. Note that `platformio.ini` configures `test_ignore = test_unit/test_weather` during host native runs because `WeatherModule.hpp` includes hardware display headers that require a minimal host U8g2 stub. Unit test execution can be enabled when targeting mock stubs:

```bash
pio test -e native
```

### Using as a Template for a New App

1. **Create your project repository:**
   ```bash
   mkdir my-esp32-app && cd my-esp32-app && git init
   ```
2. **Add the foundation as a submodule:**
   ```bash
   git submodule add https://github.com/matthewmcneill/esp32-app-foundation.git lib/esp32-app-foundation
   git submodule update --init --recursive
   ```
3. **Copy the sample app files:**
   ```bash
   cp -r lib/esp32-app-foundation/sample-app/* ./
   cp -r lib/esp32-app-foundation/sample-app/.* ./ 2>/dev/null || true
   ```
4. **Update `platformio.ini` paths** — replace `../` references with submodule paths (e.g. `lib/esp32-app-foundation/include`).
5. **Author your own `board.h`** — stock boards are reference implementations; author your own board header as the single source of truth for your device pins (see [`docs/developer/custom-boards.md`](../docs/developer/custom-boards.md)).
