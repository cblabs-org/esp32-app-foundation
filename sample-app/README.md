# Weather Station — ESP32 App Foundation Sample App

A complete, real-world weather station built on the ESP32 App Foundation
framework. This sample demonstrates how to use six framework interfaces to
build a production-quality embedded application.

## What This Sample Demonstrates

| Interface         | Purpose in Sample                                  |
|-------------------|----------------------------------------------------|
| `ITickable`       | Non-blocking tick loop (delegates work to DataManager) |
| `IConfigurable`   | Persistent config with namespace, versioning, and round-trip serialization |
| `IWebContributor` | Custom REST API + embedded JavaScript portal tab   |
| `IDataSource`     | Background HTTP fetch from OpenWeatherMap API       |
| `IMCPContributor` | MCP tool registration for AI/device interaction     |
| `IConfigGate`     | Boot-time API key validation gate                   |

### Key Patterns Illustrated

- **ADR-39 Service Locator**: `WeatherModule::begin(Application&)` extracts narrow
  service refs and never stores the Application reference.
- **EventBus Static Callback**: `WeatherClockBoard` subscribes to `weather.updated`
  using a static function pointer + `void* ctx` — the framework-required pattern
  (no `std::function`).
- **Thread Safety**: `std::atomic` for numeric fields, FreeRTOS mutex for
  variable-length string fields, demonstrating cross-task data exchange.
- **Streaming JSON**: `JsonStreamingParser` for low-memory HTTP response parsing
  (no full-document allocation).
- **IDisplayBoard**: Custom carousel board composing a `WeatherWidget` for
  U8G2 OLED rendering.

## Project Structure

```
sample-app/
├── src/
│   ├── main.cpp              — Entry point: file-scope instances, setup/loop
│   ├── WeatherModule.hpp/cpp — 6-interface module (the core sample)
│   ├── WeatherClockBoard.hpp/cpp — IDisplayBoard + EventBus subscription
│   └── WeatherWidget.hpp/cpp — Static U8G2 rendering utility
├── test/
│   └── test_unit/test_weather/ — GoogleTest unit tests
├── data/                     — LittleFS filesystem (portal assets)
├── platformio.ini            — Build config (esp32dev, esp32s3nano, native)
└── README.md                 — This file
```

## Getting Started

### Using as a standalone sample (within the framework repo)

```bash
cd sample-app
pio run -e esp32dev       # Build for ESP32-DevKit
pio test -e native        # Run unit tests
```

### Using as a template for a new project

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

4. **Update `platformio.ini` paths** — replace `../` references with submodule paths:
   ```ini
   build_flags =
       -I lib/esp32-app-foundation/include
   lib_extra_dirs =
       lib/esp32-app-foundation/modules
       lib/esp32-app-foundation/lib
   ```
   Also update `board_build.partitions` and `custom_sdkconfig` to point to
   `lib/esp32-app-foundation/partitions...` instead of `../`.

## Building

```bash
pio run                    # Build all environments
pio run -e esp32dev        # Build for ESP32-DevKit
pio run -e esp32s3nano     # Build for ESP32-S3 Nano
pio run -e native          # Build native test binary
```

## Testing

```bash
pio test -e native         # Run all native unit tests
```

Tests cover:
- **IConfigurable**: Config round-trip, defaults, partial application
- **IConfigGate**: Fail-open without KeyManager, gate metadata
- **IDataSource**: Source name, priority tier, update interval
- **Thread-safe getters**: Initial state verification
- **IWebContributor**: Tab metadata

## Configuration

The weather module requires:
1. **OpenWeatherMap API key** — obtain free at [openweathermap.org/appid](https://openweathermap.org/appid)
2. **Latitude/Longitude** — your location coordinates
3. **Units** — `metric` (°C, m/s) or `imperial` (°F, mph)

Configure via the web portal's **Weather** tab or the MCP `weather_status` tool.
