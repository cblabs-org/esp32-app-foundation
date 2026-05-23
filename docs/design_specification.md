# ESP32 App Foundation — Design Specification

**Status:** v1.1 (Post-Implementation)  
**Version:** 1.1  
**Author:** CB Labs  
**Last Updated:** 2026-05-22

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
12. [Logging & Observability Architecture](#11a-logging--observability-architecture)
13. [Repository & Build Architecture](#12-repository--build-architecture)
14. [Security Architecture](#13-security-architecture)
15. [Resource Budget & Constraints](#14-resource-budget--constraints)
16. [Module Authoring Guide](#15-module-authoring-guide)
17. [Testing Architecture](#16-testing-architecture)
18. [Open Questions & Decision Log](#17-open-questions--decision-log)

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
- Pre-wired core services (WiFi, Config, OTA, Web Portal, Crypto, Logging, MCP)
- A registration bus so optional modules self-attach without kernel changes
- Defined extension points where app-specific logic plugs in
- A base web portal that apps extend with custom tabs
- A composable widget toolkit for U8G2 monochrome displays

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

- A new app can be created by cloning the `template/` skeleton and writing only
  application-specific code
- Adding or removing optional modules requires only changes to `platformio.ini` and
  app setup code — no kernel changes
- Framework updates flow to all derived apps via git submodule SHA pinning
- The framework compiles cleanly on `esp32dev`, `esp32s3nano`, and `native`
  (host CPU for GoogleTest) environments

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
(`EventBus`, `IStatusReporter`) or framework-mediated callbacks. This
guarantees any combination of modules compiles correctly.

### 2.4 Kernel Knows Nothing About Modules

The framework kernel does not `#include` any optional module. Modules self-register
by calling `Application.register*()` methods. This is enforced at the include-graph
level — the CI workflow verifies no kernel header imports a module header.

### 2.5 Boot-Phase Allocation — Not Static, Not Unconstrained

The embedded instinct to "prefer static over dynamic" targets *runtime* heap
fragmentation — the real enemy is repeated alloc/free cycles during `tick()`
steady-state. Allocation that happens **once at boot and never again** is
functionally indistinguishable from static allocation in terms of heap health.

The framework uses a **two-tier vector model**:

- **Structural arrays** (`ITickable*`, `IDisplayBoard*`, `iDataSource*`,
  `IMcpContributor*`) — built with `std::vector` during `begin()` or a controlled
  soft-reset sequence, then `shrink_to_fit()` is called to release over-allocated
  capacity. These arrays are **never mutated during `tick()` steady-state**. Any
  structural change (e.g. adding a display board) requires a soft-reset.

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

### 2.8 Native Build Portability

All framework code must compile on both ESP32 hardware targets and host CPU
(x86_64/arm64) for native GoogleTest execution. This is a non-negotiable
cross-cutting concern that affects every module.

- Hardware-specific APIs (`esp_restart()`, `LittleFS`, `WiFi.h`, free heap queries)
  are wrapped in `#ifndef NATIVE_BUILD` preprocessor guards
- Interface headers use forward declarations for external library types
  (e.g., `class U8G2;`, `class AsyncWebServer;`) — never `#include` heavy
  hardware-dependent headers
- Widget and board `render()` methods are no-ops on native; logic-only paths
  (tick, state machines, scheduling) are fully testable
- The `millis()` function is avoided in headers; `currentMillis` parameter
  injection is the preferred pattern for testability

> **Critical LDF constraint:** PlatformIO's Library Dependency Finder (LDF)
> scans `#include` directives as **text patterns** regardless of `#ifdef`
> preprocessor guards. A conditional `#include "Application.hpp"` inside
> `#ifndef NATIVE_BUILD` will still trigger LDF to resolve and compile the
> entire dependency chain. Always forward-declare in headers.

### 2.9 No Monolithic Services

Each kernel concern lives in a distinct, self-contained service module. Bundling
multiple concerns into a single "systemManager" is explicitly prohibited.

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
│  Scheduler            │  boardLED                       │
│  buttonHandler        │  (future modules...)            │
└───────────────────────┬─────────────────────────────────┘
                        │  uses framework interfaces
┌───────────────────────▼─────────────────────────────────┐
│                  FRAMEWORK KERNEL                        │
│  Application (DI hub) │  configManager                  │
│  keyManager           │  dataManager + iDataSource      │
│  wifiManager          │  otaUpdateManager               │
│  webServer            │  timeManager                    │
│  deviceCrypto         │  EventBus                       │
│  mcpServer            │  LogRouter (vprintf pipeline)   │
│  githubClient         │  md5Utils                       │
└─────────────────────────────────────────────────────────┘
│            PlatformIO / Arduino + ESP-IDF               │
└─────────────────────────────────────────────────────────┘
```

All tiers live in a **single monorepo** (`esp32-app-foundation`). There is no
separate `esp32-modules` repo — optional modules live alongside kernel modules
under `modules/`. Apps consume the framework via git submodule.

### 3.2 `Application` as the System Hub

`Application` is the single composition root and the central name for the framework's
DI hub. It:
- Owns all kernel services as member variables (deterministic lifetime, avoids heap):
  - `EventBus _eventBus`
  - `DataManager _dataManager`
  - `WebServerManager _webServerManager`
  - `McpServer _mcpServer`
  - `NullStatusReporter _nullReporter` (private nested class)
- Exposes kernel services via reference getter methods (DI injection points)
- Maintains registration lists for tickables, config consumers, data sources,
  web contributors, and MCP contributors
- Owns and drives the `AppState` state machine
- Dispatches `begin()` and `tick()` to all registered components in the correct order
- Provides `softReset()` for structural changes at runtime

`Application` is instantiated as a **file-scope static** in `src/main.cpp` — never
stack-allocated, never a singleton via `getInstance()`. Application code receives
an `Application*` pointer at setup and uses it to access services — it never
constructs kernel services directly.

### 3.3 Entry Point Contract

The framework owns `setup()` and `loop()`. The `src/main.cpp` contains only:

```cpp
#include <AppFoundation.h>
#include "myApp.hpp"

static MyApp app;

void setup() { app.begin(); }
void loop()  { app.tick();  }
```

`AppFoundation.h` is the **canonical single-include façade** — it includes all
framework interface headers. Downstream consumers should only include this file.

`MyApp` instantiates its modules and registers them with the `Application` hub
inside `begin()`.

---

## 4. Framework Kernel

The kernel is always present. No app can opt out of kernel services.

### 4.1 Kernel Components

| Component | Purpose | Source |
|---|---|---|
| `Application` | DI hub, orchestrator, AppState owner | `modules/application/` |
| `configManager` | LittleFS JSON config persistence with schema versioning | `modules/configManager/` |
| `keyManager` | Encrypted API key/credential registry (uses DeviceCrypto) | `modules/keyManager/` |
| `dataManager` | Background data-source scheduler (FreeRTOS task) | `modules/dataManager/` |
| `wifiManager` | WiFi connection + captive portal AP mode | `modules/wifiManager/` |
| `webServer` | ESPAsyncWebServer, base portal routes, `IWebContributor` dispatch | `modules/webServer/` |
| `otaUpdateManager` | GitHub-based OTA firmware updates with rollback | `modules/otaUpdateManager/` |
| `timeManager` | Non-blocking NTP sync, POSIX timezone management | `modules/timeManager/` |
| `EventBus` | Synchronous publish/subscribe with static fn-ptr callbacks | `modules/eventBus/` |
| `mcpServer` | JSON-RPC MCP server (Serial + HTTP transport) | `modules/mcpServer/` |
| `deviceCrypto` | Hardware-backed key derivation & AES-256 encryption | `lib/deviceCrypto/` |
| `logger` | Secret-redaction utility (`Logger::logSecure()`) wired into the LogRouter `vprintf` pipeline | `lib/logger/` |
| `LogShim.h` | Unified logging shim — bridges ESP-IDF `esp_log.h` to native `printf` stubs | `include/platform/` |
| `LogRouter` | Custom `esp_log_set_vprintf()` handler: redaction → serial → ILogSink dispatch | `modules/application/` |
| `githubClient` | HTTPS GitHub API client (used by OTA) | `lib/githubClient/` |
| `md5Utils` | Streaming MD5 hashing via MbedTLS (incremental, not full-file) | `lib/md5Utils/` |

### 4.2 Kernel Configuration

The kernel's config schema (`FrameworkConfig`) covers only cross-cutting concerns:

```
hostname                — Network mDNS hostname
timezone                — POSIX timezone string
brightness              — Default display brightness (if display present)
firmwareUpdatesEnabled  — OTA update toggle
otaQuietHour            — Hour of quiet OTA check (default 3am)
carouselIntervalSecs    — Default display rotation interval
manualOverrideTimeoutSecs — Button override duration
mcpReadOnly             — MCP read-only mode toggle (default false)
```

`FrameworkConfig` is passed to `Application::begin(config)`. All domain-specific
and module-specific fields live in separate config files managed by their
respective modules.

> **Required downstream build flag:** Apps must define `APP_FOUNDATION_VERSION`
> in their `platformio.ini` `build_flags` (e.g., `-DAPP_FOUNDATION_VERSION=\"0.1.0\"`).
> The Application hub uses this for the boot banner.

---

## 5. Module System

### 5.1 What Is a Module

A module is an optional unit of functionality that:
1. Implements one or more framework interfaces (`ITickable`, `iConfigurable`,
   `iDataSource`, `IStatusReporter`, `IWebContributor`, `IMcpContributor`)
2. Self-registers with `Application` during app setup
3. Manages its own configuration file on LittleFS under `/mod/`
4. Registers its own web portal routes (if it has a UI)
5. **Never directly `#include`s another optional module**

### 5.2 Module Catalog

All modules live under `modules/` in the monorepo alongside kernel modules.

| Module | Interfaces | Config File | Notes |
|---|---|---|---|
| `u8g2Display` | `ITickable`, `IStatusReporter`, `iConfigurable` | `/mod/display.json` | Monochrome OLED/LCD + widget toolkit |
| `tft-display` | `ITickable`, `IStatusReporter`, `iConfigurable` | `/mod/display.json` | Colour TFT/touch (future) |
| `mqtt-client` | `ITickable`, `iConfigurable` | `/mod/mqtt.json` | (future) |
| `homeassistant` | `ITickable`, `iConfigurable` | `/mod/ha.json` | (future) |
| `weather-client` | `iDataSource`, `iConfigurable` | `/mod/weather.json` | (future) |
| `rss-client` | `iDataSource`, `iConfigurable` | `/mod/rss.json` | (future) |
| `scheduler` | `ITickable`, `iConfigurable` | `/mod/scheduler.json` | Generic time-based callback scheduler |
| `boardLED` | `ITickable` | — | Optional: device has a status LED |
| `buttonHandler` | `ITickable` | — | Optional: device has a physical button |

> **On `scheduler`**: The kernel does not include a scheduler. An optional `scheduler`
> module provides a generic time-based callback mechanism (`schedule.at(hour, min, fn)`,
> `schedule.every(ms, fn)`). App-specific logic such as "show board X between 08:00
> and 18:00" is implemented by the app using this module — the module itself has no
> concept of display boards or carousel routing.

### 5.3 Adding a Module to an App

Apps consume the framework via **git submodule**:

```bash
# Add the framework as a submodule (pinned to a commit SHA)
git submodule add https://github.com/matthewmcneill/esp32-app-foundation.git framework
git submodule update --init
```

In `platformio.ini`:
```ini
[env:esp32dev]
platform = espressif32
framework = arduino, espidf
lib_extra_dirs =
    framework/modules
    framework/lib
lib_deps =
    framework/                  ; local submodule path
    olikraus/U8g2@2.36.18
    bblanchon/ArduinoJson@7.4.3
    ESP32Async/ESPAsyncWebServer
extra_scripts =
    pre:framework/scripts/build_timestamp.py
```

> **Warning:** `build_src_filter` does NOT work with ESP-IDF (it uses `CMakeLists.txt`
> for component discovery, not PlatformIO's source filter). Use `lib_extra_dirs` +
> explicit `lib_deps` instead.

In app setup:
```cpp
#include <AppFoundation.h>
#include <u8g2DisplayModule.hpp>
#include <boardLED.hpp>

static U8G2DisplayModule display;
static BoardLED statusLed;

void MyApp::begin() {
    app.registerTickable(&display);
    app.registerTickable(&statusLed);
    app.registerConfigConsumer(&display);
}
```

---

## 6. Extension Points & Interfaces

All interfaces live in the framework kernel under `include/interfaces/` and are the
only cross-tier dependency.

### 6.1 Interface Naming Convention

The framework uses a mixed naming convention inherited from the reference application:

- **PascalCase `I` prefix**: `ITickable`, `IStatusReporter`, `IWebContributor`,
  `IDisplayBoard`, `IMcpContributor`
- **camelCase `i` prefix**: `iConfigurable`, `iDataSource`

All interfaces are **pure abstract classes** with only pure virtual methods and
`virtual ~IFoo() = default;`. Interface headers MUST forward-declare external
library types (e.g., `class U8G2;`, `class AsyncWebServer;`) — never `#include`
them. This is required for native build portability.

### 6.2 `ITickable`

```cpp
class ITickable {
public:
    virtual void tick(uint32_t currentMillis) = 0;
    virtual ~ITickable() = default;
};
```

Registered via `app.registerTickable(ITickable*)`. Called every `loop()`.
Modules use this for polling, state transitions, and heartbeat operations.

### 6.3 `iConfigurable`

```cpp
class iConfigurable {
public:
    virtual void reapplyConfig(const FrameworkConfig& config) = 0;
    virtual ~iConfigurable() = default;
};
```

Registered via `app.registerConfigConsumer(iConfigurable*)`. Called on
boot after config load and after any web portal config update.

### 6.4 `iDataSource`

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

> **Note:** `dataManager` is a generic data-source scheduler, not HTTP-specific.
> HTTP fetching is the responsibility of individual `iDataSource` implementations.
> `PriorityTier` uses numerical mapping: `HIGH = 0`, `LOW = 2` — lower value
> means higher scheduling precedence.

### 6.5 `IStatusReporter`

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
module is present, a `NullStatusReporter` (private nested class within
`Application`) handles calls silently.

### 6.6 `IDisplayBoard` (Display Module Interface)

See Section 7. This interface is defined by the display module, not the kernel.
App display boards implement it and register with the display module directly.

### 6.7 `IWebContributor`

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
`WebServerManager` collects non-null `getTabName()` results to build the
`GET /api/tabs` response.

### 6.8 `IMcpContributor`

```cpp
class IMcpContributor {
public:
    virtual void registerMcpTools(McpToolRegistry& registry) = 0;
    virtual ~IMcpContributor() = default;
};
```

Registered via `app.registerMcpContributor(IMcpContributor*)`. Mirrors the
`IWebContributor` pattern — Hollywood Principle applied to MCP tool registration.
The `McpServer` calls `registerMcpTools()` on all contributors during init.

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
| `u8g2Display` | U8G2 library | Monochrome OLED/LCD (SSD1322, SH1106, SSD1306, etc.) |
| `tft-display` | LovyanGFX | Colour TFT with touch (ILI9341, ST7789, etc.) — future |

Each module provides its own board interface and carousel management. App display
boards are written natively for their target display technology.

### 7.2 Display Module Responsibilities

A display module:
1. Initialises the physical hardware driver
2. Implements `IStatusReporter` — enables cross-module status messages
3. Manages the board carousel (slot pool via `std::vector<IDisplayBoard*>`, locked
   post-`begin()` — no `MAX_BOARDS` ceiling)
4. Provides system boards (boot splash, loading, WiFi wizard, OTA progress, error screens)
5. Exposes `registerBoard(IDisplayBoard*)` for app-specific views
6. Registers itself with `Application` as `ITickable` and `iConfigurable`

**Override Board Mechanism**: System boards bypass the carousel entirely via an
`_overrideBoard` pointer. When `showBoard(IDisplayBoard*)` receives a non-pool
board, the module stores it in `_overrideBoard` and renders it instead of the
current carousel board. This ensures system boards (boot splash, error, OTA
progress) are shown immediately without disrupting carousel state.

**Display Controller Selection**: The `U8G2_DISPLAY_TYPE` macro defaults to
`U8G2_SSD1306_128X64_NONAME_F_HW_I2C`, overridable via `build_flags` in
`platformio.ini`. The module owns a `U8G2` instance by value (not pointer).

**System boards** are stored as member variables of the module (not heap-allocated),
ensuring predictable lifetime and zero heap fragmentation.

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
    virtual bool isExpired() { return false; }
    virtual ~IDisplayBoard() = default;
};
```

#### Board Expiry Semantics

The `isExpired()` method controls when a board auto-dismisses. The framework uses
an **asymmetric expiry model** — different board types have different lifecycles:

- **OTA Progress**: `isExpired()` returns `true` when complete AND successful.
  Failed OTA persists to show the error.
- **Error Board**: `isExpired()` returns `true` only when explicitly `dismiss()`ed.
  The `fatal` flag controls UI text ("System halted") but NOT expiry.
- **Screensaver**: `isExpired()` returns `true` when `!_active`. Starts expired
  by default; controlled via `activate()`/`deactivate()` lifecycle.
- **Transient boards** (boot splash, loading): Auto-dismiss after their purpose
  is fulfilled.

### 7.4 Board Registration

```cpp
// In app setup — registering a custom board with the U8G2 display module
u8g2Display.registerBoard(&myDeparturesBoard);
```

The board slot pool (`std::vector<IDisplayBoard*>`) allocates the slot during
`begin()`; the pool is locked after `_compactAndLock()`. The app provides the
board instance (app-owned, static lifetime).

### 7.5 System Boards

The display module provides a standard set of system boards that the framework
drives directly during boot, WiFi setup, OTA, and error states:

- Boot Splash
- Loading / Progress
- WiFi Setup Wizard
- OTA Firmware Update Progress
- Error screens (WiFi lost, data error, auth error)
- Screensaver / Sleep Clock

All system boards internally compose with widgets from the widget toolkit (§7.7).
System board content is customisable per app (app name, logo, branding).

### 7.6 Sleep / Wake Integration

The display module manages power saving via sleep/wake:

- **`_sleep()` sequence**: Set `_sleeping=true` → activate screensaver → show
  screensaver board → call `setPowerSave(1)` (display off)
- **`_wake()` sequence**: Set `_sleeping=false` → deactivate screensaver →
  `setPowerSave(0)` + restore contrast → carousel resumes from current position

Activity tracking: `_lastActivityMs` updated on user interaction; sleep triggered
after configurable timeout.

### 7.7 Widget Toolkit Architecture

The widget toolkit provides composable UI building blocks for U8G2 displays.
All widget files live within `modules/u8g2Display/widgets/`, fonts in
`modules/u8g2Display/fonts/`, and board layout interfaces in
`modules/u8g2Display/boards/interfaces/`.

#### Three-Tier Widget Architecture

| Tier | Components | Dependencies |
|---|---|---|
| **Primitives** | `iGfxWidget`, `drawingPrimitives`, `U8g2StateSaver`, `iBoardLayout` | U8G2 only |
| **Generic Widgets** | `labelWidget`, `scrollingTextWidget`, `clockWidget`, `progressBarWidget`, `imageWidget` | Tier 1 |
| **Module-Aware Widgets** | `wifiStatusWidget`, `systemMessageWidget`, `scrollingMessagePoolWidget` | Tier 1 + framework services |

#### `iGfxWidget` (Abstract Base)

```cpp
class iGfxWidget {
public:
    int16_t x, y, width, height;  // concrete members, not virtual getters
    bool visible = true;

    virtual void tick(uint32_t currentMillis) = 0;
    virtual void render(U8G2& display) = 0;
    virtual void renderAnimationUpdate(U8G2& display) {}
    virtual ~iGfxWidget() = default;

    void setCoords(int16_t x, int16_t y);
};
```

Key design decisions:
- **Concrete x/y/w/h members** — zero vtable overhead for position queries
- **`currentMillis` parameter injection** — no `Arduino.h` in widget headers
- **Three-phase lifecycle**: `tick()` for logic, `render()` for full redraw,
  `renderAnimationUpdate()` for partial animation frames

#### `drawingPrimitives` (Stateless Utility Functions)

Free functions for consistent text rendering with alignment and truncation:
- `drawText(U8G2&, x, y, text, align, maxWidth)` with `TextAlign` enum
  (`LEFT`, `CENTER`, `RIGHT`)
- `blankArea(U8G2&, x, y, w, h)`
- `drawTriangle(U8G2&, ...)`

`TextAlign` is declared outside `#ifndef NATIVE_BUILD` (universally available).
All other drawing functions are guarded.

#### `U8g2StateSaver` (RAII Scope Guard)

Non-copyable scope guard that saves/restores U8G2 font, clip window, and draw
colour. Constructor saves state, destructor restores. Holds `U8G2&` reference —
zero heap allocation.

#### `iBoardLayout` (Board View Interface)

Decouples rendering from board controller logic. Receives `Application*` for
framework service access. Forward-declares both `class U8G2;` and
`class Application;` in the header.

#### `AppFonts` Namespace

Inline functions returning U8G2 built-in Helvetica fonts on hardware, `nullptr`
on native. `#include <U8g2lib.h>` placed inside the namespace block (never at
header top level).

#### Display Dimension Queries

All widgets use `display.getDisplayWidth()` and `display.getDisplayHeight()` at
runtime — **no compile-time `SCREEN_WIDTH`/`SCREEN_HEIGHT` constants**. When
`width == -1`, widgets fall back to full display width. In `tick()` (where no
display reference is available), a conservative 256px fallback is used.

#### `scrollingMessagePoolWidget`

Consumes EventBus topic `"display.message"` via static callback + `void* ctx`.
Maintains a fixed circular buffer (`MessageEntry[8]`). Uses late binding via
`begin(EventBus*)` — no EventBus dependency at construction time.

---

## 8. Configuration Architecture

### 8.1 Separation of Concerns

Configuration is split across multiple JSON files on LittleFS, organised under
the directory convention defined in §2.7:

| Path | Owner | Contents |
|---|---|---|
| `/etc/config.json` | Framework kernel | FrameworkConfig (hostname, timezone, OTA, mcpReadOnly) |
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

### 8.2 Config Versioning & Schema Migration

Each config file includes a `configVersion` integer field. On load, the owning
module:
1. Reads the stored version
2. Performs incremental migration if the stored version is older than the current schema
3. Falls back to safe defaults if migration is impossible or the file is corrupt
4. Writes the updated config with the current version

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

### 8.4 Config Consumer Notification & Hot Reload

When the web portal saves any config, it calls:
```cpp
appContext.getConfigManager().requestReload();
```

On the next `tick()`, the framework safely executes the reload on the main thread
and calls `reapplyConfig()` on all registered `iConfigurable` consumers. This is
an EventBus-dispatched reload — **no device reboot required**.

### 8.5 JSON Memory Safety

**`DynamicJsonDocument` is banned project-wide.** The framework uses ArduinoJson
v7, which replaces `StaticJsonDocument<N>` and `DynamicJsonDocument` with a
unified `JsonDocument` type. All JSON operations use the v7 API:

```cpp
// ✅ Correct — ArduinoJson v7 unified type
JsonDocument doc;

// ❌ Banned — v6 API, causes heap fragmentation
DynamicJsonDocument doc(1024);
StaticJsonDocument<512> doc;  // v6 legacy — use JsonDocument instead
```

---

## 9. Application Lifecycle

### 9.1 Boot State Machine

```
BOOTING
  │  (crypto init, display init, splash)
  │  (filesystem mount, config load)
  │  (WiFi connect attempt)
  ├──→ WIFI_SETUP   (WiFi failed, AP mode active — show WiFi wizard)
  │       ├──→ BOOTING (on successful connect)
  │       └──→ WIFI_SHUTDOWN (10-min AP inactivity timeout)
  │
  │  (NTP sync, web server init, MCP server init, first data fetch)
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
7. NTP sync (asynchronous — never blocks boot; sync status checked periodically)
8. DataManager task start (FreeRTOS background worker, ≥ 8KB stack)
9. Register all module config consumers
10. WebServerManager init (registers all `IWebContributor` routes)
11. McpServer init (registers built-in + all `IMcpContributor` tools)
12. OTA manager init
13. OTA firmware blessing (see §13.3)
14. `_compactAndLock()` — shrink structural arrays, log heap, publish `"system.heap"`
15. → Yield to `tick()` loop

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
6. McpServer tick (Serial JSON-RPC polling)
7. WiFi connection health monitor

Under `NATIVE_BUILD`, `tick()` also calls `_dataManager.processNext()` synchronously
(on hardware this runs on the FreeRTOS background task).

---

## 10. Web Portal Architecture

### 10.1 Base Portal

The framework provides a base web portal as a PROGMEM-hosted Single Page Application.
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

### 10.3 POST Body Handling

POST requests use ESPAsyncWebServer's multi-argument `server.on` with a chunk
body callback:

```cpp
server.on("/api/endpoint", HTTP_POST,
    onRequest,     // called after body received
    nullptr,       // upload handler (unused)
    onBody);       // (request, data, len, index, total)
```

Body size validation is performed **per-handler** by checking `len` and `total`
in the body callback. `AsyncCallbackJsonWebHandler` / `AsyncJson.h` is NOT used.
Single-chunk assumption for small WiFi/Config JSON payloads; deserialized directly
with `deserializeJson(doc, data, len)`.

> **Note:** `AsyncWebServer::setMaxBodySize()` does not exist in the ESPAsyncWebServer
> library version used. All body validation must be manual.

### 10.4 Static Asset Organisation

The portal uses a **hybrid asset model**:
- **Framework SPA shell** — compiled into PROGMEM as gzip-compressed `const uint8_t[]`
  arrays inside the kernel library. Served at ~0ms. Always present with the firmware.
  No LittleFS dependency for the base portal.
- **App-specific tab fragments** — stored in LittleFS under `www/` and served at
  request time by ESPAsyncWebServer. Updated via `pio run --target uploadfs`.

Source directory: `web/portal/` contains the SPA source files.

PROGMEM assets (compiled by `portalBuilder.py`):
```
[kernel library PROGMEM]
  index_html_gz[]   ← SPA shell (HTML)
  app_css_gz[]      ← Base styles
  app_js_gz[]       ← Core SPA logic, tab routing, ApiQueue
  wifi_js_gz[]      ← WiFi tab logic
  system_js_gz[]    ← System tab logic
  ota_js_gz[]       ← OTA tab logic
```

Generated PROGMEM headers (`include/portal_assets/*.h`) MUST include `#ifndef NATIVE_BUILD`
guards with fallback `#define PROGMEM` and `<cstddef>`/`<cstdint>` includes for
native test compatibility.

LittleFS app fragments:
```
data/                      ← LittleFS image root
└── www/                   ← Web portal app-specific assets only
    └── <appname>/         ← One subdirectory per app
        ├── station.html   ← App tab content fragment
        └── station.js     ← App tab logic
```

### 10.5 API Request Serialisation

The SPA serialises API requests via the `ApiQueue` class in `app.js`. All frontend
code MUST use `window.apiFetch(url, options)` — never raw `fetch()` or `XMLHttpRequest`.

- One HTTP request inflight at a time (protects ESP32's limited TCP socket pool)
- Requests are queued and dispatched sequentially
- Yields to event loop between requests to prevent UI blocking

### 10.6 AP Mode Timeout

When in AP (captive portal) mode, the framework tracks API activity:

- `WifiManager::tick()` maintains `lastApiRequestTime`
- Updated via `notifyApiActivity()` callback from API handlers
- After 10 minutes of API inactivity → transitions to `WIFI_SHUTDOWN` state
- Conserves power and limits vulnerability window

### 10.7 API Conventions

All API endpoints follow REST conventions:

- `GET /api/<resource>` — read current state as JSON
- `POST /api/<resource>` — update state, body is JSON, response is `{ok: true}` or error
- `GET /api/tabs` — returns nav tab registry `[{name, endpoint}]`
- `POST /api/mcp` — MCP JSON-RPC endpoint (registered by McpServer via IWebContributor)

---

## 11. Inter-Module Communication

### 11.1 The Core Rule

**Modules never call each other directly.** All inter-module communication
uses one of three framework-mediated mechanisms:

### 11.2 EventBus (Synchronous Publish/Subscribe)

The `EventBus` is the primary mechanism for decoupled cross-module communication.
Modules publish events by string topic; any number of subscribers receive them
without the publisher knowing who (if anyone) is listening.

**Callback signature:**
```cpp
void (*callback)(const char* topic, const void* payload, void* ctx)
```

**Key design facts:**
- Topics are `const char*` strings (e.g., `"ota.progress"`, `"display.message"`)
- Payloads are `const void*` — **the EventBus is payload-type-agnostic**. JSON
  typing (if desired) is the publisher/subscriber's responsibility, not the bus's.
- Subscribers use **static function pointers** with a `void* ctx` context pointer
  (never `std::function` — prevents heap fragmentation on ESP32)
- Publishing is **synchronous** — `publish()` iterates subscribers sequentially
  in the same thread. Payloads are valid **ONLY during callback execution**
  (no copying, no queueing)
- **Capacity limits:** 32 maximum subscribers, 64 bytes maximum topic length
- If no subscribers exist for a topic, the event is silently dropped

**Usage example:**
```cpp
// Publisher (e.g., OTA manager)
OtaProgressPayload payload = {45, "Downloading...", false, false};
app.getEventBus().publish("ota.progress", &payload);

// Subscriber (e.g., display module) — static function + void* ctx
static void onOtaProgress(const char* topic, const void* payload, void* ctx) {
    auto* self = static_cast<DisplayModule*>(ctx);
    auto* p = static_cast<const OtaProgressPayload*>(payload);
    self->updateOtaBoard(p->progress, p->status);
}
app.getEventBus().subscribe("ota.progress", onOtaProgress, this);
```

**Known payload structs:**
- `OtaProgressPayload` — progress percentage, status text, complete flag, success flag
- `DisplayMessagePayload` — message text, duration in ms

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
(private nested class in `Application`) no-ops silently.

### 11.4 Callbacks / std::function (Point-to-Point)

For tight coupling between two known kernel components (e.g., OTA manager → display),
the framework uses typed callbacks registered at setup. This pattern is **only**
used for kernel→kernel-service coupling, never for cross-module communication:

```cpp
otaManager.setProgressCallback([&display](int pct) {
    display.showOtaProgress(pct);
});
```

> **Known platform constraint:** `Arduino.h` defines `#define LOW 0x0` and
> `#define HIGH 0x1`, which collide with the `PriorityTier` enum values.
> `dataManager.cpp` must include explicit `#undef LOW` and `#undef HIGH`
> after including `Arduino.h` (guarded by `#ifndef NATIVE_BUILD`).

---

## 11A. Logging & Observability Architecture

### 11A.1 Design Principles

The framework standardises on **ESP-IDF's `esp_log.h`** as the single logging API.
No module invents custom macros. No module uses `Serial.print()` for logging.
The ESP-IDF logging system provides compile-time stripping, runtime per-module
verbosity control, thread safety (FreeRTOS), and pluggable backends — all of
which are lost when modules use ad-hoc alternatives.

### 11A.2 Unified Logging via `LogShim.h`

All modules include `platform/LogShim.h` instead of `esp_log.h` directly. This
header bridges ESP-IDF's logging to the native GoogleTest build:

```cpp
// include/platform/LogShim.h
#pragma once
#ifndef NATIVE_BUILD
    #include "esp_log.h"
#else
    #include <cstdio>
    #define ESP_LOGE(tag, fmt, ...) printf("E (%s): " fmt "\n", tag, ##__VA_ARGS__)
    #define ESP_LOGW(tag, fmt, ...) printf("W (%s): " fmt "\n", tag, ##__VA_ARGS__)
    #define ESP_LOGI(tag, fmt, ...) printf("I (%s): " fmt "\n", tag, ##__VA_ARGS__)
    #define ESP_LOGD(tag, fmt, ...) printf("D (%s): " fmt "\n", tag, ##__VA_ARGS__)
    #define ESP_LOGV(tag, fmt, ...) printf("V (%s): " fmt "\n", tag, ##__VA_ARGS__)
#endif
```

**Module logging pattern** (every `.cpp` file):
```cpp
#include "platform/LogShim.h"
static const char* TAG = "MyModule";

ESP_LOGI(TAG, "Initialized with %d items", count);
ESP_LOGW(TAG, "Timeout after %lu ms", elapsed);
ESP_LOGE(TAG, "Fatal: filesystem mount failed");
ESP_LOGD(TAG, "Scheduling source '%s' in %lu ms", name, interval);
```

### 11A.3 Three-Tier Log Level Control

| Tier | When | Mechanism | Effect |
|------|------|-----------|--------|
| **Compile-time ceiling** | Build | `CORE_DEBUG_LEVEL` in `platformio.ini` | Logs above this level are **dead-code eliminated** — format strings removed from binary |
| **Boot default** | Startup | `CONFIG_LOG_DEFAULT_LEVEL` in `sdkconfig.defaults` | What prints before any runtime configuration |
| **Runtime per-module** | Anytime | `esp_log_level_set("WiFiManager", ESP_LOG_DEBUG)` | Dynamically crank one module to verbose without rebuilding |

**Build flag configuration:**
```ini
# Development — see info, warnings, errors
build_flags = -DCORE_DEBUG_LEVEL=3

# Production — errors only, all other strings stripped from binary
build_flags = -DCORE_DEBUG_LEVEL=1
```

### 11A.4 LogRouter Pipeline

The framework installs a custom `esp_log_set_vprintf()` handler during
`Application::begin()`. This handler forms a **log pipeline**:

```
ESP_LOGI(TAG, "...")   → esp_log_write()
  → LogRouter (custom vprintf handler)
    → Step 1: Format the string
    → Step 2: Redact secrets (Logger::redactInPlace)
    → Step 3: Write to Serial (always)
    → Step 4: Write to RAM ring buffer (always, ~4-8KB)
    → Step 5: Dispatch to registered ILogSink implementations
```

**Secret redaction** is applied globally via the `Logger` library's
`redactInPlace()` function. Any string registered as a secret via
`Logger::registerSecret()` is automatically masked with `*` characters
in all log output. This is safe for development use; production builds
strip most log output entirely via `CORE_DEBUG_LEVEL`.

### 11A.5 `ILogSink` Interface

```cpp
class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void write(const char* formattedLine, size_t len) = 0;
};
```

Registered via `app.registerLogSink(ILogSink*)`. The LogRouter dispatches
formatted, redacted log lines to all registered sinks after writing to Serial.
This follows the same Hollywood Principle pattern as `IWebContributor` and
`IMcpContributor`.

### 11A.6 RAM Ring Buffer & Crash Persistence

The LogRouter maintains a **RAM ring buffer** (~4-8KB) that always contains
the most recent log output. This buffer serves two purposes:

1. **Live telemetry**: Optional `ILogSink` implementations (e.g., MQTT) read
   from the buffer and flush to their transport periodically.
2. **Post-mortem "black box"**: On panic/crash, a registered panic handler
   dumps the ring buffer contents to a small LittleFS file. On next boot,
   the application reads the crash log, optionally publishes it, and deletes it.

**Why RAM, not flash?**
- Flash write endurance is 10,000–100,000 erase cycles per sector
- Continuous logging at ~10 lines/sec would kill the flash partition in ~4 months
- Flash writes take 20–200ms, violating the ≤10ms tick budget if triggered from
  a log call
- RAM ring buffer has zero write latency and infinite write endurance
- Flash persistence is only used on crash — maybe once a week/month — negligible
  wear

### 11A.7 Heartbeat Telemetry

The `Application::tick()` loop emits a periodic heartbeat log every ~15 seconds
containing:
- Free heap (via `heap_caps_get_free_size(MALLOC_CAP_8BIT)`)
- Minimum free heap watermark (via `heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT)`)
- Uptime in seconds
- WiFi RSSI (if connected)

This provides continuous runtime observability without requiring any external
monitoring infrastructure — just a serial connection.

### 11A.8 Optional MQTT Telemetry Module

An optional module (`mqttTelemetry`) can be added to any application for remote
log forwarding. It implements `ILogSink` and:

- Reads from the LogRouter's RAM ring buffer
- A background FreeRTOS task flushes batched log lines to the MQTT broker
  periodically (e.g., every 5-15 seconds)
- Publishes to a structured topic: `devices/{device_id}/logs`
- If MQTT is disconnected, it silently drops the oldest entries (never blocks)
- The `esp_log_set_vprintf()` call path is **never blocked** by network I/O

This module is **not part of the kernel** — it is an optional module that
applications include if they need remote observability.

### 11A.9 Module Logging Requirements

Every module in the framework MUST include logging at the following lifecycle
points:

| Lifecycle Point | Level | Purpose |
|----------------|-------|--------|
| `begin()` / init | `ESP_LOGI` | Confirm initialisation, log key parameters |
| State transitions | `ESP_LOGI` | Boot phase changes, sleep/wake, sync completion |
| Registration | `ESP_LOGD` | Source/board/tool registration (debug level) |
| Error paths | `ESP_LOGE` | All error returns, failed operations |
| Degraded paths | `ESP_LOGW` | Timeouts, fallbacks, rejected registrations |
| Hot-path detail | `ESP_LOGD` | Per-fetch, per-dispatch detail (stripped in production) |

**Prohibited patterns:**
- `Serial.print()` / `Serial.printf()` for logging
- Per-module custom macro shims (e.g., `MY_MODULE_LOGI`)
- Logging inside `tick()` at `ESP_LOGI` level for per-frame events (use `ESP_LOGD`)

---

## 12. Repository & Build Architecture

### 12.1 Repository Topology

The framework is a **monorepo** — kernel, optional modules, scripts, template,
and documentation all live in a single repository.

#### Architectural Rationale: Monorepo vs Multi-repo

The framework deliberately uses a monorepo topology rather than splitting into separate repositories (e.g., `esp32-app-foundation`, `esp-module-display`, `esp-module-mqtt`). This architectural decision solves several critical challenges inherent to embedded C++ development in PlatformIO:

1. **Avoiding the "Version Matrix of Hell":** Multi-repo architectures require tight version orchestration. If a kernel interface changes, all module repos must be updated. App developers easily end up with incompatible version combinations in their `lib_deps`. A monorepo guarantees that every commit is a fully compatible, API-synchronised snapshot.
2. **Rock-Solid OTA Reproducibility:** Pinning the monorepo via a single git submodule SHA ensures that the exact state of the kernel *and* all optional modules is recorded. If a deployed firmware crashes, the developer knows exactly what was compiled into it.
3. **Atomic Refactoring:** Sweeping architectural changes can be executed and tested atomically in a single PR, rather than requiring synchronized merges across multiple repositories.
4. **Linker Efficiency:** The monorepo does *not* cause firmware bloat. PlatformIO and the C++ linker only compile and link the modules explicitly `#include`'d in the application code. Unused modules take up zero bytes on the ESP32 flash.

*Decision:* Keep it as a Monorepo for `v1.x`. While separating mature modules into their own repositories might encourage third-party development later (e.g., `v2.x`), doing so prematurely creates unnecessary fragmentation and versioning friction.

```
esp32-app-foundation/           ← Monorepo (everything lives here)
  include/                      ← All framework interface headers + FrameworkConfig
    interfaces/                 ← Pure abstract interfaces (ITickable, etc.)
    platform/                   ← Platform abstraction headers
      LogShim.h                 ← Unified ESP-IDF ↔ native logging bridge
    portal_assets/              ← Generated PROGMEM gzip headers (build output)
    AppFoundation.h             ← Canonical single-include façade
    AppState.hpp                ← BOOTING, WIFI_SETUP, APP_SETUP, RUNNING
    FrameworkConfig.hpp         ← Kernel config struct
  lib/                          ← Low-level utilities
    deviceCrypto/
    logger/
    md5Utils/
    githubClient/
  modules/
    application/                ← Kernel: Application hub + AppState
    configManager/              ← Kernel: LittleFS JSON config
    keyManager/                 ← Kernel: encrypted credential store
    wifiManager/                ← Kernel: WiFi + captive portal
    timeManager/                ← Kernel: NTP + POSIX timezone
    otaUpdateManager/           ← Kernel: GitHub OTA + rollback
    webServer/                  ← Kernel: ESPAsyncWebServer + REST API
    eventBus/                   ← Kernel: typed pub/sub
    dataManager/                ← Kernel: FreeRTOS background scheduler
    mcpServer/                  ← Kernel: JSON-RPC MCP server
    u8g2Display/                ← Optional: U8G2 monochrome display module
      widgets/                  ← Widget toolkit (3-tier)
      fonts/                    ← Font management
      boards/interfaces/        ← iBoardLayout
  scripts/                      ← Build scripts
  web/portal/                   ← Web portal SPA source
  test/                         ← All tests
    test_unit/                  ← GoogleTest native — no hardware required
    test_hardware/              ← Hardware integration tests
    test_e2e/                   ← Full boot-to-RUNNING integration test
  template/                     ← Starter skeleton for new apps
    src/main.cpp                ← 3-line entry point
    src/myApp.hpp/.cpp          ← Skeleton app class
    platformio.ini              ← lib_deps = framework/
    README.md                   ← "How to start a new app" guide
  docs/                         ← This document
  partitions.esp32dev.csv
  partitions.esp32s3nano.csv
  sdkconfig.defaults
  sdkconfig.defaults.esp32s3nano
  library.json                  ← PlatformIO library manifest
```

### 12.2 How Apps Consume the Framework

Apps add the framework as a **git submodule** pinned to a commit SHA:

```bash
git submodule add https://github.com/matthewmcneill/esp32-app-foundation.git framework
git submodule update --init
```

App `platformio.ini`:
```ini
[env:esp32dev]
platform = espressif32
framework = arduino, espidf
lib_extra_dirs =
    framework/modules
    framework/lib
lib_deps =
    framework/                  ; local submodule path — no registry, no absolute paths
    olikraus/U8g2@2.36.18
    bblanchon/ArduinoJson@7.4.3
    ESP32Async/ESPAsyncWebServer
extra_scripts =
    pre:framework/scripts/build_timestamp.py
build_flags =
    -DAPP_FOUNDATION_VERSION=\"0.1.0\"
```

> **Warning:** `build_src_filter` does NOT work with ESP-IDF (it uses `CMakeLists.txt`
> for component discovery). Use `lib_extra_dirs` + explicit `lib_deps` instead.

### 12.3 Local Development Workflow

When actively developing the framework alongside an app, the submodule points at
a local checkout. Push changes to the framework repo; update the submodule SHA
when ready to adopt a new framework version.

### 12.4 Build Scripts

Build scripts ship in the framework repo under `scripts/`.

| Script | Type | Purpose |
|---|---|---|
| `build_timestamp.py` | PIO hook (`extra_scripts`) | Injects `BUILD_TIME` and `BUILD_DATE` macros via SCons `Import("env")` |
| `build_fonts.py` | Standalone CLI | Compiles custom U8G2 fonts → C++ headers. Run manually when font files change |
| `generate_layouts.py` | Standalone CLI | Generates display layout constants from YAML/JSON → C++ headers. Run manually when layouts change |
| `portalBuilder.py` | PIO hook | Packages `web/portal/` assets → PROGMEM gzip headers in `include/portal_assets/` |

> **Standalone vs hook:** `build_fonts.py` and `generate_layouts.py` are deliberately
> NOT PIO hooks — hooking them into compile-time would break builds if downstream
> projects lack the input files (BDF fonts, YAML layouts). Run them manually, commit
> the generated headers.

### 12.5 CI Workflow

The framework provides `.github/workflows/ci.yml`:

1. **Dual-env firmware build**: `esp32dev` + `esp32s3nano`
2. **Native test execution**: `pio test -e native` (GoogleTest)
3. **Include-graph lint**: Verifies no kernel header imports a module header

### 12.6 PlatformIO LDF Constraints

> **Critical:** PlatformIO's Library Dependency Finder (LDF) scans `#include`
> directives as **text patterns** regardless of `#ifdef` preprocessor guards.
> A conditional `#include "Application.hpp"` inside `#ifndef NATIVE_BUILD` will
> still trigger LDF to resolve and compile the entire dependency chain.

**Mitigation strategies:**
- Forward-declare types in headers; include only in `.cpp` files behind guards
- `library.json` files should have NO `"platforms"` or `"frameworks"` restrictions
  (unless the library is genuinely platform-specific)
- Never include `Application.hpp` in display module native-visible translation units
- Use `lib_extra_dirs = modules` so headers resolve by basename (e.g.,
  `#include "EventBus.hpp"` without directory prefix)

---

## 13. Security Architecture

### 13.1 DeviceCrypto

The framework includes a hardware-backed cryptographic engine (`DeviceCrypto`)
that derives a unique device key from the ESP32's hardware MAC/eFuse and uses it
for symmetric encryption of sensitive configuration values (API tokens, passwords).

- Keys are **never stored in plaintext** on LittleFS
- Encrypted values use AES-256 with the device-unique key
- The device key is non-exportable — encrypted config is device-specific
- All cryptographic operations use **ESP-IDF/MbedTLS** hardware-accelerated APIs
  (not standard C++ library crypto)

### 13.2 KeyManager & API Key Registry

Key and credential management is a distinct concern from configuration persistence
and is handled by a dedicated `KeyManager` kernel component:

- `KeyManager` depends on `DeviceCrypto` (initialised first) for encryption/decryption
- API keys and tokens are stored in `/etc/apikeys.json` in encrypted form
- **CRUD API**: `addKey(id, secret)`, `getKeyById(id, buffer, size)`, `deleteKey(id)`
  — ConfigManager has no knowledge of key content
- Keys are referenced by opaque IDs (e.g., `k-12345678`) in module configs — the
  ID is not sensitive; only the resolved key value is
- Modules retrieve keys at runtime via `app.getKeyManager().getKeyById(id)`
- **Plaintext credential storage is prohibited** — all credentials must be AES-256
  encrypted via DeviceCrypto before writing to flash

### 13.3 OTA Security & Blessing Ceremony

The framework provides a complete OTA firmware update pipeline: release discovery
→ binary download → cryptographic signature verification → flash → reboot. The
pipeline replaces the earlier MD5-only verification with full RSA+SHA256 signature
validation, providing tamper-proof firmware authenticity when configured with a
public key.

#### 13.3.1 Architecture Overview

The OTA subsystem is composed of three layers, each with a distinct responsibility:

| Layer | Location | Responsibility |
|-------|----------|----------------|
| **`otaUpdateManager`** | `modules/otaUpdateManager/` | Orchestrator — state machine, FreeRTOS task, EventBus progress, REST API routes |
| **`OtaDownloadEngine`** | `lib/otaDownloadEngine/` | Utility — redirect-aware HTTP download, binary header verification, chunked streaming flash, verifier integration |
| **`IOtaVerifier` / `MbedTLSSignatureVerifier`** | `include/interfaces/` + `lib/otaDownloadEngine/` | Interface + default impl — RSA+SHA256 signature verification via mbedtls |

The three layers follow the Hollywood Principle: `otaUpdateManager` (a module)
depends on `OtaDownloadEngine` and `IOtaVerifier` (both in `lib/`), but the
libraries have no knowledge of the module. The download engine accepts a
`WiFiClient&` from the caller and never creates its own network client.

#### 13.3.2 Cryptographic Verification Pipeline

The `IOtaVerifier` interface defines a three-step verification contract:

```cpp
class IOtaVerifier {
public:
    virtual bool begin(const char* publicKeyPem) = 0;   // Load key + init digest
    virtual void update(const uint8_t* data, size_t length) = 0;  // Per-chunk hash
    virtual bool verify(const uint8_t* signature, size_t signatureLength) = 0;  // Validate
};
```

**Default implementation: `MbedTLSSignatureVerifier`**

- Uses `mbedtls_pk_context` for public key parsing and `mbedtls_md_context_t`
  for SHA-256 digest accumulation
- RAII lifecycle — destructor calls `mbedtls_pk_free()` + `mbedtls_sha256_free()`,
  preventing resource leaks even on error paths
- The public key is PEM-encoded RSA, injected at runtime via
  `FrameworkConfig.otaPublicKey` — never hardcoded in the framework
- Non-copyable, non-movable (owns mbedtls context handles)
- On native builds (`NATIVE_BUILD`), stub implementations compile cleanly but
  return failure, enabling interface contract tests without crypto dependencies

**Degraded security mode:** If `FrameworkConfig.otaPublicKey` is `nullptr`,
signature verification is **skipped entirely**. The download proceeds without
cryptographic validation, and a warning is logged at `ESP_LOGW` level. This
allows development and testing without key infrastructure but should never be
used in production deployments.

#### 13.3.3 Download Engine

`OtaDownloadEngine` handles the complete binary download pipeline:

1. **Redirect-following HTTP GET** — up to 5 hops (`kMaxRedirects`), using
   HTTP/1.0 to avoid chunked-transfer encoding complications on the ESP32
2. **Separate `.sig` file fetch** — `fetchSignature()` downloads the detached
   signature file with its own redirect chain
3. **ESP32 binary header validation** — verifies the first byte is the ESP32
   magic byte `0xE9`, rejecting non-firmware payloads before any flash write
4. **Partition space validation** — checks `Content-Length` against
   `ESP.getFreeSketchSpace()`, failing early with `OtaError::InsufficientSpace`
   if the binary exceeds the available OTA partition
5. **Chunked streaming write** — reads the binary in 1024-byte chunks
   (`kChunkSize`), writing each chunk to the flash via `Update.write()` and
   feeding it to `IOtaVerifier::update()` for hash accumulation
6. **Post-download signature verification** — calls `IOtaVerifier::verify()`
   against the fetched `.sig` file contents
7. **Progress callbacks** — invokes `OtaProgressCallback(bytesWritten, totalSize)`
   after each chunk for UI integration

**Error codes** (`OtaError` enum):

| Code | Value | Description |
|------|-------|-------------|
| `None` | 0 | Success |
| `HttpConnectionFailed` | -1 | TCP/TLS connection could not be established |
| `HttpWrongResponseCode` | -2 | Unexpected HTTP status code |
| `HttpFileNotFound` | -3 | Server returned 404 |
| `HttpForbidden` | -4 | Server returned 403 |
| `ServerNoContentLength` | -5 | Server did not report Content-Length |
| `TooManyRedirects` | -6 | Exceeded 5 redirect hops |
| `InsufficientSpace` | -7 | Binary exceeds free sketch space |
| `NoOtaPartition` | -8 | No OTA partition available |
| `BinaryHeaderInvalid` | -9 | First byte is not `0xE9` |
| `SignatureFetchFailed` | -10 | Failed to download `.sig` file |
| `SignatureVerifyFailed` | -11 | Cryptographic verification failed |
| `UpdateBeginFailed` | -12 | `Update.begin()` returned false |
| `UpdateWriteFailed` | -13 | `Update.write()` wrote fewer bytes than expected |
| `UpdateEndFailed` | -14 | `Update.end()` returned false |
| `StreamReadTimeout` | -15 | Stream read timed out during download |

Each error maps to a human-readable string via `otaErrorToString()`.

> **Design constraint:** The engine does NOT own the `WiFiClient` — the caller
> passes a configured client by reference. This allows the orchestrator to
> control TLS configuration and heap allocation per §14.3.

#### 13.3.4 Orchestrator State Machine

`otaUpdateManager` manages the OTA lifecycle through the `OtaState` enum:

```
Idle → Checking → Idle              (no update found)
Idle → Downloading → Verifying → Complete → (reboot)
Any  → Error                        (on failure)
```

| State | Description |
|-------|-------------|
| `Idle` | No OTA activity — ready to accept a check or update |
| `Checking` | Querying GitHub for latest release metadata |
| `Downloading` | FreeRTOS task running the download pipeline |
| `Verifying` | Signature verification in progress |
| `Complete` | Update finalised — waiting for reboot |
| `Error` | An error occurred (query `getLastOtaError()` for details) |

**`checkForUpdate()`** creates a `GithubClient` with the repo and token from
`FrameworkConfig`, queries the latest release, and returns `true` if a new
release is available. Guards: must be in `Idle` state with a configured
`otaGithubRepo`.

**`performUpdate()`** launches a FreeRTOS task (8KB stack, priority 1) that
executes the full 8-step pipeline:

1. Check GitHub for releases via `GithubClient`
2. Find `.bin` and `.sig` assets in the release
3. Initialise `MbedTLSSignatureVerifier` (if `otaPublicKey` configured)
4. Allocate `WiFiClientSecure` on the heap (per §14.3)
5. Create and configure `OtaDownloadEngine`
6. Fetch the `.sig` signature file
7. Download and flash the firmware binary
8. Finalise and reboot (2-second delay for event propagation)

The FreeRTOS task ensures the `tick() ≤ 10ms` constraint is never violated by
blocking download I/O. On native builds, `performUpdate()` sets `OtaState::Error`
(no FreeRTOS available).

#### 13.3.5 FrameworkConfig OTA Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `firmwareUpdatesEnabled` | `bool` | `true` | Master kill-switch for OTA update checks |
| `otaQuietHour` | `uint8_t` | `3` | Hour (0–23) during which OTA checks are suppressed |
| `otaPublicKey` | `const char*` | `nullptr` | PEM-encoded RSA public key for signature verification. `nullptr` = skip verification (degraded mode) |
| `otaGithubRepo` | `const char*` | `nullptr` | GitHub repository in `"owner/repo"` format for OTA releases |
| `otaGithubToken` | `const char*` | `nullptr` | Optional GitHub access token for private repos |

All OTA config fields have safe defaults so the framework boots without
requiring any OTA configuration. The consuming app populates these fields
during the `BOOTING` phase via `FrameworkConfig`.

#### 13.3.6 EventBus Integration

The orchestrator publishes `OtaProgressEvent` on topic `"ota.progress"`:

```cpp
struct OtaProgressEvent {
    float progress;       // 0.0 to 1.0 download fraction
    const char* status;   // Human-readable status message
    bool complete;        // True when OTA cycle is finished
    bool success;         // True if OTA succeeded (only valid when complete)
};
```

`OtaProgressEvent` is **layout-compatible** with `OtaProgressPayload` in
`u8g2DisplayModule.hpp` — the display module subscribes to `"ota.progress"` and
can safely cast the payload to either type for rendering the `OtaProgressBoard`.

Events are published:
- At each major pipeline step (checking, downloading, verifying, complete, error)
- On every progress callback from the download engine (per-chunk granularity)
- With `complete=true` on both success and failure (check `success` to distinguish)

#### 13.3.7 Firmware Blessing Ceremony

After the OTA pipeline completes and the device reboots, the new firmware enters
a "pending verification" state managed by the ESP-IDF bootloader:

1. New firmware boots in "pending verification" state
2. The **`Application` hub** (not `otaUpdateManager`) calls
   `esp_ota_mark_app_valid_cancel_rollback()` after verifying:
   - WiFi connectivity established
   - LittleFS mounted successfully
   - Core services initialised without fatal errors
3. If blessing never occurs, the bootloader automatically reverts to the previous
   firmware on next restart

> **Prohibited:** Blessing the partition at download/write time. This defeats the
> rollback purpose — firmware could still have fatal startup or runtime crashes.

This requires `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` in `sdkconfig.defaults`
(see §14.3).

#### 13.3.8 REST API

The `otaUpdateManager` registers the following routes via `IWebContributor`:

| Method | Route | Action | Description |
|--------|-------|--------|-------------|
| `GET` | `/api/ota` | — | Returns `{ "status": "ready" }` |
| `POST` | `/api/ota` | `"checkGithub"` | Triggers `checkForUpdate()` + `performUpdate()` if an update is available |
| `POST` | `/api/ota` | `"status"` | Returns `{ "state": "idle\|checking\|downloading\|verifying\|complete\|error", "lastError": "..." }` |
| `POST` | `/api/ota` | `"rollback"` | Triggers `rollbackFirmware()` — reverts to previous partition and reboots |
| `POST` | `/api/ota` | `"markValid"` | Calls `markAppValid()` — blesses the running firmware |

The `checkGithub` action is the primary entry point for web-triggered OTA
updates. It performs the check and immediately starts the update pipeline if a
new release is found.

#### 13.3.9 Security Considerations

- **Public key management:** The consuming app provides the PEM-encoded RSA
  public key via `FrameworkConfig.otaPublicKey`. The framework never hardcodes
  key material — this is an app-side responsibility
- **WiFiClientSecure allocation:** Always heap-allocated via
  `std::unique_ptr<WiFiClientSecure>(new (std::nothrow) WiFiClientSecure())`
  per §14.3 memory rules — never on the stack
- **GitHub CDN TLS:** Uses `setInsecure()` for CDN redirect hops because GitHub's
  CDN endpoints use different certificate chains than `api.github.com`. The
  initial API call to `api.github.com` uses the standard cert chain
- **Degraded mode warning:** If no public key is configured, downloads proceed
  without signature verification. A warning is logged at `ESP_LOGW` level. This
  is acceptable for development but should be addressed before production
  deployment
- **Rollback safety:** The blessing ceremony (§13.3.7) ensures that bad firmware
  is automatically reverted by the bootloader if core services fail to initialise
- **Legacy direct-upload path:** The web-upload OTA route
  (`beginUpdate`/`writeStream`/`endUpdate`) is preserved alongside the new
  GitHub pipeline. It uses MD5 verification via `Update.setMD5()` — this path
  does not use RSA+SHA256 signature verification

### 13.4 Web Portal Security

- The portal is only accessible on the local network (no cloud dependency)
- No authentication is implemented by default (local network trust model)
- Future: optional HTTP Basic Auth configurable via `FrameworkConfig`

### 13.5 MCP Security

- `FrameworkConfig.mcpReadOnly` toggle (default `false`)
- Each tool in `McpToolEntry` has a `bool mutatesState` metadata field (default `false`)
- When `mcpReadOnly == true`, tools with `mutatesState == true` return JSON-RPC
  error code `-32000` ("Read-only mode")
- HTTP transport (`POST /api/mcp`) inherits the same security checks as Serial
  transport — both call the same `processRequest()` core

---

## 14. Resource Budget & Constraints

### 14.1 Target Hardware & Partition Layout

| Parameter | esp32dev (4MB) | esp32s3nano (8MB+) |
|---|---|---|
| Flash | 4MB | 8MB+ |
| DRAM | 320KB (usable ~200KB) | 512KB |
| IRAM | 128KB | 128KB |
| PSRAM | Not assumed | Board-dependent |
| OTA Partition A | 1280KB | 2MB |
| OTA Partition B | 1280KB | 2MB |
| LittleFS | 896KB | 3.75MB |

> **Note:** esp32dev partition layout was resized from the original 960KB OTA / 1.5MB
> LittleFS to accommodate U8G2 font tables in the firmware binary.

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

**Required SDK configuration:**
- `CONFIG_AUTOSTART_ARDUINO=y` in `sdkconfig.defaults` — mandatory for dual-framework
  (Arduino + ESP-IDF) projects. Without it, Arduino's `app_main()` wrapper fails
  to link against ESP-IDF's `app_startup.c`.
- `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` — enables OTA rollback.

**Known platform constraint:**
- `Arduino.h` defines `#define LOW 0x0` and `#define HIGH 0x1`, which collide with
  the `PriorityTier` enum. `dataManager.cpp` must `#undef LOW` and `#undef HIGH`
  after including `Arduino.h`.

### 14.4 Registration Array Model

There are **no `MAX_*` compile-time ceilings**. The framework sizes arrays exactly
to what the application registers — no waste, no artificial constraints.

**Structural arrays** (rebuilt at boot/soft-reset, `shrink_to_fit()` after build):

| Array | Type | Mutation Policy |
|---|---|---|
| `ITickable*` | `std::vector` | Soft-reset only |
| `IDisplayBoard*` | `std::vector` | Soft-reset only |
| `iDataSource*` | `std::vector` | Soft-reset only |
| `IMcpContributor*` | `std::vector` | Soft-reset only |

**Lightweight arrays** (may grow at any time):

| Array | Type | Mutation Policy |
|---|---|---|
| `iConfigurable*` | `std::vector` | Append any time |
| `IWebContributor*` | `std::vector` | Append any time |
| `KeyManager` entries | `std::vector` | Append any time |

**After each build phase**, the framework logs heap stats and publishes:
```cpp
app.getEventBus().publish("system.heap", &heapPayload);
```
The web portal's System tab consumes this directly.

### 14.5 Boot-Phase Allocation Pattern (Reference Implementation)

This section documents how the two-tier vector model is implemented inside the
`Application` hub. It is the normative reference for any contributor implementing
or extending the hub.

#### The Three Lifecycle Phases

```
╔══════════════════════════════════════════════════════════════════════╗
║  PHASE 1: REGISTRATION                                           ║
║  begin() open — all register*() calls accepted                  ║
║  std::vector grows via push_back() (amortised doubling)         ║
╠══════════════════════════════════════════════════════════════════════╣
║  PHASE 2: COMPACTION                                             ║
║  Structural arrays: shrink_to_fit() releases over-capacity      ║
║  Heap stats logged + 'system.heap' EventBus event fired         ║
║  _locked = true  ← structural arrays are now immutable          ║
╠══════════════════════════════════════════════════════════════════════╣
║  PHASE 3: STEADY-STATE  (tick() loop)                           ║
║  Structural: reads only — zero allocation, zero deallocation    ║
║  Lightweight: append freely — no lock, no shrink                ║
║  Any call to register structural array → assert / panic         ║
╚══════════════════════════════════════════════════════════════════════╝
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
    void registerMcpContributor(IMcpContributor* mc) {
        ASSERT_MESSAGE(!_locked, "registerMcpContributor() after begin()");
        _mcpContributors.push_back(mc);
    }

    // ── Lightweight registration (any phase) ────────────────────
    void registerConfigConsumer(iConfigurable* c)  { _configConsumers.push_back(c); }
    void registerWebContributor(IWebContributor* w) { _webContributors.push_back(w); }

    // ── Kernel service accessors ────────────────────────────────
    EventBus& getEventBus()                { return _eventBus; }
    DataManager& getDataManager()          { return _dataManager; }
    WebServerManager& getWebServerManager(){ return _webServerManager; }
    McpServer& getMcpServer()              { return _mcpServer; }
    IStatusReporter& getStatusReporter();

    // ── Lifecycle ───────────────────────────────────────────────
    void begin(const FrameworkConfig& config);
    void tick();
    void softReset();

private:
    // ── Kernel services (owned by value) ─────────────────────────
    EventBus          _eventBus;
    DataManager       _dataManager;
    WebServerManager  _webServerManager;
    McpServer         _mcpServer;

    // ── Structural arrays ────────────────────────────────────────
    std::vector<ITickable*>        _tickables;
    std::vector<iDataSource*>      _dataSources;
    std::vector<IMcpContributor*>  _mcpContributors;
    bool                           _locked = false;

    // ── Lightweight arrays ───────────────────────────────────────
    std::vector<iConfigurable*>    _configConsumers;
    std::vector<IWebContributor*>  _webContributors;

    // ── Status reporter fallback ─────────────────────────────────
    class NullStatusReporter : public IStatusReporter {
        void reportMessage(const char*, const char*) override {}
        void reportProgress(const char*, int) override {}
        void reportError(const char*, const char*) override {}
    };
    NullStatusReporter _nullReporter;

    void _compactAndLock();
    void _reportHeap();
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
     b. _tickables.clear() / _dataSources.clear() / _mcpContributors.clear()
     c. _onRegister() — app re-registers all modules (including new one)
     d. _compactAndLock() — shrink_to_fit + heap report + _locked = true
5. tick() resumes with the updated arrays
```

The deferred flag pattern mirrors the existing config reload flag — structural
changes are never applied mid-dispatch.

### 14.6 Build Profiles & Compiler Optimization

The framework uses aggressive binary size optimization to maximise headroom on
the 4MB esp32dev target. All settings are in `sdkconfig.defaults` and apply to
both esp32dev and esp32s3nano environments unless overridden by
`sdkconfig.defaults.<env>`.

#### 14.6.1 Compiler Optimization: `-Os`

The production build uses `CONFIG_COMPILER_OPTIMIZATION_SIZE=y` (`-Os`), which
enables the same optimization passes as `-O2` (the "fast" flag) but disables
optimizations that increase code size — specifically loop unrolling and
aggressive function inlining.

**Performance impact on ESP32: negligible to beneficial.**

- The ESP32 executes code from external SPI flash via a 32KB instruction cache.
  Smaller code means fewer cache misses, which can make `-Os` *faster* than
  `-O2` on cache-constrained microcontrollers.
- The framework is I/O-bound — WiFi packets (milliseconds), HTTP responses
  (hundreds of milliseconds), display refresh (I2C at 400KHz), and `tick()`
  sleep intervals (10ms+). The 10–20% instruction-level overhead of `-Os` vs
  `-O2` on tight computational loops is invisible when the CPU is idle 99%+ of
  the time.
- ESP-IDF documentation recommends `-Os` as the standard production setting.
  The default `-Og` exists specifically for debugger-friendly code layout, not
  for performance.
- The only workloads where `-Os` has measurable impact are pure CPU tasks like
  mbedTLS RSA signature verification (~10–20% slower) and large JSON parsing
  (~5–15% slower). Both are infrequent, one-shot operations (OTA updates and
  API responses) where sub-millisecond differences are irrelevant.

#### 14.6.2 Logging Strategy

The framework maintains **208 active log call sites** (82 ERROR, 40 WARN,
86 INFO) across all modules, consuming approximately 8.5KB of format strings
in the binary. An additional 15 DEBUG-level call sites exist in the source but
are stripped at compile time.

**Production builds use `CONFIG_LOG_MAXIMUM_LEVEL=3` (INFO).** This is a
**compile-time ceiling** — the preprocessor completely removes `ESP_LOGD` and
`ESP_LOGV` calls from the binary. They cannot be re-enabled at runtime via
`esp_log_level_set()` without recompiling.

| Level | Macro | Ceiling | Status in Production |
|-------|-------|:---:|---|
| ERROR (1) | `ESP_LOGE` | ≤3 | ✅ Always compiled in |
| WARN (2) | `ESP_LOGW` | ≤3 | ✅ Always compiled in |
| INFO (3) | `ESP_LOGI` | ≤3 | ✅ Always compiled in |
| DEBUG (4) | `ESP_LOGD` | >3 | ❌ Stripped by preprocessor |
| VERBOSE (5) | `ESP_LOGV` | >3 | ❌ Stripped by preprocessor |

**Rationale for keeping INFO in production:**

- INFO provides full lifecycle visibility: boot sequence, WiFi connection,
  OTA progress, state transitions, module initialisation. These are essential
  for field diagnostics when a USB cable is the only debugging tool.
- Dropping to WARN would save only ~5.6KB (0.7% of the binary) — not
  meaningful given the 469KB of available headroom.
- The serial port is shared with the MCP server, making logging a first-class
  diagnostic channel alongside structured MCP tool queries.
- The framework invested significant effort instrumenting all modules with
  consistent `ESP_LOGx` calls via `LogShim.h`. Stripping them at compile time
  would undermine that observability architecture.

**Runtime log level control** remains available within the compiled ceiling.
An app can call `esp_log_level_set("McpServer", ESP_LOG_ERROR)` to silence a
noisy module at runtime without recompiling, as long as the requested level
is ≤ the compile-time maximum (3 = INFO).

#### 14.6.3 Build Profiles

Three build profiles are supported via `sdkconfig.defaults` overrides. The
production profile is the default; debug and release profiles can be activated
by creating environment-specific override files or by modifying
`sdkconfig.defaults` before building.

**Production (default):**

| Setting | Value | Effect |
|---------|-------|--------|
| `CONFIG_COMPILER_OPTIMIZATION_SIZE` | `y` | `-Os` compiler flag |
| `CONFIG_NEWLIB_NANO_FORMAT` | `y` | Smaller `printf`/`scanf` (~25–50KB savings) |
| `CONFIG_LOG_MAXIMUM_LEVEL` | `3` (INFO) | DEBUG/VERBOSE stripped at compile time |
| `CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_SILENT` | `y` | `assert()` aborts without file/line strings |
| `CONFIG_ESP_ERR_TO_NAME_LOOKUP` | `n` | `esp_err_to_name()` returns `"UNKNOWN"` |

**Debug** (for development — override with `CONFIG_LOG_MAXIMUM_LEVEL=5` etc.):

| Setting | Value | Effect |
|---------|-------|--------|
| `CONFIG_COMPILER_OPTIMIZATION_DEBUG` | `y` | `-Og` for debugger-friendly code |
| `CONFIG_NEWLIB_NANO_FORMAT` | `n` | Full `printf` formatting |
| `CONFIG_LOG_MAXIMUM_LEVEL` | `5` (VERBOSE) | All log levels available at runtime |
| `CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_DISABLE` | `n` | Full assertion messages with file/line |
| `CONFIG_ESP_ERR_TO_NAME_LOOKUP` | `y` | Human-readable error names |

> **Note:** The debug profile increases binary size by ~220KB. On the 4MB
> esp32dev target this exceeds 80% flash usage. Debug builds are recommended
> only on the esp32s3nano (8MB) target or when temporarily overriding for a
> specific investigation on esp32dev.

**Release** (maximum size reduction for space-constrained deployments):

| Setting | Value | Effect |
|---------|-------|--------|
| All production settings | — | As above |
| `CONFIG_LOG_MAXIMUM_LEVEL` | `2` (WARN) | INFO stripped, saves ~5.6KB |
| `CONFIG_LOG_DEFAULT_LEVEL` | `1` (ERROR) | Only errors emitted at runtime |

> **Warning:** The release profile strips INFO-level lifecycle logging. Serial
> monitoring will show only warnings and errors. Use only for final production
> deployments where serial diagnostics are not expected.

#### 14.6.4 Observability Implications of Silent Assertions & Error Lookup

Two production settings reduce diagnostic clarity in exchange for flash savings:

**`CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_SILENT=y`** — when `assert()` or
`configASSERT()` fires, the device aborts and reboots *without printing the
file name and line number* that triggered the assertion. The abort still occurs
(the watchdog catches it), but the serial output will show only the panic
handler backtrace, not the assertion source location. Use `addr2line` on the
backtrace addresses against the `.elf` file to locate the source.

**`CONFIG_ESP_ERR_TO_NAME_LOOKUP=n`** — `esp_err_to_name(err)` returns
`"UNKNOWN"` for all error codes instead of human-readable strings like
`"ESP_ERR_NO_MEM"`. Framework log messages that include `esp_err_to_name()`
will show the numeric error code but not the symbolic name. Consult the ESP-IDF
`esp_err.h` header to decode numeric values.

#### 14.6.5 Size Optimization Summary

The cumulative effect of all optimizations (PR-10):

| Optimization | Savings | Section |
|---|---|---|
| Compiler `-Os` + newlib nano + silent assertions | ~174KB | §14.6.1, §14.6.3 |
| mbedTLS cipher pruning (3DES, Blowfish, Camellia, etc.) | ~24KB | §14.6.6 |
| ECC curve pruning, WPS disable, ChaCha20 disable | ~17KB | §14.6.6 |
| sdkconfig cleanup (console, PPP, libsodium) | ~11KB | §14.6.6 |
| **Total** | **~226KB (−21.4%)** | |

Final binary: **830KB (63.4%)** on esp32dev, leaving **469KB (36.6%)** headroom
for application code and optional modules.

#### 14.6.6 mbedTLS & Network Pruning

The production build disables unused cryptographic primitives to reduce the
mbedTLS contribution to the binary. Only the cipher suites required for
HTTPS (GitHub API, OTA downloads) and OTA signature verification are retained.

**Disabled ciphers:** 3DES, Blowfish, XTEA, Camellia, RC4, ChaCha20-Poly1305,
RIPEMD-160.

**Disabled key exchange modes:** DHE-RSA, DHE-PSK, RSA-PSK, PSK. The framework
uses ECDHE-RSA and ECDHE-ECDSA for TLS, which are retained.

**Disabled ECC curves:** All except secp256r1 (P-256), which is the standard
TLS curve used by GitHub's servers and most HTTPS endpoints.

**Network:** IPv6 is disabled (`CONFIG_LWIP_IPV6=n`). The framework operates
exclusively on IPv4 local networks. WPS (WiFi Protected Setup) is disabled
as the framework uses its own captive portal for WiFi provisioning.

> **TLS compatibility note:** These pruning decisions are based on GitHub's
> current TLS requirements (TLS 1.2+ with AES-GCM or AES-CBC, ECDHE key
> exchange, P-256 curve). If future OTA sources require different cipher
> suites, the corresponding `CONFIG_MBEDTLS_*` options must be re-enabled
> in `sdkconfig.defaults`.

#### 14.6.7 Known Platform Limitation: Arduino Managed Components

The Arduino Core framework (`framework-arduinoespressif32`) declares 27
managed component dependencies in its `idf_component.yml` manifest, of which
the framework uses only 3 (`littlefs`, `mdns`, and the Arduino core itself).
The unused components include Rainmaker (cloud IoT), Zigbee, Modbus, cellular
modem, DSP, speech recognition, MP3 decoding, and libsodium.

**The standard ESP-IDF `EXCLUDE_COMPONENTS` mechanism does not work with
PlatformIO's dual-framework build.** PlatformIO's IDF Component Manager
resolves and compiles managed components before CMake processes the
`EXCLUDE_COMPONENTS` variable. Additionally, PlatformIO auto-generates
`CMakeLists.txt` (gitignored), so persistent modifications are not possible.

The linker's garbage collector (`--gc-sections`) strips most unused code from
these components, but static initialisers, VFS registration macros, and
`.rodata` string tables may survive. This is an estimated 20–60KB of residual
dead weight that cannot currently be eliminated without switching away from
PlatformIO or using ESP-IDF as the sole framework.

**Mitigation:** Monitor PlatformIO releases for `EXCLUDE_COMPONENTS` support
in dual-framework builds. If space becomes critical, evaluate migrating to
pure ESP-IDF with Arduino as a manually managed component (bypassing the
Component Manager entirely).

---

## 15. Module Authoring Guide

### 15.1 Creating a New Module

A minimal module that participates in the framework tick loop:

**`myModule.hpp`**
```cpp
#pragma once
#include <AppFoundation.h>  // Canonical single-include façade

class MyModule : public ITickable, public iConfigurable, public IWebContributor {
public:
    void begin(Application* ctx);
    void tick(uint32_t ms) override;
    void reapplyConfig(const FrameworkConfig& cfg) override;
    void registerRoutes(WebServerManager& server) override;
    const char* getTabName() const override { return "My Module"; }
    const char* getTabEndpoint() const override { return "/api/mymodule"; }
private:
    Application* _ctx = nullptr;
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
    JsonDocument doc;   // ArduinoJson v7 unified type
    deserializeJson(doc, f);
    strlcpy(_config.host, doc["host"] | "localhost", sizeof(_config.host));
    _config.port = doc["port"] | 1883;
    f.close();
}
```

### 15.3 Module Checklist

- [ ] Implements required interfaces (at minimum `ITickable`)
- [ ] Uses `app.getStatusReporter()` for user-visible status
- [ ] Publishes to EventBus topics for cross-module messages (e.g., `"display.message"`)
- [ ] Config file is stored at `/mod/<modulename>.json` — nowhere else
- [ ] Config file includes a `configVersion` field; module validates and migrates on load
- [ ] Config is parsed with `JsonDocument` (ArduinoJson v7) — never v6 `StaticJsonDocument` or `DynamicJsonDocument`
- [ ] All logging uses `#include "platform/LogShim.h"` + `ESP_LOGx(TAG, ...)` — never `Serial.print()` or custom macros
- [ ] Module instance declared `static` — never stack-allocated
- [ ] Never `#include`s another optional module header
- [ ] `tick()` completes in ≤ 10ms — long I/O is deferred to a FreeRTOS task
- [ ] Registers web routes via `IWebContributor` if it has a UI
- [ ] Web assets (if any) stored under `www/<appname>/` in LittleFS
- [ ] Implements `reapplyConfig()` to respond to config changes
- [ ] Has no compile-time dependency on display hardware
- [ ] All hardware-specific calls wrapped in `#ifndef NATIVE_BUILD`
- [ ] Module compiles on `native` environment for GoogleTest
- [ ] Interface headers forward-declare external types — never `#include` heavy libs
- [ ] `currentMillis` parameter injection preferred over calling `millis()` directly
- [ ] `library.json` has NO `"platforms"` or `"frameworks"` restrictions (unless platform-specific)

---

## 16. Testing Architecture

### 16.1 Three Testing Tiers

| Tier | Runs Where | Purpose |
|---|---|---|
| **Tier 1 — Native Unit Tests** | Host CPU via `[env:native]` + GoogleTest | Logic, state machines, registration, scheduling |
| **Tier 2 — Hardware Integration** | Physical ESP32 board | WiFi, OTA, LittleFS, crypto, display |
| **Tier 3 — End-to-End Boot** | Physical board, full stack | Boot-to-`RUNNING` all services |

### 16.2 `NATIVE_BUILD` Guard Pattern

All modules must compile on native (host CPU) for Tier 1 testing. The `NATIVE_BUILD`
preprocessor define (set by `[env:native]`) controls hardware-specific code:

#### Architectural Rationale: The Cost vs Benefit of Native Builds

While littering the codebase with `#ifndef NATIVE_BUILD` guards may feel like a heavy aesthetic tax, it is the most important architectural discipline in the framework:

1. **Iteration Speed:** Hardware-only testing (compile → flash via USB → boot) takes 15–45 seconds per cycle. Native GoogleTest execution runs in milliseconds, fundamentally changing how developers write and refactor logic.
2. **CI/CD Feasibility:** Native builds allow the full logic suite to run on standard cloud runners (e.g., GitHub Actions) without complex Hardware-in-the-Loop server farms.
3. **Forced Decoupling:** The host CPU lacks `<WiFi.h>` and `<LittleFS.h>`. This missing dependency actively prevents developers from tightly coupling high-level logic to low-level hardware drivers. It forces the use of forward declarations (e.g., `class U8G2;`) and dependency injection (e.g., passing `currentMillis` instead of calling `millis()`), ensuring business logic remains pure and testable.

```cpp
#ifndef NATIVE_BUILD
#include <WiFi.h>
#include <LittleFS.h>
// ... hardware-specific implementation
#else
// ... stubs or no-ops for native testing
#endif
```

**Guard strategies:**
- **Individual call wrapping**: Hardware calls wrapped inline — same source compiles
  on both targets
- **Split guards**: Enums/structs outside guard (always available), function bodies
  inside guard
- **Full-file guards**: Entire `.cpp` file wrapped when it's purely hardware rendering

**Testing helpers exposed under `NATIVE_BUILD`:**
- `DataManager::processNext()` — drive scheduling synchronously
- `DataManager::setSimulatedTimeMs()` — manual time progression
- `McpServer::_processLine()` — direct JSON-RPC dispatch without Serial

### 16.3 Mock Strategy

- `MockDeviceCrypto` subclass for `encrypt()`/`decrypt()` virtual methods
- `AsyncWebServer` dummy class in `webServer.hpp` (canonical mock — never define
  inline duplicates)
- `AsyncWebServerResponse` stub with `beginResponse_P`, `addHeader`, `send`
- `DNSServer` dummy class in `wifiManager.hpp` (resolves incomplete type for
  `std::unique_ptr<DNSServer>`)
- `Logger` uses `PIO_UNIT_TESTING` flag to stub ESP log macros
- Serial mocked with string-buffer class for MCP server tests

> **Critical:** Always use project-wide canonical mocks. Defining inline duplicate
> mocks causes ODR (One Definition Rule) violations that break downstream tests.

### 16.4 Build Cache

`pio run -t clean -e native` MUST be run whenever core test entry points or
preprocessor guards change fundamentally. Stale cached objects from different
architectures (x86_64 vs arm64) cause linker errors.

---

## 17. Open Questions & Decision Log

This section tracks decisions made and questions still to resolve.

### Resolved Decisions

| # | Decision | Rationale |
|---|---|---|
| D1 | No unified `IDisplayDriver` interface | U8G2 and TFT API models are incompatible; display module variants are the correct embedded pattern |
| D2 | Per-module LittleFS config files, not a monolithic struct | Enables true module independence; avoids schema coupling |
| D3 | Two-tier `std::vector` model, no `MAX_*` ceilings | Boot-phase allocation is functionally static. Avoids over-allocating and removes artificial ceilings. Structural arrays use `shrink_to_fit()` after each build phase. |
| D4 | Framework owns `setup()` and `loop()` | Defines this as an application framework (Hollywood Principle) |
| D5 | Monorepo — kernel + all optional modules in one repo | No premature repo fragmentation; avoids version compatibility matrix in v1. Modules extracted to submodules only when mature |
| D6 | Structural array mutations require soft-reset; lightweight arrays may grow freely | Keeps `tick()` hot path completely free of vector mutations. Structural changes are rare; the soft-reset cost is acceptable. |
| D7 | Heap stats logged + published on `'system.heap'` EventBus after each build phase | Replaces artificial MAX_* ceilings with observable runtime evidence |
| D8 | Linux-inspired LittleFS directory layout: `etc/`, `mod/`, `app/`, `www/` | Directory-per-tier makes ownership self-evident, enables selective wipe, prevents naming collisions |
| D9 | Hybrid web portal asset model: PROGMEM shell + LittleFS `www/` app fragments | Framework shell in PROGMEM: serves at ~0ms, always present with firmware. App fragments in LittleFS: updated independently |
| D10 | Apps consume framework via git submodule, not `lib_deps` registry | Deterministic SHA pinning; no absolute path hacks; CI reproducible |
| D11 | Template as `template/` within monorepo, not separate repo | One repo to clone; no separate template overhead |
| D12 | EventBus uses static fn-ptr + `void* ctx`, not `std::function` | Prevents heap fragmentation on ESP32; zero overhead |
| D13 | EventBus is payload-type-agnostic (`const void*`), not JSON-typed | Maximum flexibility; JSON typing is subscriber's responsibility |
| D14 | DataManager is a generic scheduler, not HTTP-specific | HTTP fetching is individual `iDataSource`'s concern |
| D15 | `NullStatusReporter` as private nested class in Application | Fully encapsulated; no external/global class |
| D16 | Widget toolkit: three-tier composable architecture | Primitives → Generic → Module-Aware. Runtime display dimensions, parameter injection |
| D17 | MCP Server as kernel service with `IMcpContributor` extension | Mirrors IWebContributor pattern; Hollywood Principle |
| D18 | MCP transport-agnostic via `processRequest()` | Decouples from Serial; enables HTTP and future WebSocket |
| D19 | `McpServer` implements `IWebContributor` for HTTP route | Follows Hollywood Principle; WebServerManager has no MCP coupling |
| D20 | OTA blessing by Application hub, not OTA manager | App must verify WiFi, LittleFS, core services before blessing |
| D21 | `setMaxBodySize()` unavailable; per-handler body validation | Library method doesn't exist; manual len/total checks |
| D22 | POSIX TZ via `setenv`/`tzset` for timezone | Leverages standard `localtime()` for automatic DST handling |
| D23 | Non-blocking async NTP sync | Never blocks boot; sync checked asynchronously |
| D27 | Standardise on `ESP_LOGx` via `LogShim.h` — no custom macros | ESP-IDF logging provides compile-time stripping, runtime per-module control, thread safety, and pluggable backends |
| D28 | Global secret redaction via `esp_log_set_vprintf()` pipeline | All log output automatically redacted; only matters during debug builds where logging is active |
| D29 | RAM ring buffer for log capture, flash only on crash | Flash endurance (10K-100K cycles) and write latency (20-200ms) make continuous flash logging infeasible; RAM has zero overhead |
| D30 | `ILogSink` interface for pluggable log destinations | Mirrors `IWebContributor`/`IMcpContributor` pattern; enables optional MQTT telemetry without kernel coupling |
| D31 | Heartbeat telemetry every ~15s in `Application::tick()` | Provides continuous runtime observability (heap, uptime, RSSI) with zero external infrastructure |
| D32 | MQTT telemetry as optional module, not kernel | Not all apps need remote logging; kernel stays minimal |
| D33 | ArduinoJson v7 `JsonDocument` replaces v6 `StaticJsonDocument`/`DynamicJsonDocument` | Unified API; v6 types are deprecated |
| D24 | `build_fonts.py`/`generate_layouts.py` as standalone CLI, not PIO hooks | Avoids breaking builds if input files are missing |
| D25 | `U8G2_DISPLAY_TYPE` macro for display controller selection | Overridable via `build_flags`; default SSD1306 128x64 |
| D26 | Dual framework: Arduino + ESP-IDF | OTA rollback, IDF logging, DeviceCrypto require IDF; display/web libs require Arduino |

### Open Questions

| # | Question | Impact |
|---|---|---|
| Q1 | Should `schedulerManager` be kernel or optional module? | Medium — most apps benefit from it; few apps are truly headless |
| Q2 | What is the `webServer` base portal's auth model? | Security — no auth vs. optional HTTP Basic Auth |
| Q3 | Should display modules provide a `PSRAM framebuffer` option for S3 boards? | Performance — large colour displays need framebuffer in PSRAM |
| Q4 | How should multi-board hardware configs (e.g., dual display) be handled? | Scope — possibly out of scope for v1 |

### Known Issues & Technical Debt

| # | Issue | Status |
|---|---|---|
| TD-1 | `u8g2Display` has broken forward declarations on `esp32s3nano` (missing `Application.hpp` include) | Needs fix |
| TD-2 | `test_webServer` pre-existing failure (`setMaxBodySize` assertion) | Known — requires mock update |
| TD-3 | `get_logs` MCP tool is a stub placeholder | Needs implementation |
| TD-4 | `portalBuilder.py` referenced in spec but not fully integrated into CI | Deferred |
| TD-5 | `webServer` portal_assets headers break native builds without `NATIVE_BUILD` guards | Requires guard on all generated headers |

---

*This document is a living specification. It was updated on 2026-05-22 to reflect
all architectural decisions made during PR-01 through PR-08 implementation,
including the unified logging/observability architecture (§11A) and ArduinoJson
v7 migration.*
