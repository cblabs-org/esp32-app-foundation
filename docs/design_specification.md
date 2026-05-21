# ESP32 App Foundation — Design Specification

**Status:** Draft  
**Version:** 0.1  
**Author:** CB Labs  
**Last Updated:** 2026-05-19

---

## Table of Contents

1. [Purpose & Vision](#1-purpose--vision)
2. [Design Principles](#2-design-principles)
3. [System Architecture Overview](#3-system-architecture-overview)
4. [Framework Kernel](#4-framework-kernel)
5. [Module System](#5-module-system)
6. [Extension Points & Interfaces](#6-extension-points--interfaces)
7. [Display Architecture](#7-display-architecture)
8. [Configuration Architecture](#8-configuration-architecture)
9. [Application Lifecycle](#9-application-lifecycle)
10. [Web Portal Architecture](#10-web-portal-architecture)
11. [Inter-Module Communication](#11-inter-module-communication)
12. [Repository & Build Architecture](#12-repository--build-architecture)
13. [Security Architecture](#13-security-architecture)
14. [Resource Budget & Constraints](#14-resource-budget--constraints)
15. [Module Authoring Guide](#15-module-authoring-guide)
16. [Open Questions & Decision Log](#16-open-questions--decision-log)

---

## 1. Purpose & Vision

### 1.1 Problem Statement

Embedded ESP32 development repeatedly requires the same foundational infrastructure:
WiFi provisioning, configuration persistence, OTA firmware updates, a device web
portal, logging, and system lifecycle management. Without a shared foundation, each
new project either starts from scratch or inherits a messy copy-paste of a previous
project's internals — leading to divergent, hard-to-maintain codebases.

### 1.2 What This Framework Is

The **ESP32 App Foundation** is a **C++ application framework** for ESP32 devices
built with PlatformIO (Arduino + ESP-IDF dual framework). It provides:

- A structured, deterministic application lifecycle (`begin()` / `tick()`)
- Pre-wired core services (WiFi, Config, OTA, Web Portal, Crypto, Logging)
- A registration bus so optional modules self-attach without kernel changes
- Defined extension points where app-specific logic plugs in
- A base web portal that apps extend with custom tabs

### 1.3 What This Framework Is Not

- **Not ESPHome**: This is a code-first C++ framework, not a YAML/config-driven
  device compiler. It targets genuinely custom application logic — complex state
  machines, display rendering pipelines, data transformation — not sensor reporting.
- **Not a monolith**: The framework does not mandate a display, MQTT, or any domain
  module. Apps compose the modules they need.
- **Not a HAL or RTOS**: The framework operates above the Arduino/ESP-IDF layer.
  FreeRTOS task management and peripheral drivers are app concerns.

### 1.4 Primary Use Case

Building custom embedded applications on ESP32 that require a standardised
boot sequence, network stack, web configuration portal, and OTA updates — but
with bespoke application logic that would be impossible to express in a declarative
config-driven system.

**Reference application**: The `departures-board` project (rail/tube/bus departure
display) demonstrates the class of application this framework targets.

### 1.5 Goals

- A new app can be created by cloning a template repo and writing only application-
  specific code
- Adding or removing optional modules requires only changes to `platformio.ini` and
  `appSetup.cpp` — no kernel changes
- Framework updates (bug fixes, new features) flow to all derived apps via `pio pkg update`
- The framework compiles cleanly on both `esp32dev` and `esp32s3nano` environments

---

## 2. Design Principles

### 2.1 The Hollywood Principle — "Don't Call Us, We'll Call You"

The framework owns `setup()` and `loop()`. Application code never writes these
directly. The framework calls application code at defined extension points.
This is what makes it an application *framework* rather than a library collection.

### 2.2 Composition Over Inheritance

Application modules register with the framework via interfaces. There is no
`BaseApp` class that apps inherit from. The `Application` hub owns registered
services and dispatches lifecycle events to them. This avoids single-inheritance
rigidity and allows modules to be independently composed.

### 2.3 No Cross-Module Direct Dependencies

Optional modules never `#include` other optional modules. All inter-module
communication routes through the framework kernel via shared services
(`MessagePool`, `IStatusReporter`) or framework-mediated callbacks. This
guarantees any combination of modules compiles correctly.

### 2.4 Kernel Knows Nothing About Modules

The framework kernel does not `#include` any optional module. Modules self-register
by calling `Application.register*()` methods. This is enforced at the include-graph
level — a CI check should verify no kernel header imports a module header.

### 2.5 Boot-Phase Allocation — Not Static, Not Unconstrained

The embedded instinct to "prefer static over dynamic" targets *runtime* heap
fragmentation — the real enemy is repeated alloc/free cycles during `tick()`
steady-state. Allocation that happens **once at boot and never again** is
functionally indistinguishable from static allocation in terms of heap health.

The framework uses a **two-tier vector model**:

- **Structural arrays** (`ITickable*`, `IDisplayBoard*`, `iDataSource*`) — built
  with `std::vector` during `begin()` or a controlled soft-reset sequence, then
  `shrink_to_fit()` is called to release over-allocated capacity. These arrays
  are **never mutated during `tick()` steady-state**. Any structural change
  (e.g. adding a display board) requires a soft-reset.

- **Lightweight arrays** (`KeyManager` entries, web routes, config consumers) —
  `std::vector` that may grow at any time since additions do not affect loop
  scheduling or carousel timing. No `shrink_to_fit()` required.

There are **no `MAX_*` compile-time ceilings**. Over-allocating static arrays
to fit a worst-case that never occurs wastes RAM on every device. Under-allocating
causes hard limits that constrain future development. The vector model sizes
exactly to what the app uses, measured not guessed.

Heap usage is logged after each build phase and published on the `'system.heap'`
EventBus topic so monitoring modules can react programmatically.

### 2.6 Sensible Defaults, Everything Overridable

The framework bootstraps with working defaults for every service (hostname, brightness,
timezone, OTA settings). Every default can be overridden via config or compile-time
`#define`. Apps should work out-of-the-box on first flash.

### 2.7 Convention Over Configuration

Standard directory layout, standard module naming, and standard interface contracts
mean the framework "just works" when conventions are followed. Developers only
configure what differs from the default.

#### LittleFS Directory Convention

LittleFS supports directories (unlike the deprecated SPIFFS). The framework uses
a Linux-inspired layout so that file ownership is immediately self-evident:

```
/                          ← LittleFS root
├── etc/                   ← Framework kernel — system config
│   ├── config.json        ← FrameworkConfig (hostname, timezone, OTA)
│   └── apikeys.json       ← KeyManager encrypted credential store
├── mod/                   ← Module config files (one per module)
│   ├── display.json       ← Display module settings
│   ├── mqtt.json          ← MQTT module settings
│   ├── ha.json            ← Home Assistant module settings
│   └── scheduler.json     ← Scheduler module settings
├── app/                   ← Application owned files
│   └── <appname>.json     ← App-specific config
└── www/                   ← Web portal LittleFS-served assets
    └── <appname>/         ← App tab fragments (served by ESPAsyncWebServer)
        ├── station.html   ← App tab content fragment
        └── station.js     ← App tab logic
```

**Ownership rules:**
- `etc/` — written only by the framework kernel (`configManager`, `keyManager`). Never touched by modules or apps.
- `mod/` — each module owns exactly one file, named after the module. No module reads another module's file.
- `app/` — owned by application code. The framework never writes here.
- `www/` — app-specific web assets served at runtime. Framework SPA shell lives in PROGMEM, not here.

**Why short names?** LittleFS stores directory entries inline; shorter path components reduce inode overhead on the 4MB flash partition.

---

## 3. System Architecture Overview

### 3.1 Three-Tier Model

```
┌─────────────────────────────────────────────────────────┐
│                   APPLICATION TIER                      │
│  App-specific modules (boards, data clients, UI)        │
│  App config schema & web portal tabs                    │
│  platformio.ini (board pins, environment flags)         │
└───────────────────────┬─────────────────────────────────┘
                        │  registers via extension points
┌───────────────────────▼─────────────────────────────────┐
│                 OPTIONAL MODULE CATALOG                  │
│  U8G2 Display Module  │  MQTT Module                    │
│  TFT Display Module   │  HomeAssistant Module           │
│  Weather Client       │  RSS Client                     │
│  MCP Server Module    │  Scheduler Module               │
│  boardLED             │  buttonHandler                  │
│                       │  (future modules...)            │
└───────────────────────┬─────────────────────────────────┘
                        │  uses framework interfaces
┌───────────────────────▼─────────────────────────────────┐
│                  FRAMEWORK KERNEL                        │
│  Application (DI hub) │  configManager                 │
│  keyManager           │  dataManager + iDataSource     │
│  wifiManager          │  otaUpdateManager              │
│  webServer (base)     │  timeManager                   │
│  deviceCrypto         │  EventBus                      │
│  logger (ESP-IDF wrap)│                                │
└─────────────────────────────────────────────────────────┘
│            PlatformIO / Arduino + ESP-IDF               │
└─────────────────────────────────────────────────────────┘
```

### 3.2 `Application` as the System Hub

`Application` is the single composition root and the central name for the framework's
DI hub. It:
- Owns all kernel services by value (deterministic lifetime, avoids heap)
- Exposes kernel services via accessor methods (DI injection points)
- Maintains registration lists for tickables, config consumers, and data sources
- Owns and drives the `AppState` state machine
- Dispatches `begin()` and `tick()` to all registered components in the correct order

Application code receives an `Application*` pointer at setup and uses it to access
services — it never constructs kernel services directly.

### 3.3 Entry Point Contract

The framework owns `setup()` and `loop()`. The template project's `main.cpp`
contains only:

```cpp
#include <AppFoundation.h>
#include "myApp.hpp"

MyApp app;

void setup() { app.begin(); }
void loop()  { app.tick();  }
```

`MyApp` instantiates its modules and registers them with the `Application` hub
inside `begin()`.

---

## 4. Framework Kernel

The kernel is always present. No app can opt out of kernel services.

### 4.1 Kernel Components

| Component | Purpose | Source |
|---|---|---|
| `Application` | DI hub, orchestrator, AppState owner | `modules/application/` |
| `configManager` | LittleFS JSON config persistence | `modules/configManager/` |
| `keyManager` | Encrypted API key/credential registry (uses DeviceCrypto) | `modules/keyManager/` |
| `dataManager` | Background HTTP fetch worker (FreeRTOS task) | `modules/dataManager/` |
| `wifiManager` | WiFi connection + captive portal AP mode | `modules/wifiManager/` |
| `webServer` | ESPAsyncWebServer, base portal routes | `modules/webServer/` |
| `otaUpdateManager` | GitHub-based OTA firmware updates | `modules/otaUpdateManager/` |
| `timeManager` | NTP sync, timezone management | `modules/timeManager/` |
| `EventBus` | Generic typed publish/subscribe event bus | `modules/eventBus/` |
| `deviceCrypto` | Hardware-backed key derivation & encryption | `lib/deviceCrypto/` |
| `logger` | Thin wrapper around ESP-IDF logging (adds secret redaction) | `lib/logger/` |
| `githubClient` | HTTPS GitHub API client (used by OTA) | `lib/githubClient/` |
| `md5Utils` | MD5 hashing utility | `lib/md5Utils/` |

### 4.2 Kernel Configuration

The kernel's config schema (`FrameworkConfig`) covers only cross-cutting concerns:

```
hostname          — Network mDNS hostname
timezone          — POSIX timezone string
brightness        — Default display brightness (if display present)
firmwareUpdatesEnabled  — OTA update toggle
otaQuietHour      — Hour of quiet OTA check (default 3am)
carouselIntervalSecs    — Default display rotation interval
manualOverrideTimeoutSecs — Button override duration
```

All domain-specific and module-specific fields live in separate config files
managed by their respective modules.

---

## 5. Module System

### 5.1 What Is a Module

A module is an optional unit of functionality that:
1. Implements one or more framework interfaces (`ITickable`, `iConfigurable`,
   `iDataSource`, `IStatusReporter`)
2. Self-registers with `appContext` during app setup
3. Manages its own configuration file on LittleFS
4. Registers its own web portal routes (if it has a UI)
5. **Never directly `#include`s another optional module**

### 5.2 Module Catalog (Planned)

| Module | Interfaces | Config File | Notes |
|---|---|---|---|
| `u8g2-display` | `ITickable`, `IStatusReporter`, `iConfigurable` | `/display.json` | Monochrome OLED/LCD |
| `tft-display` | `ITickable`, `IStatusReporter`, `iConfigurable` | `/display.json` | Colour TFT/touch |
| `mqtt-client` | `ITickable`, `iConfigurable` | `/mqtt.json` | |
| `homeassistant` | `ITickable`, `iConfigurable` | `/ha.json` | |
| `weather-client` | `iDataSource`, `iConfigurable` | `/weather.json` | |
| `rss-client` | `iDataSource`, `iConfigurable` | `/rss.json` | |
| `mcp-server` | `ITickable` | `/mcp.json` | |
| `scheduler` | `ITickable`, `iConfigurable` | `/scheduler.json` | Generic time-based callback scheduler |
| `boardLED` | `ITickable` | — | Optional: device has a status LED |
| `buttonHandler` | `ITickable` | — | Optional: device has a physical button |

> **On `scheduler`**: The kernel does not include a scheduler. An optional `scheduler`
> module provides a generic time-based callback mechanism (`schedule.at(hour, min, fn)`,
> `schedule.every(ms, fn)`). App-specific logic such as "show board X between 08:00
> and 18:00" is implemented by the app using this module — the module itself has no
> concept of display boards or carousel routing.

### 5.3 Adding a Module to an App

In `platformio.ini`:
```ini
lib_deps =
    https://github.com/matthewmcneill/esp32-app-foundation.git#main
    https://github.com/matthewmcneill/esp32-modules.git#main
```

In `appSetup.cpp`:
```cpp
#include <u8g2DisplayModule.hpp>
#include <mqttModule.hpp>
#include <boardLED.hpp>

static U8G2DisplayModule display;
static MqttModule mqtt;
static BoardLED statusLed;

void MyApp::begin() {
    app.registerTickable(&display);
    app.registerTickable(&mqtt);
    app.registerTickable(&statusLed);
    app.registerConfigConsumer(&display);
    app.registerConfigConsumer(&mqtt);
}
```

---

## 6. Extension Points & Interfaces

All interfaces live in the framework kernel and are the only cross-tier dependency.

### 6.1 `ITickable`

```cpp
class ITickable {
public:
    virtual void tick(uint32_t currentMillis) = 0;
    virtual ~ITickable() = default;
};
```

Registered via `app.registerTickable(ITickable*)`. Called every `loop()`.
Modules use this for polling, state transitions, and heartbeat operations.

### 6.2 `iConfigurable`

```cpp
class iConfigurable {
public:
    virtual void reapplyConfig(const FrameworkConfig& config) = 0;
    virtual ~iConfigurable() = default;
};
```

Registered via `app.registerConfigConsumer(iConfigurable*)`. Called on
boot after config load and after any web portal config update.

### 6.3 `iDataSource`

```cpp
class iDataSource {
public:
    virtual UpdateStatus updateData() = 0;
    virtual void executeFetch() = 0;
    virtual uint32_t getNextFetchTime() = 0;
    virtual PriorityTier getPriorityTier() = 0;
    // ... (full interface in dataManager/iDataSource.hpp)
};
```

Registered via `app.registerDataSource(iDataSource*)`. The `dataManager`
background task calls `executeFetch()` on a FreeRTOS worker thread — network
operations must ONLY occur inside this method.

### 6.4 `IStatusReporter`

The framework-level abstraction for cross-module status display. Any module can
report status without depending on a concrete display implementation.

```cpp
class IStatusReporter {
public:
    virtual void reportMessage(const char* source, const char* message) = 0;
    virtual void reportProgress(const char* label, int percent) = 0;
    virtual void reportError(const char* source, const char* error) = 0;
    virtual ~IStatusReporter() = default;
};
```

Registered via `app.setStatusReporter(IStatusReporter*)`. If no display
module is present, a `NullStatusReporter` handles calls silently.

### 6.5 `IDisplayBoard` (Display Module Interface)

See Section 7. This interface is defined by the display module, not the kernel.
App display boards implement it and register with the display module directly.

### 6.6 `IWebContributor`

```cpp
class IWebContributor {
public:
    virtual void registerRoutes(WebServerManager& server) = 0;
    virtual const char* getTabName() const { return nullptr; } // nullptr = no nav tab
    virtual const char* getTabEndpoint() const { return nullptr; }
    virtual ~IWebContributor() = default;
};
```

Registered via `app.registerWebContributor(IWebContributor*)`. The base
web portal calls `registerRoutes()` on all contributors during `webServer.init()`.

---

## 7. Display Architecture

### 7.1 Design Decision: Module Variants, Not a Unified Driver Interface

A unified `IDisplayDriver` abstraction that both U8G2 (monochrome bitmap) and
TFT colour displays implement is architecturally problematic:

- U8G2 uses a page-buffer model; TFT uses a full framebuffer / sprite model
- Their drawing APIs are fundamentally different in colour model and coordinate system
- A meaningful common interface would either be too thin to be useful or too thick
  to avoid re-implementing both libraries

**Decision**: There is no `IDisplayDriver`. Instead, the module catalog provides
**separate display modules**:

| Module | Technology | Use Case |
|---|---|---|
| `u8g2-display-module` | U8G2 library | Monochrome OLED/LCD (SSD1322, SH1106, etc.) |
| `tft-display-module` | LovyanGFX | Colour TFT with touch (ILI9341, ST7789, etc.) |

Each module provides its own board interface and carousel management. App display
boards are written natively for their target display technology.

### 7.2 Display Module Responsibilities

A display module:
1. Initialises the physical hardware driver
2. Implements `IStatusReporter` — enables cross-module status messages
3. Manages the board carousel (slot pool, rotation, screensaver)
4. Provides system boards (boot splash, loading, WiFi wizard, OTA progress, error screens)
5. Exposes `registerBoard(IDisplayBoard*)` for app-specific views
6. Registers itself with `appContext` as `ITickable` and `iConfigurable`

### 7.3 `IDisplayBoard` (U8G2 Variant)

```cpp
class IDisplayBoard {
public:
    virtual const char* getBoardName() const = 0;
    virtual void onActivate() = 0;
    virtual void onDeactivate() = 0;
    virtual void tick(uint32_t ms) = 0;
    virtual void render(U8G2& display) = 0;  // U8G2-native
    virtual UpdateStatus updateData() = 0;
    virtual bool isScrollFinished() { return true; }
    virtual ~IDisplayBoard() = default;
};
```

The TFT display module defines an equivalent `ITftDisplayBoard` with
`render(LovyanGFX& display)`.

### 7.4 Board Registration

```cpp
// In app setup — registering a custom board with the U8G2 display module
u8g2Display.registerBoard(&myDeparturesBoard);
```

The display module's static slot pool allocates the slot; the app provides the
board instance (app-owned, static lifetime).

### 7.5 System Boards

Both display modules provide a standard set of system boards that the framework
drives directly during boot, WiFi setup, OTA, and error states:

- Boot Splash
- Loading / Progress
- WiFi Setup Wizard
- Station/App Setup Guide
- OTA Firmware Update
- Error screens (WiFi lost, data error, auth error)
- Screensaver / Sleep Clock

System board content is customisable per app (app name, logo, branding).

---

## 8. Configuration Architecture

### 8.1 Separation of Concerns

Configuration is split across multiple JSON files on LittleFS, organised under
the directory convention defined in §2.7:

| Path | Owner | Contents |
|---|---|---|
| `/etc/config.json` | Framework kernel | FrameworkConfig (hostname, timezone, OTA) |
| `/etc/apikeys.json` | Framework kernel | KeyManager encrypted credential store |
| `/mod/display.json` | Display module | Brightness, flip, sleep timeout |
| `/mod/mqtt.json` | MQTT module | Broker host, port, credentials |
| `/mod/ha.json` | HA module | HA API endpoint, token reference |
| `/mod/scheduler.json` | Scheduler module | Schedule rules |
| `/app/<appname>.json` | Application | App-specific settings |

**Ownership rules:**
- Framework kernel reads/writes only `etc/`
- Each module reads/writes only its own file under `mod/`
- Application code reads/writes only `app/`
- No component reads or writes outside its designated directory

### 8.2 Config Versioning

Each config file includes a `configVersion` field. The owning module validates
this on load and performs migration or writes a fresh default if the version is
incompatible.

### 8.3 App Config Extension Pattern

The application defines its own struct and manages serialisation:

```cpp
struct MyAppConfig {
    char stationId[13];
    char apiKeyId[13];
    int refreshInterval;
};

class MyAppConfigManager {
    MyAppConfig config;
public:
    void load();   // reads /app/myapp.json from LittleFS
    bool save();   // writes /app/myapp.json to LittleFS
    MyAppConfig& getConfig() { return config; }
};
```

The app's web portal endpoints read/write this config independently.

### 8.4 Config Consumer Notification

When the web portal saves any config, it calls:
```cpp
appContext.getConfigManager().requestReload();
```

On the next `tick()`, the framework safely executes the reload on the main thread
and calls `reapplyConfig()` on all registered `iConfigurable` consumers.

---

## 9. Application Lifecycle

### 9.1 Boot State Machine

```
BOOTING
  │  (crypto init, display init, splash)
  │  (filesystem mount, config load)
  │  (WiFi connect attempt)
  ├──→ WIFI_SETUP   (WiFi failed, AP mode active — show WiFi wizard)
  │       └──→ BOOTING (on successful connect)
  │
  │  (NTP sync, web server init, first data fetch)
  ├──→ APP_SETUP    (no app config present — show setup guide)
  │       └──→ RUNNING (on config save via portal)
  │
  └──→ RUNNING      (normal operation)
         │
         └──→ WIFI_SETUP (on WiFi loss during runtime)
```

### 9.2 `begin()` Sequence (Framework-Managed)

The framework executes boot in this deterministic order:

1. DeviceCrypto init (must precede any credential reads)
2. Display module init → Boot Splash (if display module registered)
3. LittleFS mount
4. FrameworkConfig load → KeyManager load
5. App config load (app-provided hook)
6. WiFi init (non-blocking, transitions to WIFI_SETUP if AP mode needed)
7. DataManager task start
8. Register all module config consumers
9. OTA manager init
10. → Yield to `tick()` loop

> Modules such as `boardLED` and `buttonHandler` self-initialise during their own
> `registerTickable()` call. The framework does not assume their presence.

Progress is reported to `IStatusReporter` at each step (visible on display if present).

### 9.3 `tick()` Sequence (Every Loop)

1. Check deferred config reload flag → reload + notify if pending
2. WiFiManager tick (connection state machine)
3. OTA manager tick (update check, download state)
4. AppState evaluation → drive state transitions
5. Registered `ITickable` dispatch (all modules, including boardLED, buttonHandler,
   display module, MQTT, etc. — whatever the app registered)
6. WiFi connection health monitor

---

## 10. Web Portal Architecture

### 10.1 Base Portal

The framework provides a base web portal as a LittleFS-hosted Single Page Application.
The SPA shell (HTML/CSS/JS) is framework-owned and serves:

- **WiFi tab** — SSID scan, connect, hostname, password change
- **System tab** — Device info, firmware version, build time, heap stats
- **OTA tab** — Firmware update trigger, release notes, OTA settings

### 10.2 App Tab Extension

Modules register additional tabs via `IWebContributor`:

```cpp
class MyModule : public IWebContributor {
    const char* getTabName() const override { return "Station Config"; }
    const char* getTabEndpoint() const override { return "/api/station"; }
    void registerRoutes(WebServerManager& server) override {
        server.on("/api/station", HTTP_GET, handleGetStation);
        server.on("/api/station", HTTP_POST, handlePostStation);
    }
};
```

The portal shell fetches `/api/tabs` on load and dynamically builds the navigation.
Each tab's content is fetched from its registered endpoint.

### 10.3 Static Asset Organisation

The portal uses a **hybrid asset model** (see ADR D8):
- **Framework SPA shell** — compiled into PROGMEM as gzip-compressed `const uint8_t[]`
  arrays inside the kernel library. Served at ~0ms. Always present with the firmware.
  No LittleFS dependency for the base portal.
- **App-specific tab fragments** — stored in LittleFS under `www/` and served at
  request time by ESPAsyncWebServer. Updated via `pio run --target uploadfs`.

```
data/                      ← LittleFS image root (portalBuilder.py output)
└── www/                   ← Web portal app-specific assets only
    └── <appname>/         ← One subdirectory per app
        ├── station.html   ← App tab content fragment
        └── station.js     ← App tab logic
```

Framework assets that ship in PROGMEM (NOT in LittleFS):
```
[kernel library PROGMEM]
  index_html_gz[]   ← SPA shell (HTML)
  app_css_gz[]      ← Base styles
  app_js_gz[]       ← Core SPA logic, tab routing
  wifi_js_gz[]      ← WiFi tab logic
  system_js_gz[]    ← System tab logic
  ota_js_gz[]       ← OTA tab logic
```

### 10.4 API Conventions

All API endpoints follow REST conventions:

- `GET /api/<resource>` — read current state as JSON
- `POST /api/<resource>` — update state, body is JSON, response is `{ok: true}` or error
- `GET /api/tabs` — returns nav tab registry `[{name, endpoint}]`

---

## 11. Inter-Module Communication

### 11.1 The Core Rule

**Modules never call each other directly.** All inter-module communication
uses one of three framework-mediated mechanisms:

### 11.2 EventBus (General Publish/Subscribe)

The `EventBus` is the primary mechanism for decoupled cross-module communication.
Modules publish typed events by topic; any number of subscribers receive them
without the publisher knowing who (if anyone) is listening.

```cpp
// Publisher (e.g., MQTT module)
app.getEventBus().publish("mqtt.connected", "{\"broker\":\"192.168.1.10\"}");

// Subscriber (e.g., display module, logger, HA module)
app.getEventBus().subscribe("mqtt.connected", [](const char* payload) {
    // react to the event
});
```

- Topics are strings (e.g. `"mqtt.connected"`, `"sensor.temperature"`)
- Payloads are small JSON strings or plain strings
- Subscribers are registered at setup; the bus uses a static fixed-size table
- If no subscribers exist for a topic, the event is silently dropped

> **Note:** The departures-board's `MessagePool` (a scrolling-text ring buffer for
> station messages) was a domain-specific implementation, not a general bus. Display
> modules may subscribe to a `"display.message"` topic on the EventBus to achieve
> a similar scrolling-text effect in a generalised way.

### 11.3 IStatusReporter (Structured UI Status)

For progress bars, error overlays, and structured status during long operations
(boot, OTA, data fetch), any component can report status without knowing if a
display is present:

```cpp
app.getStatusReporter().reportProgress("Connecting...", 45);
app.getStatusReporter().reportError("MQTT", "Auth failed");
```

The display module's `IStatusReporter` implementation maps these to appropriate
system boards or overlay widgets. If no display is present, the `NullStatusReporter`
no-ops silently.

### 11.4 Callbacks / std::function (Point-to-Point)

For tight coupling between two known kernel components (e.g., OTA manager → display),
the framework uses typed callbacks registered at setup. This pattern is **only**
used for kernel→kernel-service coupling, never for cross-module communication:

```cpp
otaManager.setProgressCallback([&display](int pct) {
    display.showOtaProgress(pct);
});
```

---

## 12. Repository & Build Architecture

### 12.1 Repository Topology

```
esp32-app-foundation/    ← Framework kernel (this repo)
  lib/                   ← Low-level utilities
  modules/               ← Core framework modules
  docs/                  ← This document
  scripts/               ← Build scripts (portalBuilder, build_fonts, etc.)
  data/                  ← Base portal LittleFS assets
  library.json           ← PlatformIO library manifest

esp32-modules/           ← Optional module catalog (separate repo)
  u8g2-display/
  tft-display/
  mqtt-client/
  homeassistant/
  weather-client/
  rss-client/
  mcp-server/

esp32-app-template/      ← GitHub Template repo (new project starting point)
  src/main.cpp           ← Skeleton entry point
  src/myApp.cpp          ← App setup skeleton
  platformio.ini         ← Template with lib_deps
  scripts/               ← Build scripts (symlinked or copied from framework)
  partitions.*.csv       ← Standard partition tables
  sdkconfig.defaults*    ← Standard SDK config

<new-app-repo>/          ← Per-project app repo
  src/                   ← App-specific logic
  modules/               ← App-specific modules (if any)
  data/<appname>/        ← App-specific web assets
  platformio.ini         ← Selects modules via lib_deps
```

### 12.2 `platformio.ini` Pattern for an App

```ini
[env:esp32dev]
platform = ...
framework = arduino, espidf
lib_deps =
    https://github.com/matthewmcneill/esp32-app-foundation.git#v1.0.0
    https://github.com/matthewmcneill/esp32-modules.git#v1.0.0
    olikraus/U8g2@2.36.18
    bblanchon/ArduinoJson@7.4.3
    ESP32Async/ESPAsyncWebServer
extra_scripts =
    pre:scripts/build_timestamp.py
    pre:scripts/portalBuilder.py
```

### 12.3 Local Development Workflow

When actively developing the framework alongside an app, use a local path override:

```ini
lib_deps =
    file:///Users/dev/esp32-app-foundation   ; local path for active development
    file:///Users/dev/esp32-modules
```

Push changes to the framework repo; bump the `#tag` in app `lib_deps` when ready
to adopt a new framework version.

### 12.4 Build Scripts

Build scripts ship in the framework repo under `scripts/`. The template repo
references them. Standard scripts:

| Script | Purpose |
|---|---|
| `build_timestamp.py` | Embeds build time as `BUILD_TIME` macro |
| `portalBuilder.py` | Packages web assets into LittleFS image |
| `build_fonts.py` | Compiles custom U8G2 fonts |
| `auto_build_wasm.py` | Builds any WASM components |
| `generate_layouts.py` | Generates display layout constants |

---

## 13. Security Architecture

### 13.1 DeviceCrypto

The framework includes a hardware-backed cryptographic engine (`DeviceCrypto`)
that derives a unique device key from the ESP32's hardware MAC/eFuse and uses it
for symmetric encryption of sensitive configuration values (API tokens, passwords).

- Keys are **never stored in plaintext** on LittleFS
- Encrypted values are encrypted with AES-256 using the device-unique key
- The device key is non-exportable — encrypted config is device-specific

### 13.2 KeyManager & API Key Registry

Key and credential management is a distinct concern from configuration persistence
and is handled by a dedicated `KeyManager` kernel component:

- `KeyManager` depends on `DeviceCrypto` (initialised first) for encryption/decryption
- API keys and tokens are stored in `/apikeys.json` in encrypted form
- `KeyManager` exposes `addKey()`, `deleteKey()`, `getKeyById()` — ConfigManager
  has no knowledge of key content
- Keys are referenced by opaque IDs (e.g., `k-12345678`) in module configs — the
  ID is not sensitive; only the resolved key value is
- Modules retrieve keys at runtime via `app.getKeyManager().getKeyById(id)`

### 13.3 OTA Security

GitHub-based OTA:
- Downloads firmware only over HTTPS with certificate validation
- Verifies firmware binary MD5 before applying
- Uses ESP-IDF's partition rollback: if the new firmware fails to call
  `esp_ota_mark_app_valid_cancel_rollback()` within the boot timeout, the
  bootloader automatically reverts to the previous firmware

### 13.4 Web Portal Security

- The portal is only accessible on the local network (no cloud dependency)
- No authentication is implemented by default (local network trust model)
- Future: optional HTTP Basic Auth configurable via `FrameworkConfig`

---

## 14. Resource Budget & Constraints

### 14.1 Target Hardware

| Parameter | esp32dev | esp32s3nano |
|---|---|---|
| Flash | 4MB (default) / 16MB (S3) | 16MB |
| DRAM | 320KB (usable ~200KB) | 512KB |
| IRAM | 128KB | 128KB |
| PSRAM | Not assumed | Board-dependent |

### 14.2 Framework Kernel Budget

The kernel (without any optional modules) should target:

| Resource | Budget | Notes |
|---|---|---|
| Flash (binary) | < 600KB | Leaves room for app + LittleFS partition |
| Static RAM | < 40KB | Globals, BSS, static arrays |
| Heap at boot | > 120KB free | Required for WiFi stack + async web server |

### 14.3 Key Memory Rules

- `WiFiClientSecure`, `HTTPClient` must NEVER be allocated on the stack in HTTPS
  operations — always heap via `std::unique_ptr<WiFiClientSecure>(new (std::nothrow) WiFiClientSecure())`
- Registration arrays are `std::vector` — built once at boot/soft-reset,
  `shrink_to_fit()` called on structural arrays after each build phase
- **Structural arrays are NEVER mutated during `tick()` steady-state** — any
  structural change triggers a soft-reset cycle
- Lightweight arrays (keys, web routes, config consumers) may grow at any time
  since they do not affect loop scheduling
- Module instances MUST be declared `static` at file scope — never stack-allocated.
  Stack-allocated module destructors fire on scope exit, leaving dangling pointers
  in the `Application` hub's tick array.

### 14.4 Registration Array Model

There are **no `MAX_*` compile-time ceilings**. The framework sizes arrays exactly
to what the application registers — no waste, no artificial constraints.

**Structural arrays** (rebuilt at boot/soft-reset, `shrink_to_fit()` after build):

| Array | Type | Mutation Policy |
|---|---|---|
| `ITickable*` | `std::vector` | Soft-reset only |
| `IDisplayBoard*` | `std::vector` | Soft-reset only |
| `iDataSource*` | `std::vector` | Soft-reset only |

**Lightweight arrays** (may grow at any time):

| Array | Type | Mutation Policy |
|---|---|---|
| `iConfigurable*` | `std::vector` | Append any time |
| `IWebContributor*` | `std::vector` | Append any time |
| `KeyManager` entries | `std::vector` | Append any time |

**After each build phase**, the framework logs heap stats and publishes:
```cpp
app.getEventBus().publish("system.heap",
    "{\"free\":" + String(ESP.getFreeHeap()) +
    ",\"minFree\":" + String(ESP.getMinFreeHeap()) + "}");
```
The web portal's System tab consumes this directly.

### 14.5 Boot-Phase Allocation Pattern (Reference Implementation)

This section documents how the two-tier vector model is implemented inside the
`Application` hub. It is the normative reference for any contributor implementing
or extending the hub.

#### The Three Lifecycle Phases

```
╔══════════════════════════════════════════════════════════════════╗
║  PHASE 1: REGISTRATION                                           ║
║  begin() open — all register*() calls accepted                  ║
║  std::vector grows via push_back() (amortised doubling)         ║
╠══════════════════════════════════════════════════════════════════╣
║  PHASE 2: COMPACTION                                             ║
║  Structural arrays: shrink_to_fit() releases over-capacity      ║
║  Heap stats logged + 'system.heap' EventBus event fired         ║
║  _locked = true  ← structural arrays are now immutable          ║
╠══════════════════════════════════════════════════════════════════╣
║  PHASE 3: STEADY-STATE  (tick() loop)                           ║
║  Structural: reads only — zero allocation, zero deallocation    ║
║  Lightweight: append freely — no lock, no shrink                ║
║  Any call to register structural array → assert / panic         ║
╚══════════════════════════════════════════════════════════════════╝
```

#### `Application` Hub Sketch

```cpp
class Application {
public:
    // ── Structural registration (Phase 1 only) ──────────────────
    void registerTickable(ITickable* t) {
        ASSERT_MESSAGE(!_locked, "registerTickable() after begin() — use softReset()");
        _tickables.push_back(t);
    }
    void registerDataSource(iDataSource* ds) {
        ASSERT_MESSAGE(!_locked, "registerDataSource() after begin() — use softReset()");
        _dataSources.push_back(ds);
    }

    // ── Lightweight registration (any phase) ────────────────────
    void registerConfigConsumer(iConfigurable* c)  { _configConsumers.push_back(c); }
    void registerWebContributor(IWebContributor* w) { _webContributors.push_back(w); }

    // ── Lifecycle ───────────────────────────────────────────────
    void begin() {
        // ... boot sequence (DeviceCrypto, LittleFS, WiFi, etc.) ...
        _compactAndLock();   // Phase 2: shrink + lock + heap report
    }

    void tick() {
        // Phase 3: structural arrays are read-only pointer iteration — no alloc
        for (ITickable* t : _tickables) {
            t->tick(millis());
            yield();  // feed TWDT between dispatches
        }
    }

    // ── Soft-reset (for structural changes mid-session) ─────────
    void softReset() {
        // 1. Drain current tick cycle (caller ensures we're at a safe boundary)
        // 2. Unlock structural arrays
        _locked = false;
        _tickables.clear();
        _dataSources.clear();
        // 3. App re-registers its modules (same as begin() setup block)
        _onRegister();       // app-provided hook: calls all register*() again
        // 4. Re-compact and re-lock
        _compactAndLock();
    }

private:
    // ── Structural arrays ────────────────────────────────────────
    std::vector<ITickable*>   _tickables;
    std::vector<iDataSource*> _dataSources;
    bool                      _locked = false;

    // ── Lightweight arrays ───────────────────────────────────────
    std::vector<iConfigurable*>   _configConsumers;
    std::vector<IWebContributor*> _webContributors;

    // ── Compaction + heap report ─────────────────────────────────
    void _compactAndLock() {
        _tickables.shrink_to_fit();
        _dataSources.shrink_to_fit();
        _locked = true;
        _reportHeap();
    }

    void _reportHeap() {
        uint32_t free    = ESP.getFreeHeap();
        uint32_t minFree = ESP.getMinFreeHeap();
        ESP_LOGI("Application", "Heap after build: free=%u minFree=%u", free, minFree);
        char payload[64];
        snprintf(payload, sizeof(payload),
                 "{\"free\":%u,\"minFree\":%u}", free, minFree);
        getEventBus().publish("system.heap", payload);
    }
};
```

#### The Soft-Reset Cycle in Detail

A soft-reset is triggered when a structural change is needed at runtime (e.g.
the user adds a new display board via the web portal, or a module is dynamically
enabled by a config change):

```
1. Web portal receives POST /api/module/add
2. Handler sets a deferred flag: _softResetPending = true
3. Next tick() boundary: framework detects flag before dispatching tickables
4. framework calls softReset():
     a. _locked = false
     b. _tickables.clear() / _dataSources.clear()
     c. _onRegister() — app re-registers all modules (including new one)
     d. _compactAndLock() — shrink_to_fit + heap report + _locked = true
5. tick() resumes with the updated arrays
```

The deferred flag pattern mirrors the existing config reload flag — structural
changes are never applied mid-dispatch.

#### Why This Is Safer Than Fixed `MAX_*` Arrays on ESP32

| | Fixed `MAX_*` | Two-Tier Vector |
|---|---|---|
| RAM cost (4 modules registered) | `MAX_TICKABLES × 4 bytes` regardless | `4 × 4 bytes` exactly |
| RAM cost (worst case) | Same as above | Slightly higher (vector metadata: 12 bytes) |
| Overflow behaviour | Silent memory corruption | Assertion → watchdog rollback |
| Adding an 9th module when MAX=8 | Cannot — recompile required | Works — vector grows |
| Runtime fragmentation risk | None (static) | None (allocated once, `shrink_to_fit`) |
| Heap observability | None | `'system.heap'` EventBus after each build |

---

## 15. Module Authoring Guide

### 15.1 Creating a New Module

A minimal module that participates in the framework tick loop:

**`myModule.hpp`**
```cpp
#pragma once
#include <AppFoundation.h>  // Framework interfaces

class MyModule : public ITickable, public iConfigurable, public IWebContributor {
public:
    void begin(appContext* ctx);
    void tick(uint32_t ms) override;
    void reapplyConfig(const FrameworkConfig& cfg) override;
    void registerRoutes(WebServerManager& server) override;
    const char* getTabName() const override { return "My Module"; }
    const char* getTabEndpoint() const override { return "/api/mymodule"; }
private:
    appContext* _ctx = nullptr;
    // module state...
};
```

**Registration in app setup:**
```cpp
static MyModule myModule;

void MyApp::begin() {
    myModule.begin(&appCtx);
    appCtx.registerTickable(&myModule);
    appCtx.registerConfigConsumer(&myModule);
    appCtx.registerWebContributor(&myModule);
}
```

### 15.2 Module Config File Pattern

All modules store config under `/mod/<modulename>.json`:

```cpp
// Convention: /mod/<modulename>.json
constexpr char CONFIG_PATH[] = "/mod/mymodule.json";

void MyModule::loadConfig() {
    File f = LittleFS.open(CONFIG_PATH, "r");
    if (!f) { writeDefaults(); return; }
    StaticJsonDocument<512> doc;   // bounded — never DynamicJsonDocument in handlers
    deserializeJson(doc, f);
    strlcpy(_config.host, doc["host"] | "localhost", sizeof(_config.host));
    _config.port = doc["port"] | 1883;
    f.close();
}

void MyModule::writeDefaults() {
    File f = LittleFS.open(CONFIG_PATH, "w");
    if (!f) return;
    f.print("{\"configVersion\":1,\"host\":\"localhost\",\"port\":1883}");
    f.close();
}
```

### 15.3 Module Checklist

- [ ] Implements required interfaces (at minimum `ITickable`)
- [ ] Uses `app.getStatusReporter()` for user-visible status
- [ ] Publishes to `"display.message"` EventBus topic for scrolling text (not MessagePool)
- [ ] Config file is stored at `/mod/<modulename>.json` — nowhere else
- [ ] Config file includes a `configVersion` field; module validates it on load
- [ ] Config is parsed with `StaticJsonDocument<N>` — never `DynamicJsonDocument`
- [ ] Module instance declared `static` — never stack-allocated
- [ ] Never `#include`s another optional module header
- [ ] `tick()` completes in ≤ 10ms — long I/O is deferred to a FreeRTOS task
- [ ] Registers web routes via `IWebContributor` if it has a UI
- [ ] Web assets (if any) stored under `www/<appname>/` in LittleFS
- [ ] Implements `reapplyConfig()` to respond to config changes
- [ ] Has no compile-time dependency on display hardware

---

## 16. Open Questions & Decision Log

This section tracks decisions made and questions still to resolve.

### Resolved Decisions

| # | Decision | Rationale |
|---|---|---|
| D1 | No unified `IDisplayDriver` interface | U8G2 and TFT API models are incompatible; display module variants are the correct embedded pattern |
| D2 | Per-module LittleFS config files, not a monolithic struct | Enables true module independence; avoids schema coupling |
| D3 | Two-tier `std::vector` model, no `MAX_*` ceilings | Boot-phase allocation is functionally static (no runtime fragmentation). Avoids over-allocating for a worst-case that never occurs, and removes artificial ceilings that constrain development. Structural arrays use `shrink_to_fit()` after each build phase. |
| D4 | Framework owns `setup()` and `loop()` | Defines this as an application framework (Hollywood Principle) |
| D5 | lib_deps for kernel + modules, template repo for scripts | Separates versioned library code from project scaffolding |
| D6 | Structural array mutations require soft-reset; lightweight arrays may grow freely | Keeps `tick()` hot path completely free of vector mutations. Structural changes (adding a display board) are rare; the soft-reset cost is acceptable. Lightweight changes (adding an API key) are safe to append at any time. |
| D7 | Heap stats logged + published on `'system.heap'` EventBus after each build phase | Replaces artificial MAX_* ceilings with observable runtime evidence. Portal System tab surfaces live heap; monitoring modules can subscribe to the event. |
| D8 | Linux-inspired LittleFS directory layout: `etc/`, `mod/`, `app/`, `www/` | LittleFS supports directories; flat root is an anti-pattern inherited from SPIFFS. Directory-per-tier makes ownership self-evident, enables selective wipe (e.g. `rm /mod/*` to reset all module configs), and prevents naming collisions between modules. |
| D9 | Hybrid web portal asset model: PROGMEM shell + LittleFS `www/` app fragments | Framework shell in PROGMEM: serves at ~0ms, always present with firmware, no LittleFS dependency in AP/captive mode. App fragments in LittleFS: updated independently via `uploadfs` without reflashing firmware. |

### Open Questions

| # | Question | Impact |
|---|---|---|
| Q1 | Should `schedulerManager` be kernel or optional module? | Medium — most apps benefit from it; few apps are truly headless |
| Q2 | What is the `webServer` base portal's auth model? | Security — no auth vs. optional HTTP Basic Auth |
| Q3 | Should display modules provide a `PSRAM framebuffer` option for S3 boards? | Performance — large colour displays need framebuffer in PSRAM |
| Q4 | How should multi-board hardware configs (e.g., dual display) be handled? | Scope — possibly out of scope for v1 |
| Q5 | What is the versioning / release cadence for the framework? | DX — semver tags vs. rolling `#main` |
| Q6 | Should the template repo use git submodules or lib_deps for the scripts? | Build — scripts need to be editable in local dev |

---

*This document is a living specification. Update the Decision Log as design choices
are finalised during implementation.*
