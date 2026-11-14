<div align="center">

![ESP32 App Foundation](docs/img/hero-banner.jpg)

# ESP32 App Foundation

**A modular C++ firmware framework for ESP32 devices: you write the application modules, the framework provides the device.**

[![License: Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Status: 1.0 public beta](https://img.shields.io/badge/status-1.0%20public%20beta-yellow)](#project-status)
[![Latest release](https://img.shields.io/github/v/release/cblabs-org/esp32-app-foundation?include_prereleases&sort=semver)](https://github.com/cblabs-org/esp32-app-foundation/releases)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32%20%7C%20ESP32--S3-orange?logo=platformio)](https://platformio.org)
[![Framework](https://img.shields.io/badge/Arduino%20as%20ESP--IDF%20component-blue)](https://docs.espressif.com/projects/arduino-esp32/en/latest/esp-idf_component.html)

</div>

---

## What is it?

ESP32 App Foundation is a firmware framework for Wi-Fi connected ESP32 and
ESP32-S3 devices. Your application is a set of small modules that implement
pure virtual interfaces. You register them in `setup()`, and the framework
wires each one into the services it asked for: configuration storage, the web
portal, the display, Home Assistant, an on-device MCP server for AI agents, and
signed OTA updates.

It is for makers and small teams who want a device that is configurable,
updatable and observable from the first build, without writing Wi-Fi
provisioning, a config UI or an update pipeline again for every project.

What you get:

1. **A finished device shell.** Captive-portal Wi-Fi setup, mDNS, an
   authenticated web portal, encrypted credential storage, and readiness
   tracking for every external dependency your app has.
2. **Contracts instead of glue code.** Implement `IConfigurable` and your
   settings persist; implement `IWebContributor` and you get a portal tab and
   REST routes; implement `IMCPContributor` and AI agents can call your tools.
3. **A release pipeline.** Version tags become firmware versions, releases are
   RSA-signed, and devices update themselves from a signed manifest on the
   `dev`, `beta` or `stable` channel.

```cpp
#include "AppFoundation.h"
#include "WeatherModule.hpp"

static Application app;
static WeatherModule weather;

void setup() {
    FrameworkConfig config;              // OTA manifest URL, public key, ...
    app.setAppName("Weather Station");   // also sets the hostname and setup AP name
    app.registerModule(&weather);        // interfaces are detected at compile time
    app.begin(config);
}

void loop() {
    app.tick();
}
```

---

## Features

| Area | What the framework provides |
|---|---|
| **Module model** | `registerModule()` detects which interfaces a module implements at compile time and wires it up. Modules never include framework internals; `AppFoundation.h` is the only header they need. |
| **Wi-Fi & provisioning** | First boot opens a captive-portal access point. Once configured, the device retries its network indefinitely with backoff and opens a fallback AP (portal login still required) after 3 minutes offline. mDNS hostname derived from the app name. |
| **Web portal** | Single-page portal served from flash. Built-in System, Wi-Fi, Keys and OTA tabs, plus a tab for each module that contributes one. Session login, optional HTTPS with a device-generated certificate. |
| **Configuration** | Per-module JSON config in LittleFS with schema versions and migration, and a config manifest that carries settings across OTA updates. |
| **Credentials** | `KeyManager` stores API keys and passwords AES-256 encrypted with a device-bound key. Secrets are redacted from logs. |
| **Readiness** | `PreconditionEngine` tracks what the device needs before it can run (Wi-Fi, an API key, a location) and what it needs to keep running, and shows the gaps on the portal and display. |
| **Background data** | `DataManager` runs `IDataSource` fetches on a worker task with deadlines, a framework-managed TLS client, and per-source health with exponential backoff. A failing source degrades; it never reboots the device. |
| **OTA updates** | Signed manifest and signed firmware, three channels, anti-rollback counter, bootloader rollback if the new image fails its self-test, and manual signed upload from the portal. |
| **MCP server** | JSON-RPC 2.0 at `POST /api/mcp` and over serial. Built-in tools for system info, Wi-Fi, OTA, keys, config, data-source health, audit log and display state; modules add their own. Read-only by default. |
| **Displays** | Monochrome OLED carousel via U8g2 (`IDisplayBoard`), or full-colour LVGL 9 UIs designed in LVGL Pro behind a hardware adapter (`ILVGLHardwareAdapter`). |
| **Home Assistant** | MQTT auto-discovery with 11 entity types (sensor, binary sensor, switch, light, climate, cover, fan, button, number, select, device trigger), LWT and optional TLS. |
| **Bluetooth LE** | `BLEManager` (ESP32-S3) hosts peripheral GATT services and central clients side by side, with Wi-Fi coexistence handling. |
| **Input** | Debounced buttons and rotary encoders publishing gestures on the EventBus. |
| **Audit trail** | Optional flash-backed, rate-limited audit log (`AUDIT()`), readable from the portal and MCP. |
| **Testing** | GoogleTest suites run on the host through the `native` environment, plus Playwright tests for the portal. |

Optional modules are compiled in with `WITH_*` build flags, so you pay only for
what you use: `WITH_DISPLAY_U8G2`, `WITH_DISPLAY_LVGL`, `WITH_BUTTONS`,
`WITH_ENCODER`, `WITH_MCP`, `WITH_HOMEASSISTANT`, `WITH_OTA`, `WITH_TIME`,
`WITH_BLE` and `WITH_AUDIT`. See
[Using Optional Modules](docs/developer/using-optional-modules.md).

<p align="center">
  <img src="docs/img/web-portal-screenshot.jpg" alt="Web portal" width="48%">
  <img src="docs/img/display-carousel.jpg" alt="OLED display carousel" width="48%">
</p>

---

## Choose your path

### Requirements

- [PlatformIO](https://platformio.org/install) (the VS Code extension or the CLI) and Python 3.
  PlatformIO downloads the toolchain, ESP-IDF and Arduino-ESP32 on the first build.
- An ESP32 or ESP32-S3 board. The sample app targets `esp32dev` and `esp32s3nano`
  without changes; any other board needs a `board.h` (see [Custom Boards](docs/developer/custom-boards.md)).
- A USB cable that carries data.

The first build takes several minutes while PlatformIO installs the platform.

### (a) Flash the sample app

The sample app is a weather station that uses most of the framework: a
background data source, preconditions, a portal tab, an MCP tool and a display
board.

```bash
git clone https://github.com/cblabs-org/esp32-app-foundation.git
cd esp32-app-foundation/sample-app
pio run -e esp32s3nano -t upload      # or -e esp32dev
pio run -e esp32s3nano -t uploadfs    # config manifest and timezone data
pio device monitor
```

On first boot the device opens an open Wi-Fi network named after its hostname
(`weather-station`). Join it and the captive portal opens; if it doesn't,
browse to `http://192.168.4.1`. Choose your network, then reconnect to your
own Wi-Fi and open `http://weather-station.local`. The Weather tab asks for a
free [OpenWeatherMap](https://openweathermap.org/appid) API key and a
location; once both are set the device moves from *configuring* to *running*.

Walkthrough: [Sample App Walkthrough](docs/developer/sample-app-walkthrough.md) ·
[sample-app/README.md](sample-app/README.md)

### (b) Start your own app from the template

`template-app/` is a minimal module (`MyApp`) that shows five interfaces with
stub logic. Add the framework to your repository as a submodule and copy the
template in:

```bash
mkdir my-device && cd my-device && git init
git submodule add https://github.com/cblabs-org/esp32-app-foundation.git framework
cp -r framework/template-app/. .
```

The template's `platformio.ini` assumes it sits inside the framework, so point
its `../` paths at `framework/` instead. Leave the `BOARD_HEADER` lines alone:
that path is resolved from the framework's `hardware/` folder.

```bash
sed -i.bak '/BOARD_HEADER/!s#\.\./#framework/#g' platformio.ini && rm platformio.ini.bak
git add -A && git commit -m "Start from the App Foundation template"
pio run -e esp32dev
```

Make that first commit before building: the ESP-IDF build reads the git
revision and fails in a repository with no commits.

Next, copy `framework/hardware/_TEMPLATE/` to `hardware/<your-board>/` and
describe your own pins. Guides: [template-app/README.md](template-app/README.md) ·
[Creating a Module](docs/developer/creating-a-module.md) ·
[Custom Boards](docs/developer/custom-boards.md)

### (c) Let an AI coding agent set it up

The repository ships agent skills that know the framework's contracts, rules
and patterns. The `framework-setup` skill turns an empty folder into a wired
project: it adds the framework as a submodule at `framework/`, links the
`framework-expert` and `framework-portal-design` skills, and writes the agent
bootstrap files for Claude Code and Antigravity. It then hands over to
`framework-expert`, which can walk you through a design spec and scaffold your
first module.

For Claude Code, install the setup skill once:

```bash
mkdir -p ~/.claude/skills/framework-setup
curl -fsSL https://raw.githubusercontent.com/cblabs-org/esp32-app-foundation/main/.agents/skills/framework-setup/SKILL.md \
  -o ~/.claude/skills/framework-setup/SKILL.md
```

Then open an empty folder and ask: *"Set up a new ESP32 App Foundation project."*
Other agents can read the same `SKILL.md` and follow its steps.
More on how the repository is built with agents:
[AI-Native Development](docs/developer/ai-native-development.md).

---

## Supported hardware

The framework builds for any ESP32 or ESP32-S3 board through a per-board
`board.h`. These boards are the reference targets that the framework and sample
apps are built and tested on:

| Board | Chip | PSRAM | Display | PlatformIO env |
|---|---|---|---|---|
| ESP32-DevKit V1 | ESP32-WROOM-32 | none | SSD1322 256×64 OLED (SPI) | `esp32dev` |
| Waveshare ESP32-S3-Nano | ESP32-S3 | 8 MB | SSD1322 256×64 OLED (SPI) | `esp32s3nano`, `esp32s3nano_ble` |
| Elecrow CrowPanel 1.28″ HMI Rotary | ESP32-S3 | 8 MB | GC9A01 240×240 round LCD, touch, rotary encoder | `crowpanel-128-rotary` |

The ESP32-S3 boards are recommended for new work: PSRAM leaves room for TLS,
large JSON documents and LVGL. Bluetooth LE support targets the ESP32-S3.
Wiring tables and first-flash steps: [Hardware Setup](docs/developer/hardware-setup.md).

The stock board headers are reference implementations. Your application should
own its `board.h`, the single place that defines every pin it uses.

---

## Security

The security model assumes a device on a local network, protecting against
other devices on that network, tampered updates and credential theft from
flash. In summary:

| Layer | Implementation |
|---|---|
| Credentials at rest | AES-256-CBC via `DeviceCrypto`, master key generated on the device and kept in NVS; `KeyManager` stores secrets encrypted |
| Portal and MCP access | Salted SHA-256 password hash, `HttpOnly` session cookie, bearer token for MCP; login is skipped only during first-boot setup |
| Portal transport | Optional HTTPS on port 443 with a device-generated EC P-256 certificate, HTTP redirected |
| Outbound TLS | Data sources get a `FrameworkSecureClient` validated against the ESP-IDF root CA bundle |
| OTA | Manifest and firmware each carry an RSA-2048 / SHA-256 signature, verified while streaming; certificate-validated TLS; image version checked before flashing |
| Anti-rollback | Monotonic `SECURITY_VERSION` in NVS; older security versions are rejected |

Details and trade-offs: [Security](docs/developer/security.md) and
[OTA Update System](docs/developer/ota-update-system.md). To report a
vulnerability, see [SECURITY.md](SECURITY.md).

---

## Documentation

| Start here | |
|---|---|
| [Developer Guide](docs/developer/README.md) | Index of the developer documentation, with a reading order |
| [Architecture Overview](docs/developer/architecture-overview.md) | Boot lifecycle, the 3-tier model, how registration works |
| [Creating a Module](docs/developer/creating-a-module.md) | Writing a module against the interfaces |
| [Coding Patterns](docs/developer/coding-patterns.md) | Service modules, ownership, threading rules |
| [Extending the Web Portal](docs/developer/extending-the-web-portal.md) | Tabs, REST routes, front-end conventions |
| [Scheduling Data Fetches](docs/developer/scheduling-data-fetches.md) | `IDataSource`, `FetchContext`, health and backoff |
| [MCP Server](docs/developer/mcp-server.md) | Tool registration and the JSON-RPC protocol |
| [Bluetooth LE](docs/developer/bluetooth.md) | `BLEManager`, providers, memory budget, coexistence |
| [OTA Update System](docs/developer/ota-update-system.md) | Keys, signing, manifest, channels, deploy |
| [Versioning Policy](docs/developer/versioning-policy.md) | Tags, channels and how releases are cut |
| [Testing](docs/developer/testing.md) | Native unit tests and portal tests |
| [Design Specification](docs/design_specification.md) | The architectural specification and its invariants |

For AI agents, `.agents/skills/framework-expert/` holds the same knowledge in a
form organised for agents.

---

## Project status

**1.0 public beta.** The interfaces and module model are the ones intended for
1.0, and the framework runs on the reference boards above. During the beta,
releases are tagged `v1.0.0-beta.N` and breaking changes are still possible;
each one is listed in [CHANGELOG.md](CHANGELOG.md) with a migration note.
`v1.0.0` will follow once the framework has had outside use and feedback.

Feedback is the point of the beta. Please
[open an issue](https://github.com/cblabs-org/esp32-app-foundation/issues/new/choose)
for bugs, rough edges in the docs, boards you have tried, or features you
need.

---

## Contributing

Contributions are welcome: bug reports, fixes, documentation, board support and
new modules. Start with [CONTRIBUTING.md](CONTRIBUTING.md), which covers the
build and test setup and what a pull request needs. Everyone taking part is
expected to follow the [Code of Conduct](CODE_OF_CONDUCT.md).

---

## License

© 2026 CB Labs. Licensed under the [Apache License 2.0](LICENSE).
