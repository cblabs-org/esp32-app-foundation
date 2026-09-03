# ESP32 App Foundation — Design Specification

**Status:** v2.0 (in preparation — sections being rewritten)  
**Version:** 1.1  
**Author:** CB Labs  
**Last Updated:** 2026-09-03

---

> [!WARNING]
> **This specification is being rebuilt as v2.0.** Sections marked ✅ have been rewritten
> against the delivered architecture. Sections marked ⚠️ contain content from v1.1
> (2026-05-22) that does not reflect the current codebase. See the
> [design spec audit report](conversation://8b733dfa-645a-478e-b6cc-e08c50620730)
> for the full findings.

| Section | Status | Notes |
|---|---|---|
| §1 Purpose & Vision | ⚠️ v1.1 | Missing LVGL, AuthManager, PreconditionEngine |
| §2 Design Principles | ⚠️ v1.1 | ConfigManager-as-Gateway not reflected |
| §3 System Architecture | ⚠️ v1.1 | Component diagram severely outdated |
| §4 Framework Kernel | ⚠️ v1.1 | Kernel table missing 7 components |
| §5 Module System | ⚠️ v1.1 | Module catalog severely outdated |
| §6 Extension Points | ⚠️ v1.1 | **Interface signatures will not compile** |
| §7 Display Architecture | ⚠️ v1.1 | Missing LVGL, IDisplayModule |
| §8 Configuration | ⚠️ v1.1 | Describes pre-PR-11 pattern |
| §9 Application Lifecycle | ⚠️ v1.1 | AppState has 3 states not 4 |
| §10 Web Portal | ⚠️ v1.1 | Wrong web server library |
| §11 Inter-Module Communication | ⚠️ v1.1 | EventBus dispatch model wrong |
| §11A Logging & Observability | ✅ v2.0 | Written 2026-09-01 during PR-211 |
| §12 Repository & Build | ⚠️ v1.1 | Directory structure outdated |
| §13 Security | ⚠️ v1.1 | Missing AuthManager, OTA v2 |
| §14 Resource Budget | ⚠️ v1.1 | Partition sizes and locations wrong |
| §15 Module Authoring | ⚠️ v1.1 | Code samples won't compile |
| §16 Testing | ⚠️ v1.1 | Test infrastructure evolved |
| §17 Decision Log | ⚠️ v1.1 | Missing D40+ decisions |

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

> 📖 **Developer Guide:** [Coding Patterns](developer/coding-patterns.md) — Hollywood Principle, Composition over Inheritance, SOLID, Service Module Pattern.

### 2.1 The Hollywood Principle — "Don't Call Us, We'll Call You"

> 📖 See [Coding Patterns § Hollywood Principle](developer/coding-patterns.md) and [Architecture Overview § The Hollywood Principle](developer/architecture-overview.md#2-the-hollywood-principle) for implementation guidance.

The framework owns `setup()` and `loop()`. Application code never writes these
directly. The framework calls application code at defined extension points.
This is what makes it an application *framework* rather than a library collection.

### 2.2 Composition Over Inheritance

> 📖 See [Coding Patterns § SOLID Principles](developer/coding-patterns.md#2-solid-principles-in-the-framework) for interface composition patterns.

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

> 📖 See [Embedded Safety § Memory Rules](developer/embedded-safety.md#1-memory-rules) and [Architecture Overview § Two-Tier Vector Model](developer/architecture-overview.md#two-tier-vector-model--_compactandlock-lifecycle) for implementation details.

The embedded instinct to "prefer static over dynamic" targets *runtime* heap
fragmentation — the real enemy is repeated alloc/free cycles during `tick()`
steady-state. Allocation that happens **once at boot and never again** is
functionally indistinguishable from static allocation in terms of heap health.

The framework uses a **two-tier vector model**:

- **Structural arrays** (`ITickable*`, `IDisplayBoard*`, `IDataSource*`,
  `IMCPContributor*`) — built with `std::vector` during `begin()` or a controlled
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

> 📖 See [Configuration System](developer/config-system.md) for the config file ownership model and directory conventions.

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
    └── <appname>/         ← App tab fragments (served by PsychicHttpServer)
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

> 📖 See [Build System](developer/build-system.md#native-environment) and [Creating a Module § Native Build Compilation Guards](developer/creating-a-module.md#native_build-compilation-guards) for guard patterns.

All framework code must compile on both ESP32 hardware targets and host CPU
(x86_64/arm64) for native GoogleTest execution. This is a non-negotiable
cross-cutting concern that affects every module.

- Hardware-specific APIs (`esp_restart()`, `LittleFS`, `WiFi.h`, free heap queries)
  are wrapped in `#ifdef ESP_PLATFORM` preprocessor guards (or platform stubs)
- Interface headers use forward declarations for external library types
  (e.g., `class U8G2;`, `class PsychicHttpServer;`) — never `#include` heavy
  hardware-dependent headers
- Widget and board `render()` methods are no-ops on native; logic-only paths
  (tick, state machines, scheduling) are fully testable
- The `millis()` function is avoided in headers; `currentMillis` parameter
  injection is the preferred pattern for testability

> **Critical LDF constraint:** PlatformIO's Library Dependency Finder (LDF)
> scans `#include` directives as **text patterns** regardless of `#ifdef`
> preprocessor guards. A conditional `#include "Application.hpp"` inside
> `#ifdef ESP_PLATFORM` will still trigger LDF to resolve and compile the
> entire dependency chain. Always forward-declare in headers.

### 2.9 No Monolithic Services

> 📖 See [Coding Patterns § Service Module Pattern](developer/coding-patterns.md#4-service-module-pattern-smp) for the correct inversion.

Each kernel concern lives in a distinct, self-contained service module. Bundling
multiple concerns into a single "systemManager" is explicitly prohibited.

---

## 3. System Architecture Overview

> 📖 **Developer Guide:** [Architecture Overview](developer/architecture-overview.md) — tutorials on the 3-tier model, boot sequence, and interface contracts.

### 3.1 Three-Tier Model

> 📖 See [Architecture Overview § The 3-Tier Model](developer/architecture-overview.md#1-the-3-tier-model) for a tutorial walkthrough.

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
│  U8G2Display Module   │  LVGLDisplay Module             │
│  ButtonModule         │  EncoderModule                  │
│  HomeAssistant Module │  AuditModule (WITH_AUDIT)       │
│  (app-specific & future modules...)                     │
└───────────────────────┬─────────────────────────────────┘
                        │  uses framework interfaces
┌───────────────────────▼─────────────────────────────────┐
│                  FRAMEWORK KERNEL                        │
│  Application (DI hub) │  ConfigManager                  │
│  KeyManager           │  DataManager + IDataSource      │
│  WifiManager          │  OTAUpdateManager               │
│  WebServer            │  TimeManager                    │
│  DeviceCrypto         │  EventBus                       │
│  MCPServer            │  PreconditionEngine             │
│  AuthManager          │  DisplayServiceHelper           │
│  LogPipeline          │  Redactor (ISanitizer)          │
│  MD5Utils             │  OTACerts / DeepStackExecutor   │
└─────────────────────────────────────────────────────────┘
│            PlatformIO / Arduino + ESP-IDF               │
└─────────────────────────────────────────────────────────┘
```

All tiers live in a **single monorepo** (`esp32-app-foundation`). There is no
separate `esp32-modules` repo — optional modules live alongside kernel modules
under `modules/`. Apps consume the framework via git submodule.

### 3.2 `Application` as the System Hub

> 📖 See [Architecture Overview](developer/architecture-overview.md) for DI hub wiring and service accessors.

`Application` is the single composition root and the central name for the framework's
DI hub. It:
- Owns all kernel and domain services as member variables (deterministic lifetime, avoids heap allocation):
  - Infrastructure Services:
    - `ConfigManager _configManager` (centralised LittleFS JSON config gateway)
    - `DataManager _dataManager` (background `IDataSource` worker task)
    - `EventBus _eventBus` (synchronous pub/sub event broker)
    - `PreconditionEngine _preconditionEngine` (readiness evaluation, damping, circuit breaking)
    - `WebServerManager _webServerManager` (PsychicHttpServer portal and REST API)
  - Core Domain Services:
    - `WifiManager _wifiManager` (WiFi STA state machine and captive portal AP)
    - `WifiPreconditionContributor _wifiPrecondition` (built-in WiFi readiness contributor)
    - `KeyManager _keyManager` (encrypted API key and credential store)
    - `AuthManager _authManager` (portal authentication and MCP access control)
    - `DeviceCrypto _deviceCrypto` (hardware-backed AES-256 encryption)
  - Display and Status Adapters:
    - `NullStatusReporter _nullReporter` (fallback status reporter)
    - `IStatusReporter* _statusReporter` (display adapter for boot status)
    - `IBootReporter* _bootReporter` (display adapter for early hardware init)
- Exposes kernel services via reference getter methods (DI injection points: `getConfigManager()`,
  `getDataManager()`, `getEventBus()`, `getPreconditionEngine()`, `getWebServerManager()`,
  `getWifiManager()`, `getKeyManager()`, `getAuthManager()`, etc.)
- Maintains registration lists for tickables, config consumers, data sources,
  web contributors, and MCP contributors via the two-tier vector model
- Owns and drives the bidirectional `AppState` state machine (`Boot` → `Configuring` ↔ `Running`)
- Dispatches `begin()` and `tick()` to all registered components in registration order, monitoring
  execution duration against each module's declared tick budget
- Provides `softReset()` for structural changes at runtime

`Application` is instantiated as a **file-scope static** in `src/main.cpp` — never
stack-allocated, never a singleton via `getInstance()`. Application code receives
an `Application*` pointer (or references the static instance) at setup and uses it to access services — it never
constructs kernel services directly.

### 3.3 Entry Point Contract

> 📖 See [Creating a Module § Module Registration](developer/creating-a-module.md#4-module-registration) for the setup pattern.

The framework owns `setup()` and `loop()`. The `src/main.cpp` contains only:

```cpp
#include <AppFoundation.h>
#include "myApp.hpp"

static MyApp app;

void setup() { app.begin(); }
void loop()  { app.tick();  }
```

`AppFoundation.h` is the **canonical single-include façade** — it transitively includes:
- Version metadata (`build_version.h`)
- Framework lifecycle and configuration types (`AppState.hpp`, `FrameworkConfig.hpp`, `FetchContext.hpp`)
- All 14 framework interfaces:
  `ITickable`, `IConfigurable`, `IDataSource`, `IStatusReporter`, `ITabRegistry`,
  `IWebContributor`, `IMCPContributor`, `IPrecondition`, `IPreconditionRegistry`,
  `IPreconditionContributor`, `IOTAVerifier`, `ISanitizer`, `IBootReporter`, `IDisplayModule`
- EventBus payloads: `DataSourceHealthPayload`, `PreconditionHealthPayload`, `AppConfiguringPayload`
- Concrete Application hub headers (`Application.hpp`, `TimeManager.hpp`) under `#ifdef ESP_PLATFORM` guards
  (target builds only; host-native test builds omit platform-specific headers and compile against stubs)

Downstream consumers should only include `AppFoundation.h`.
`MyApp` instantiates its modules and registers them with the `Application` hub
inside `begin()`.

---

## 4. Framework Kernel

> 📖 **Developer Guide:** [Architecture Overview](developer/architecture-overview.md) — kernel components, lifecycle ownership, and core service wiring.

The kernel is always present. No app can opt out of kernel services.

### 4.1 Kernel Components

| Component | Purpose | Source |
|---|---|---|
| `Application` | DI hub, orchestrator, AppState lifecycle owner, two-tier module registry | `modules/Application/` |
| `ConfigManager` | LittleFS JSON config persistence gateway with schema versioning & migrations (`/mod/<ns>.json`) | `modules/ConfigManager/` |
| `KeyManager` | Encrypted API key and credential registry (uses DeviceCrypto) | `modules/KeyManager/` |
| `AuthManager` | Web portal session authentication, MCP authorization, and credential management | `modules/AuthManager/` |
| `DataManager` | Background `IDataSource` worker task (Core 0), health tracking, backoff & quarantine | `modules/DataManager/` |
| `WifiManager` | WiFi station state machine + captive portal AP mode fallback | `modules/WifiManager/` |
| `WebServer` | PsychicHttpServer wrapper, base portal routes, `IWebContributor` route & tab dispatch | `modules/WebServer/` |
| `MCPServer` | JSON-RPC MCP server (Serial + HTTP SSE/POST) with `IMCPContributor` tool registry | `modules/MCPServer/` |
| `OTAUpdateManager` | Manifest-first OTA firmware updates with dual-partition rollback and cryptographic verification | `modules/OTAUpdateManager/` |
| `TimeManager` | Non-blocking NTP synchronization, POSIX timezone management | `modules/TimeManager/` |
| `EventBus` | Synchronous publish/subscribe event broker with type-safe payloads | `modules/EventBus/` |
| `PreconditionEngine` | Precondition evaluation, damping, circuit breaker, and state machine gating | `modules/PreconditionEngine/` |
| `AuditModule` | Tamper-evident structured audit logging with flash wear-levelled ring buffer (`WITH_AUDIT`) | `modules/AuditModule/` |
| `DeviceCrypto` | Hardware-backed key derivation & AES-256 encryption via ESP32 crypto hardware | `lib/DeviceCrypto/` |
| `DisplayServiceHelper` | Shared REST endpoints and MCP tools for any `IDisplayModule` implementation | `lib/DisplayServiceHelper/` |
| `DeepStackExecutor` | Task stack overflow protection helper for deep recursion/call stacks | `lib/DeepStackExecutor/` |
| `OTACerts` | Built-in root CA certificate bundles for OTA HTTPS downloads | `lib/OTACerts/` |
| `PngWriter` | High-performance framebuffer-to-PNG stream encoder | `lib/PngWriter/` |
| `LogPipeline` | Global log interception pipeline (`vprintf` hook + secret redaction) | `lib/LogPipeline/` |
| `Redactor` | Singleton secret registry with zero-allocation in-place redaction (implements `ISanitizer`) | `lib/Redactor/` |
| `LogShim.h` | Unified logging shim — bridges ESP-IDF `esp_log.h` to native `printf` stubs | `include/platform/` |
| `MD5Utils` | Streaming MD5 hashing via MbedTLS (incremental, not full-file) | `lib/MD5Utils/` |

### 4.2 Kernel Configuration (`FrameworkConfig`)

The kernel's boot-phase configuration struct (`FrameworkConfig`, defined in `include/types/FrameworkConfig.hpp`) covers cross-cutting startup parameters and OTA credentials:

```cpp
struct FrameworkConfig {
    const char* otaPublicKey                = nullptr;  ///< PEM-encoded RSA public key for OTA signature verification (nullptr = skip)
    const char* otaManifestUrl              = nullptr;  ///< Compiled-in manifest URL — unkillable fallback for manifest-first OTA
    const char* otaToken                    = nullptr;  ///< Optional bearer token for authenticated manifest/asset downloads
    uint16_t    configuringPublishIntervalMs = 0;        ///< Gate re-publish throttle in CONFIGURING phase (0 = default 500ms)
};
```

`FrameworkConfig` is passed to `Application::begin(config)` and carries only compile-time OTA secrets and startup throttling. All runtime/module configurations that previously lived in monolithic settings (`hostname`, `timezone`, `mcpReadOnly`, `firmwareUpdatesEnabled`) are now factory defaults owned by their respective modules' `resetConfig()` and persisted in their own files under `/mod/<namespace>.json` via the `IConfigurable` interface. For example, the OTA enable/disable toggle is persisted in `/mod/ota.json` as `updatesEnabled` by `OTAUpdateManager`.

> **Required downstream build flag:** Apps must define `APP_FOUNDATION_VERSION`
> in their `platformio.ini` `build_flags` (e.g., `-DAPP_FOUNDATION_VERSION=\"0.1.0\"`).
> The Application hub uses this for the boot banner.

---

## 5. Module System

> 📖 **Developer Guide:** [Creating a Module](developer/creating-a-module.md) — step-by-step module implementation tutorial.

### 5.1 What Is a Module

> 📖 See [Creating a Module](developer/creating-a-module.md) for the step-by-step implementation tutorial.

A module is a cohesive unit of functionality that:
1. Implements one or more framework interfaces (`ITickable`, `IConfigurable`,
   `IDataSource`, `IStatusReporter`, `IBootReporter`, `IDisplayModule`,
   `IWebContributor`, `IMCPContributor`, `IPreconditionContributor`)
2. Self-registers with `Application` during app setup (preferring `app.registerModule(&module)`)
3. Manages its own configuration persistence via `ConfigManager` under `/mod/<namespace>.json` (never touching LittleFS directly)
4. Registers its own web portal routes and navigation tabs (via `IWebContributor` and `ITabRegistry`)
5. **Never directly `#include`s another optional module** (decoupling via interfaces and `EventBus`)

### 5.2 Module Catalog

All modules live under `modules/` in the monorepo alongside kernel modules.

| Module | Interfaces | Config File | Notes |
|---|---|---|---|
| `Application` | DI hub / Kernel | `/etc/config.json` | Core composition root and lifecycle coordinator |
| `AuditModule` | `ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor`, `IPreconditionContributor` | ConfigManager (`audit`) | Tamper-evident audit trail with flash wear-levelled ring buffer (`WITH_AUDIT`) |
| `AuthManager` | `IConfigurable`, `IWebContributor`, `IMCPContributor` | ConfigManager (`auth`) | Portal passwords, JWT sessions, and MCP tool access control |
| `ButtonModule` | `ITickable` | — | Physical pushbutton debouncing and EventBus event dispatch (replaces legacy `buttonHandler`) |
| `ConfigManager` | `IWebContributor` | Gateway owner | Centralised LittleFS JSON configuration persistence and schema migrations |
| `DataManager` | `ITickable`, `IWebContributor`, `IMCPContributor` | — | Background `IDataSource` worker task (Core 0), health monitoring, and quarantine |
| `EncoderModule` | `ITickable` | — | Rotary encoder pulse tracking and click events |
| `EventBus` | Kernel service | — | Synchronous pub/sub event broker |
| `HomeAssistant` | `ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor`, `IPreconditionContributor` | ConfigManager (`ha`) | MQTT auto-discovery and entity state management (`IHaEntity`) |
| `KeyManager` | `IWebContributor`, `IMCPContributor` | `/etc/apikeys.json` | Encrypted credential and API key storage (uses DeviceCrypto) |
| `LVGLDisplay` | `ITickable`, `IConfigurable`, `IDisplayModule` (`IBootReporter`, `IStatusReporter`), `IWebContributor`, `IMCPContributor`, `IPreconditionContributor` | ConfigManager (`display`) | Full-colour TFT display via LVGL v9.5 + `ILVGLHardwareAdapter` |
| `MCPServer` | `ITickable`, `IWebContributor` | ConfigManager (`mcp`) | Model Context Protocol server exposing registered tools to AI agents |
| `OTAUpdateManager` | `ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor`, `IPreconditionContributor` | ConfigManager (`ota`) | Manifest-first OTA updates with dual-partition rollback and cryptographic verification |
| `PreconditionEngine` | `ITickable`, `IWebContributor`, `IMCPContributor`, `IPreconditionRegistry` | — | Precondition evaluation, damping, circuit breaker, and state machine gating |
| `TimeManager` | `ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor`, `IPreconditionContributor` | ConfigManager (`time`) | Non-blocking NTP synchronization and POSIX timezone management |
| `U8G2Display` | `ITickable`, `IConfigurable`, `IDisplayModule` (`IBootReporter`, `IStatusReporter`), `IWebContributor`, `IMCPContributor`, `IPreconditionContributor` | ConfigManager (`display`) | Monochrome OLED/LCD display module with carousel engine (`IDisplayBoard`) |
| `WebServer` | `ITickable` | ConfigManager (`web`) | PsychicHttpServer-backed portal, REST API dispatch, tab registry |
| `WifiManager` | `ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor` | ConfigManager (`wifi`) | WiFi STA connection state machine and captive portal AP |

### 5.3 Adding a Module to an App

> 📖 See [Creating a Module § Module Registration](developer/creating-a-module.md#4-module-registration) for the registration pattern.

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
    https://github.com/hoeken/PsychicHttp.git#v1.2.1
extra_scripts =
    pre:framework/scripts/build_timestamp.py
```

> **Warning:** `build_src_filter` does NOT work with ESP-IDF (it uses `CMakeLists.txt`
> for component discovery, not PlatformIO's source filter). Use `lib_extra_dirs` +
> explicit `lib_deps` instead.

In app setup:

The framework provides a unified registration template `app.registerModule(&module)` (added in PR-25)
that uses compile-time type introspection (`if constexpr` and `std::is_base_of_v`) to auto-detect
all interfaces implemented by the concrete type and wire them into their respective subsystems:

```cpp
#include <AppFoundation.h>
#include <U8G2DisplayModule.hpp>
#include <ButtonModule.hpp>

static U8G2DisplayModule display;
static ButtonModule button(0); // GPIO 0

void MyApp::begin() {
    // Primary pattern: unified auto-detecting registration
    app.registerModule(&display);
    app.registerModule(&button);
}
```

The unified `registerModule()` method automatically checks and registers:
- `ITickable` → `registerTickable(module)`
- `IConfigurable` → `registerConfigurable(module)`
- `IWebContributor` → `registerWebContributor(module)`
- `IMCPContributor` → `registerMcpContributor(module)`
- `IDataSource` → `registerDataSource(module)`
- `IStatusReporter` → `registerStatusReporter(module)`
- `IBootReporter` → `registerBootReporter(module)`
- `IPreconditionContributor` → `module->registerPreconditions(_preconditionEngine)`

Granular per-interface registration methods remain available as secondary options for
scenarios requiring explicit control or partial registration:

```cpp
// Secondary pattern: granular per-interface registration
app.registerTickable(&display);
app.registerConfigurable(&display);
app.registerStatusReporter(&display);
app.registerBootReporter(&display);
app.registerWebContributor(&display);
app.registerMcpContributor(&display);
```

---

## 6. Extension Points & Interfaces

> 📖 **Developer Guide:** [Creating a Module](developer/creating-a-module.md) — interface catalog and implementation examples.

All framework interfaces live under `include/interfaces/` and represent the formal contracts
governing the system's extension points.

### 6.1 Interface Naming Convention

All framework interfaces strictly adhere to **PascalCase with an `I` prefix**:
- `ITickable`, `IConfigurable`, `IDataSource`, `IStatusReporter`
- `IBootReporter`, `IDisplayModule`, `IWebContributor`, `ITabRegistry`
- `IMCPContributor`, `IPreconditionContributor`, `IPreconditionRegistry`
- `IPrecondition`, `ISanitizer`, `IOTAVerifier`

Interface names are consistently uppercase-`I` prefixed across all kernel subsystems, modules, and extension points.

All kernel interfaces are abstract base classes with pure virtual methods (or safe default implementations where documented) and a virtual default destructor (`virtual ~IFoo() = default;`). Interface headers forward-declare external types (such as `class PsychicHttpServer;` or `class U8G2;`) to maintain strict separation and enable native host compilation without platform-specific dependencies.

### 6.2 `ITickable`

> 📖 See [Creating a Module § The Tick Loop](developer/creating-a-module.md#3-implementing-the-module-logic) and [Tickable Interface Doc](../.agents/skills/framework-expert/interfaces/tickable.md).

Defined in `include/interfaces/ITickable.hpp`. Contract for components participating in the cooperative main-thread update loop.

```cpp
class ITickable {
public:
    virtual ~ITickable() = default;

    /**
     * @brief Periodic update hook called by the Application hub on every loop iteration.
     * @param currentMillis Milliseconds since boot, captured once per Application::tick() loop.
     */
    virtual void tick(uint32_t currentMillis) = 0;

    /**
     * @brief Per-module tick budget in milliseconds (ADR-17).
     * @return Maximum expected tick duration in milliseconds (default: 10ms).
     */
    virtual uint32_t getTickBudgetMs() const { return 10; }
};
```

Registered via `app.registerModule(&module)` or `app.registerTickable(ITickable*)`.

**Performance Contract (ADR-17):**
`tick()` executes synchronously on Core 1 in the main loop. Implementations MUST complete within their declared budget (default: 10ms). Long I/O, network operations, or blocking sleeps MUST be deferred to dedicated FreeRTOS tasks (such as `DataManager` for background fetches). The `Application` hub monitors tick durations and emits warnings when a module exceeds its declared budget. Display modules or components with inherently heavier draw loops may override `getTickBudgetMs()` to declare higher limits.

### 6.3 `IConfigurable`

> 📖 See [Configuration System § The IConfigurable Interface](developer/config-system.md#2-the-iconfigurable-interface) and [Configurable Interface Doc](../.agents/skills/framework-expert/interfaces/configurable.md).

Defined in `include/interfaces/IConfigurable.hpp`. Contract for modules participating in the centralised configuration gateway managed by `ConfigManager`.

```cpp
class IConfigurable {
public:
    virtual ~IConfigurable() = default;

    virtual const char* getConfigNamespace() const = 0;
    virtual int getSchemaVersion() const = 0;
    virtual void applyConfig(const JsonObject& section) = 0;
    virtual void serializeConfig(JsonObject& section) const = 0;
    virtual bool migrateConfig(JsonObject& section, int fromVersion) { return true; }
    virtual void resetConfig() = 0;
};
```

Registered via `app.registerModule(&module)` or `app.registerConfigurable(IConfigurable*)`.

**ConfigManager-as-Gateway Pattern:**
Modules **never touch LittleFS directly** for configuration persistence. `ConfigManager` owns all configuration file I/O under `/mod/<namespace>.json`, enforcing transactional writes (write-to-temp then atomic rename) and schema integrity.

- **`getConfigNamespace()`**: Returns a unique compile-time string literal (e.g. `"wifi"`, `"display"`, `"ota"`). Determines file location at `/mod/<namespace>.json`.
- **`getSchemaVersion()`**: Returns the integer schema version (starting at 1). Incremented when configuration key structures change.
- **`applyConfig(const JsonObject& section)`**: Deserializes configuration into internal state during boot (`beginAll()`) or portal configuration imports. The JSON object is guaranteed to be complete.
- **`serializeConfig(JsonObject& section)`**: Serializes current configuration state into the JSON object. `ConfigManager` injects the metadata field `_v` automatically.
- **`migrateConfig(JsonObject& section, int fromVersion)`**: Invoked when stored `_v` is older than `getSchemaVersion()`. Modules modify the JSON object in-place (renaming keys, adding new defaults). Returning `true` persists the migrated format; `false` falls back to defaults.
- **`resetConfig()`**: Resets all module settings to hardcoded factory defaults. Called before factory resets or when configuration files are missing or corrupt.

### 6.4 `IDataSource`

> 📖 See [Scheduling Data Fetches § The IDataSource Interface](developer/scheduling-data-fetches.md#1-the-idatasource-interface) and [Data Source Interface Doc](../.agents/skills/framework-expert/interfaces/data-source.md).

Defined in `include/interfaces/IDataSource.hpp`. Contract for background data-fetching workers scheduled by `DataManager`.

```cpp
class IDataSource {
public:
    virtual ~IDataSource() = default;

    virtual UpdateStatus fetchUpdate(const FetchContext& ctx) = 0;
    virtual uint32_t getUpdateIntervalMs() const = 0;
    virtual const char* getSourceName() const = 0;
    virtual PriorityTier getPriorityTier() const = 0;
    virtual bool fetchesWhileConfiguring() const { return false; }
    virtual bool requiresTls() const { return true; }
    virtual uint32_t getMaxBackoffMs() const;
};
```

Registered via `app.registerModule(&module)` or `app.registerDataSource(IDataSource*)`.

**Execution Model & FreeRTOS Worker Task:**
Unlike `ITickable`, `fetchUpdate()` runs on a dedicated FreeRTOS worker task (Core 0, 12KB stack) managed by `DataManager`. This isolates blocking network operations (TLS handshakes, HTTP transactions) from the cooperative main thread.

**Key Types & Contracts:**
- **`UpdateStatus`**: Enum indicating fetch result:
  `Success` (0), `NoChange` (1, HTTP 304), `RateLimited` (2, HTTP 429), `FetchError` (3, network/TLS/timeout), `ParseError` (4, malformed payload), `AuthError` (5, 401/403 or missing credentials), `Skipped` (6, source declined to fetch due to missing prerequisites).
- **`PriorityTier`**: Scheduling cadence:
  `High` (0, ~60s), `Normal` (1, ~300s / 5 min), `Low` (2, ~1800s / 30 min).
- **`FetchContext` & `DeadlineToken`** (`include/types/FetchContext.hpp`):
  Provides cooperative cancellation and execution budget:
  - `ctx.deadline.isExpired()`: Checked periodically during socket reads and JSON parsing. Total budget is `FETCH_TOTAL_TIMEOUT_MS` (15s), with `CONNECT_TIMEOUT_MS` (5s) for TCP/TLS connection.
  - `ctx.client`: Framework-provided `WiFiClient*`. When `requiresTls() == true` (default), provides a `WiFiClientSecure` pre-attached to the system certificate bundle (`esp_crt_bundle`). When `false`, provides a plain unencrypted `WiFiClient`. Consumers never instantiate raw clients or call `setInsecure()`.
- **`SourceHealth` System**:
  Tracks health transitions across 5 states:
  - `Unknown` (0): Registered but unattempted.
  - `Healthy` (1): Last fetch succeeded.
  - `Degraded` (2): Failing and backing off (exponential backoff doubling from 30s up to `getMaxBackoffMs()`).
  - `Quarantined` (3): Backoff ceiling reached after 6 consecutive failures. Continues probing at ceiling interval for automatic self-healing.
  - `Suspended` (4): Hard failure (`AuthError`). Automatic polling ceases until credentials change or manual reset.
- **`fetchesWhileConfiguring()`**: Returns `true` to allow polling while the framework is in `AppState::Configuring`. Required for data sources that back startup preconditions.

### 6.5 `IStatusReporter`

> 📖 See [Creating a Module § Choosing Your Interfaces](developer/creating-a-module.md#1-choosing-your-interfaces) and [Status Reporter Interface Doc](../.agents/skills/framework-expert/interfaces/status-reporter.md).

Defined in `include/interfaces/IStatusReporter.hpp`. Contract for display modules providing boot-phase status overlays without coupling the kernel to concrete graphics libraries.

```cpp
class IStatusReporter {
public:
    virtual ~IStatusReporter() = default;

    virtual void reportStatus(const char* message, float progress = -1.0f) = 0;
    virtual void reportError(const char* errorMessage) = 0;
    virtual void clearStatus() = 0;
};
```

Registered via `app.registerModule(&module)` or `app.registerStatusReporter(IStatusReporter*)`.

- **`reportStatus(message, progress)`**: Renders a one-line status string. `progress` is in `[0.0, 1.0]`; negative values indicate an indeterminate spinner or progress bar.
- **`reportError(errorMessage)`**: Displays a fatal error screen and halt indicator (e.g. storage mount failure).
- **`clearStatus()`**: Dismisses active status overlays, transitioning to normal rendering.
- **`NullStatusReporter`**: A private nested class in `Application` that acts as a silent no-op fallback when running headless or in native tests.

### 6.6 `IBootReporter`

> 📖 See [IBootReporter Header](include/interfaces/IBootReporter.hpp).

Defined in `include/interfaces/IBootReporter.hpp`. Extends `IStatusReporter` to provide early hardware initialization hooks before full framework module registration.

```cpp
class IBootReporter : public IStatusReporter {
public:
    virtual bool beginEarly() = 0;
    virtual void setAppTitle(const char* title) { (void)title; }
};
```

Registered via `app.registerModule(&module)` or `app.registerBootReporter(IBootReporter*)`.

- **`beginEarly()`**: Called by `Application::begin()` before registration is locked. Initializes display hardware (SPI/I2C buses, framebuffers) and draws an instant boot splash before network and filesystem services initialize. Must be non-blocking and fast.
- **`setAppTitle(title)`**: Updates the displayed boot screen title with the runtime application name.

### 6.7 `IDisplayModule`

> 📖 See [Display Module Interface Doc](../.agents/skills/framework-expert/interfaces/display-module.md).

Defined in `include/interfaces/IDisplayModule.hpp`. Unified hardware abstraction contract for display providers (both `U8G2DisplayModule` and `LVGLDisplayModule`). Extends `IBootReporter` and decouples high-level services like `DisplayServiceHelper` from specific rendering engines.

```cpp
namespace DisplayState {
    constexpr const char* Normal      = "normal";
    constexpr const char* Error       = "error";
    constexpr const char* Configuring = "configuring";
}

struct SystemDisplayParams {
    const char* state   = DisplayState::Normal;
    const char* title   = nullptr;
    const char* message = nullptr;
    float       progress = -1.0f;
};

class IDisplayModule : public IBootReporter {
public:
    virtual ~IDisplayModule() = default;

    // Geometry
    virtual uint16_t getDisplayWidth() const = 0;
    virtual uint16_t getDisplayHeight() const = 0;

    // Brightness
    virtual void setBrightness(uint8_t level) = 0;
    virtual uint8_t getBrightness() const = 0;

    // Screensaver & Activity
    virtual void resetActivity() = 0;
    virtual bool isDimmed() const = 0;
    virtual bool isSleeping() const = 0;

    // Framebuffer Capture (PNG)
    virtual bool capturePNG(Print& sink) = 0;
    virtual bool captureStoredPNG(Print& sink) = 0;

    // System Lifecycle Overlays
    virtual void showSystemState(const SystemDisplayParams& params) = 0;
};
```

This interface unifies display geometry, backlight dimming/sleeping, screensaver timeouts, on-device PNG screenshot streaming, and system-level lifecycle overlays (`Normal`, `Error`, `Configuring`). Shared REST routes (`/api/display/*`) and MCP tools (`display_status`, `display_control`, `capture_display`) delegate directly to `IDisplayModule` via `DisplayServiceHelper`.

### 6.8 `IWebContributor`

> 📖 See [Extending the Web Portal § The IWebContributor Interface](developer/extending-the-web-portal.md#1-the-iwebcontributor-interface) and [Web Contributor Interface Doc](../.agents/skills/framework-expert/interfaces/web-contributor.md).

Defined in `include/interfaces/IWebContributor.hpp`. Contract for modules exposing REST API endpoints and navigation tabs to `WebServerManager`.

```cpp
class IWebContributor {
public:
    virtual ~IWebContributor() = default;

    virtual void registerRoutes(PsychicHttpServer& server) = 0;
    virtual void registerTabs(ITabRegistry& registry) = 0;
    virtual const char* getApiPrefix() const { return nullptr; }
    virtual size_t getMaxPostBody() const { return 4096; }
};
```

Registered via `app.registerModule(&module)` or `app.registerWebContributor(IWebContributor*)`.

- **`registerRoutes(PsychicHttpServer& server)`**: Installs HTTP route handlers on the underlying `PsychicHttpServer`. Global authentication middleware automatically enforces portal auth for `/api/` endpoints.
- **`registerTabs(ITabRegistry& registry)`**: Declares navigation tabs in the web portal via the `ITabRegistry` visitor pattern. Modules with no visible UI provide an empty implementation.
- **`getApiPrefix()`**: Returns the route prefix (e.g. `"/api/mcp"`) used by the web server's body-size middleware to match incoming POST/PUT requests.
- **`getMaxPostBody()`**: Overrides the maximum allowed request body size (default: 4096 bytes, ADR-15).

### 6.9 `ITabRegistry`

> 📖 See [ITabRegistry Header](include/interfaces/ITabRegistry.hpp).

Defined in `include/interfaces/ITabRegistry.hpp`. Abstract visitor registry passed to `IWebContributor::registerTabs()`, eliminating inner stub objects and enabling modules to register multiple tabs or sub-tabs in a single call.

```cpp
struct TabEntry {
    const char* name;    ///< Display name for the tab (static lifetime)
    const char* parent;  ///< Parent tab name for sub-navigation (nullptr = top-level)
    int         order;   ///< Sort order within level (lower = first, default 100)
    const char* icon;    ///< SVG inner content for navigation icon (nullptr = generic)
    const char* script;  ///< Route path for tab JavaScript file (nullptr = none)
};

class ITabRegistry {
public:
    virtual ~ITabRegistry() = default;

    virtual void addTab(const char* name,
                        const char* parent = nullptr,
                        int         order  = 100,
                        const char* icon   = nullptr,
                        const char* script = nullptr) = 0;

    void addTab(const TabEntry& entry);
};
```

String arguments must point to memory with static or application lifetime (compile-time string literals or class member constants), as the registry stores pointers rather than making heap copies.

### 6.10 `IMCPContributor`

> 📖 See [MCP Server § Implementing IMCPContributor](developer/mcp-server.md#3-implementing-imcpcontributor) and [MCP Contributor Interface Doc](../.agents/skills/framework-expert/interfaces/mcp-contributor.md).

Defined in `include/interfaces/IMCPContributor.hpp`. Contract for modules exposing Model Context Protocol (MCP) tools to AI agents via `MCPServer`.

```cpp
class IMCPContributor {
public:
    virtual ~IMCPContributor() = default;

    virtual void registerMCPTools(MCPToolRegistry& registry) = 0;
};
```

Registered via `app.registerModule(&module)` or `app.registerMcpContributor(IMCPContributor*)`.

**Tool Registration Pattern:**
Tools are registered inside `registerMCPTools()` using `registry.addTool()` with stateless lambdas:

```cpp
void MyModule::registerMCPTools(MCPToolRegistry& registry) {
    registry.addTool("my_action", "Executes an action on MyModule",
        [](void* ctx, JsonObject& args, JsonObject& result) {
            auto* self = static_cast<MyModule*>(ctx);
            result["success"] = self->doAction();
        },
        this,
        true // mutatesState = true (blocked in read-only mode with code -32000)
    );
}
```

- **Stateless Lambdas**: Handlers MUST NOT capture variables (`[]`), preventing hidden heap allocations (`std::function` ban, GRAVE-05).
- **`void* context`**: Pointer to the module instance (`this`), cast back via `static_cast`.
- **`mutatesState`**: Set to `true` for any action that mutates configuration, writes flash, or controls hardware. When `MCPServer` is operating in read-only mode, mutating tools are automatically rejected.

### 6.11 `IPreconditionContributor`

> 📖 See [Precondition Contributor Interface Doc](../.agents/skills/framework-expert/interfaces/precondition-contributor.md).

Defined in `include/interfaces/IPreconditionContributor.hpp`. Contract for modules contributing readiness conditions to the system via `PreconditionEngine`.

```cpp
class IPreconditionContributor {
public:
    virtual ~IPreconditionContributor() = default;

    virtual void registerPreconditions(IPreconditionRegistry& registry) = 0;
};
```

Registered via `app.registerModule(&module)` or automatically introspected during `app.registerModule()`. Modules implement `registerPreconditions()` to install evaluation callbacks and dependency edges into the central engine.

### 6.12 `IPreconditionRegistry`

> 📖 See [IPreconditionRegistry Header](include/interfaces/IPreconditionRegistry.hpp).

Defined in `include/interfaces/IPreconditionRegistry.hpp`. Visitor interface passed to `IPreconditionContributor::registerPreconditions()`.

```cpp
class IPreconditionRegistry {
public:
    using EvalFn     = std::function<PreconditionStatus()>;
    using DescribeFn = std::function<void(Cause, char*, size_t)>;

    virtual ~IPreconditionRegistry() = default;

    virtual void add(const char* name,
                     PreconditionRole role,
                     EvalFn evaluate,
                     DescribeFn describe = nullptr,
                     const Edge* edges = nullptr,
                     uint8_t edgeCount = 0,
                     const DampingPolicy* damping = nullptr,
                     const RetryPolicy*  retry   = nullptr) = 0;
};
```

- **`EvalFn evaluate`** (Hot Path): Non-blocking fast-path function called every tick. Checks a cached flag or state variable and returns `PreconditionStatus{met, cause}`.
- **`DescribeFn describe`** (Cold Path): Formats context-sensitive user guidance into a caller-provided buffer based on the current `Cause`. Called on-demand by displays and portal UI.
- **`edges` / `edgeCount`**: Prerequisite dependencies on other preconditions.

### 6.13 `IPrecondition` (Types, Enums, and Policies)

> 📖 See [IPrecondition Header](include/interfaces/IPrecondition.hpp) and [Precondition Contributor Doc](../.agents/skills/framework-expert/interfaces/precondition-contributor.md).

Defined in `include/interfaces/IPrecondition.hpp`. Defines the vocabulary and policies for multi-cause readiness evaluation:

- **`Cause`** (12 codes): `Met`, `NotConfigured`, `Connecting`, `Rejected`, `Unreachable`, `RateLimited`, `Implausible`, `DependencyUnmet`, `FallbackEngaged`, `Unavailable`, `Indeterminate`, `InternalError`.
- **`Severity`** (6 derived levels): `Transient` (self-healing), `NeedsUser` (human intervention required), `Blocked` (upstream dependency unmet), `Degraded` (fallback operational), `Inactive` (disabled by config), `Persistent` (unwired dependency or code defect).
- **`deriveSeverity(Cause cause, bool everMet = false)`**: Maps cause and trust history to operational severity (e.g. `Unreachable` is `Transient` if previously connected, but `NeedsUser` if never connected).
- **`PreconditionRole`**:
  - `Operational`: Monitored at runtime; degrades features but does not block boot.
  - `Startup`: Must be met to transition from `Boot`/`Configuring` to `Running`.
  - `Both`: Required for startup and triggers regression from `Running` to `Configuring` if suppressed.
- **`EdgeKind` & `Edge`**: Dependency graph edges: `Wants` (presentation order), `Requires` (cannot be met without), `BindsTo` (if upstream fails, this condition drops immediately).
- **`DampingPolicy` & `RetryPolicy`**: Damping score prevents flap oscillation by accumulating penalty while unmet and decaying exponentially. Circuit breaker state (`Closed`, `Open`, `HalfOpen`) governs retry backoff.
- **`PreconditionNames`**: Well-known constants (`WIFI_STATION`, `TIMEZONE`, `TIME_SYNC`).

### 6.14 `ISanitizer`

> 📖 See [ISanitizer Header](include/interfaces/ISanitizer.hpp).

Defined in `include/interfaces/ISanitizer.hpp`. Interface for zero-allocation in-place redaction of secrets, decoupling logging and audit systems from concrete redactor implementations.

```cpp
class ISanitizer {
public:
    virtual ~ISanitizer() = default;

    virtual void sanitize(char* buffer, size_t bufferSize, bool isTruncated = false) = 0;
};
```

Implemented by `Redactor` in `lib/Redactor/`. Modules such as `AuditModule` and `LogPipeline` accept an `ISanitizer*` to scrub sensitive strings (passwords, tokens, API keys) in-place without heap allocations.

### 6.15 `IOTAVerifier`

> 📖 See [OTA Verifier Interface Doc](../.agents/skills/framework-expert/interfaces/ota-verifier.md).

Defined in `include/interfaces/IOTAVerifier.hpp`. Cryptographic verification interface for OTA firmware updates, enforcing streaming signature validation before flashing.

```cpp
class IOTAVerifier {
public:
    virtual ~IOTAVerifier() = default;

    virtual bool begin(const char* publicKeyPem) = 0;
    virtual void update(const uint8_t* data, size_t length) = 0;
    virtual bool verify(const uint8_t* signature, size_t signatureLength) = 0;
};
```

**Streaming Verification Contract (ADR-13):**
Because firmware images exceed available RAM, `IOTAVerifier` consumes firmware incrementally:
1. `begin()`: Injects the PEM-encoded public key and initializes the digest context.
2. `update()`: Incrementally accumulates SHA-256 hash over incoming network chunks with $O(1)$ memory usage.
3. `verify()`: Finalizes digest and verifies RSA/ECDSA signature against the detached manifest signature.

### 6.16 Module-Local Extension Points

Per the interface placement heuristic ([ADR Interface Placement](developer/adr/adr-interface-placement.md)), interfaces tightly coupled to a specific module's graphics library or subsystem belong to that module's directory rather than the kernel `include/interfaces/`.

#### 6.16.1 `IDisplayBoard` (U8G2 Display Carousel)

Defined in `modules/U8G2Display/IDisplayBoard.hpp`. Contract for monochrome carousel rendering units managed by `U8G2DisplayModule`'s `CarouselEngine`.

```cpp
class IDisplayBoard {
public:
    virtual ~IDisplayBoard() = default;

    virtual void render(U8G2& u8g2) = 0;
    virtual bool isExpired() const = 0;
    virtual bool canAdvance() const { return true; }
    virtual const char* getBoardName() const = 0;
    virtual void onActivate() {}
    virtual void onDeactivate() {}
    virtual void tick(uint32_t currentMillis) {}
};
```

- **`render(u8g2)`**: Renders a frame using the native `U8G2` drawing API. Must be fast and non-blocking.
- **`isExpired()`**: Pure virtual query. Permanent boards return `false`; transient notification overlays return `true` when finished to evict themselves from rotation.
- **`canAdvance()`**: Bounded pull-based pacing hook. Returns `false` to defer carousel rotation away from this board while a scrolling animation or multi-frame transition is in-flight. The engine bounds this hold (default max 30s) to prevent wedging.
- **Lifecycle hooks**: `onActivate()`, `onDeactivate()`, and per-frame `tick(currentMillis)`.

#### 6.16.2 `ILVGLHardwareAdapter` (LVGL Display Hardware)

Defined in `modules/LVGLDisplay/ILVGLHardwareAdapter.hpp`. Isolates `LVGLDisplayModule` from hardware-specific display controllers, pinouts, and bus drivers.

```cpp
class ILVGLHardwareAdapter {
public:
    virtual ~ILVGLHardwareAdapter() = default;

    // Required Display Methods
    virtual lv_display_t* createDisplay() = 0;
    virtual void setBrightness(uint8_t level) = 0;
    virtual uint16_t getScreenWidth() const = 0;
    virtual uint16_t getScreenHeight() const = 0;
    virtual uint16_t getBufferLineCount() const = 0;

    // Optional Input Methods (Default nullptr)
    virtual lv_indev_t* createTouchInput() { return nullptr; }
    virtual lv_indev_t* createEncoderInput() { return nullptr; }
    virtual lv_indev_t* createKeypadInput() { return nullptr; }

    // Screensaver Touch Polling
    virtual void pollTouch() {}
};
```

Hardware adapters configure the display driver, wire the `flush_cb`, allocate draw buffers, and optionally initialize touch panels (`createTouchInput()`), rotary encoders (`createEncoderInput()`), or keypads (`createKeypadInput()`). `pollTouch()` is called on tick to wake displays when blanked.

#### 6.16.3 `IHaEntity` (Home Assistant MQTT Entities)

Defined in `modules/HomeAssistant/entities/IHaEntity.hpp`. Abstract contract for entity implementations contributing to Home Assistant MQTT auto-discovery and state tracking.

```cpp
class IHaEntity {
public:
    virtual ~IHaEntity() = default;

    virtual std::string getObjectId() const = 0;
    virtual const char* getEntityType() const = 0;
    virtual void buildDiscoveryPayload(JsonObject& payload) const = 0;
    virtual std::string getState() const = 0;
    virtual bool hasState() const { return true; }
    virtual void handleCommand(const std::string& topic, const std::string& payload) {}
    virtual bool isCommandable() const { return false; }
};
```

Entities provide object IDs, entity types (e.g. `"sensor"`, `"switch"`), discovery JSON schema, state serialization, and inbound command dispatch.

### 6.17 EventBus Payloads

Predefined typed structs published on the synchronous `EventBus`:

#### 6.17.1 `DataSourceHealthPayload`

Defined in `include/events/DataSourceHealthPayload.hpp`. Broadcast on `ApplicationEvents::DATA_SOURCE_HEALTH` (`"data.source.health"`) when a data source transitions state:

```cpp
struct DataSourceHealthPayload {
    const char*       name;   ///< Source name (static lifetime)
    SourceHealthState state;  ///< New health state (Healthy, Degraded, Quarantined, Suspended)
};
```

#### 6.17.2 `PreconditionHealthPayload`

Defined in `include/events/PreconditionHealthPayload.hpp`. Broadcast on `ApplicationEvents::PRECONDITION_HEALTH` (`"precondition.health"`) when a precondition's damped cause or severity changes:

```cpp
struct PreconditionHealthPayload {
    const char*      name;                                  ///< Precondition name (static lifetime)
    Cause            cause;                                 ///< Current damped cause
    Severity         severity;                              ///< Derived operational severity
    bool             everMet;                               ///< Trust history
    PreconditionRole role = PreconditionRole::Operational;  ///< Lifecycle role
};
```

#### 6.17.3 `AppConfiguringPayload`

Defined in `include/events/AppConfiguringPayload.hpp`. Broadcast on `ApplicationEvents::APP_CONFIGURING` (`"app.configuring"`) to guide display helper modals:

```cpp
struct AppConfiguringPayload {
    const char* preconditionName = nullptr;                  ///< Name of blocking precondition
    const char* helpText     = nullptr;                      ///< User guidance string
    Cause       cause        = Cause::NotConfigured;         ///< Precondition cause
    Severity    severity     = Severity::NeedsUser;          ///< Derived severity
    bool        isRegression = false;                        ///< True if regressed from RUNNING
    const char* hostname     = nullptr;                      ///< Device hostname (AP SSID)
    const char* deviceIP     = nullptr;                      ///< Device IP address string
};
```

---

## 7. Display Architecture

> 📖 **Developer Guide:** [Using Optional Modules](developer/using-optional-modules.md) — display subsystem, carousel, custom boards, and screensaver.

### 7.1 Dual-Display Architecture & Unified Abstraction (`IDisplayModule`)

The framework provides first-class dual-display support, accommodating both lightweight monochrome panels and high-resolution colour touchscreens:

| Module | Technology | Characteristics | Target Hardware |
|---|---|---|---|
| `U8G2DisplayModule` | U8G2 library | Monochrome OLED/LCD, page-buffer rendering, low RAM (<1 KB) | SSD1306, SH1106, SSD1322, ST7920 (I2C/SPI) |
| `LVGLDisplayModule` | LVGL v9.5 | Full-colour TFT/IPS, scene graph, DMA partial draw buffer | ST7789, GC9A01, touch/encoder (strictly ≥16 MB flash D9) |

#### Unified Hardware Abstraction (`IDisplayModule`)

Both modules implement the unified `IDisplayModule` interface defined in `include/interfaces/IDisplayModule.hpp` (see §6.7). This unified interface standardizes:
- **Geometry & Capability Queries:** `getDisplayWidth()`, `getDisplayHeight()`, `isColor()`, `getDisplayType()`
- **Brightness & Power Control:** `setBrightness(uint8_t)`, `getBrightness()`, `sleep()`, `wake()`, `isSleeping()`
- **Screensaver Idle Timings:** `setDimTimeoutMs()`, `setBlankTimeoutMs()`, `resetScreensaverActivity()`
- **On-Device PNG Streaming:** `capturePNG(BufferStream& stream)`
- **System Lifecycle Overlays:** `showSystemState(DisplaySystemState state)`
- **Boot Status Integration:** Inherits `IBootReporter` and `IStatusReporter` (`reportStatus`, `reportError`, `clearStatus`)

While `IDisplayModule` unifies hardware control, REST routes, and MCP tooling, application-level board views remain native to their rendering paradigm:
- U8G2 boards implement the page-buffered `IDisplayBoard` interface (§7.7).
- LVGL screens bind directly to LVGL Pro UI designs using reactive `sys_*` subjects (see [`modules/LVGLDisplay/README.md`](../modules/LVGLDisplay/README.md)).

### 7.2 `ILVGLHardwareAdapter` (Hardware Decoupling)

Defined in `modules/LVGLDisplay/ILVGLHardwareAdapter.hpp` (relocated from `include/interfaces/` per ADR-001). The adapter decouples physical display initialization, bus communication (SPI/QSPI/I80), touch controllers, rotary encoders, and backlight PWM from the generic `LVGLDisplayModule` engine:

```cpp
class ILVGLHardwareAdapter {
public:
    virtual ~ILVGLHardwareAdapter() = default;
    virtual bool initHardware() = 0;
    virtual lv_display_t* initDisplay(lv_display_t* disp) = 0;
    virtual lv_indev_t* initInput(lv_indev_t* indev) = 0;
    virtual void setBrightness(uint8_t level) = 0;
    virtual uint8_t getBrightness() const = 0;
    virtual void sleep() = 0;
    virtual void wake() = 0;
};
```

- **Reference Implementation:** [`CrowPanel128Adapter`](../adapters/lvgl/CrowPanel128Adapter.hpp) supports the CrowPanel 1.28" circular TFT (GC9A01 display, CST816S touch controller, EC11 rotary encoder).
- **16 MB Flash Requirement (D9):** Environments with LVGL v9, anti-aliased typography, dual 4 MB OTA app partitions, and compiled-in UI assets strictly require a minimum of 16 MB flash memory (e.g. `esp32s3nano`). 4 MB targets (`esp32dev`) are unsupported.
- **LVGL Pro Integration:** Binds designer-created UI projects via the `UIDesign` library and standard reactive subjects (`sys_heap`, `sys_state`, `sys_ip`, `sys_wifi`, `sys_ota`). See [`modules/LVGLDisplay/README.md`](../modules/LVGLDisplay/README.md) for full subject contracts and component architecture.

### 7.3 `DisplayServiceHelper` Library

Located in `lib/DisplayServiceHelper/`. Encapsulates shared web routes and MCP management tools for any module implementing `IDisplayModule`:

- **REST API Routes:**
  - `GET /api/display/brightness`, `POST /api/display/brightness` — queries or sets backlight brightness with optional `saveCallback` configuration persistence.
  - `GET /api/display/status` — reports display dimensions, color capability, brightness, sleep status, and active board name.
  - `GET /api/display/screenshot` — captures an instantaneous PNG screenshot streamed directly over HTTP.
- **MCP Tool Registration:** Registers `display_status`, `display_control`, and `capture_display` with the `MCPToolRegistry`.
- **Dedicated Stack Task (`DeepStackExecutor`):** Generating PNG screenshots requires significant stack memory (~16 KB) for compression buffers. `DisplayServiceHelper` offloads capture to an isolated FreeRTOS worker task (`png_worker`), preventing stack exhaustion on the web server or main loop tasks.

### 7.4 Display Layer Priority Stack

The display subsystem implements a strict three-layer rendering priority stack:

```
┌──────────────────────────────────────────┐  Priority
│  MODAL                                   │  HIGHEST
│  display.showModal(IDisplayBoard*)       │  transient, app-driven
│  display.clearModal()                    │  overrides base and screensaver
├──────────────────────────────────────────┤
│  BASE / CAROUSEL                         │  MID
│  display.showBase(IDisplayBoard*)        │  persistent, app-driven
│  display.carousel.*                      │  overrides screensaver
├──────────────────────────────────────────┤
│  SCREENSAVER                             │  LOWEST
│  display.screensaver.*                   │  display-driven (inactivity timer)
└──────────────────────────────────────────┘  yields to modal and base
```

- **MODAL (Highest):** Transient overlays driven by immediate application needs (e.g., `WiFiWizardBoard` during captive portal setup, `OTAProgressBoard` during firmware updates, `ErrorBoard` on fatal errors). Overrides carousel and screensaver.
- **BASE / CAROUSEL (Mid):** The persistent backdrop. Displays either the rotating user board carousel (`display.carousel.*`) during `Running`, or the `ConfiguringStatusBoard` during `Configuring`.
- **SCREENSAVER (Lowest):** Activated automatically by inactivity timers. Suppressed whenever any Modal is active or when user input wakes the display.
- **Safety Rule:** `ScreensaverBoard` must NEVER be pushed via `showModal()`. Doing so assigns it MODAL priority, which would suppress critical OTA progress and setup wizard screens.

### 7.5 Implemented Sub-Object API (`U8G2DisplayModule`)

The `U8G2DisplayModule` exposes three first-class sub-objects rather than flat root methods:

```
display
 ├── .carousel                 CarouselEngine — manages rotating board pool and intervals
 ├── .boards                   BoardCatalog   — named, concrete system board references
 │    ├── .boot                BootBoard&
 │    ├── .configuringStatus   ConfiguringStatusBoard&
 │    ├── .gateHelper          GenericGateHelperBoard&
 │    ├── .diagnostics         DiagnosticsBoard&
 │    ├── .wifiWizard          WiFiWizardBoard&
 │    ├── .otaProgress         OTAProgressBoard&
 │    └── .error               ErrorBoard&
 └── .screensaver              ScreensaverController — display-intrinsic idle policy
```

#### Canonical API Usage

```cpp
// Board configuration (concrete type domain concern):
display.boards.otaProgress.setProgress(0.5f, "Downloading...");
display.boards.wifiWizard.configure(hostname);

// Presentation control (polymorphic IDisplayBoard* boundary):
display.showModal(&display.boards.otaProgress);
display.clearModal();
display.showBase(&display.boards.configuringStatus);

// Carousel management (sub-object):
display.carousel.add(&myClockBoard);
display.carousel.suspend();
display.carousel.resume();
display.carousel.setInterval(8000);

// Screensaver idle policy:
display.screensaver.setDimTimeout(60000);
display.screensaver.setBlankTimeout(300000);
display.screensaver.resetActivity(); // called on input events
```

#### Service Module Boundary

`U8G2DisplayModule` is a pure service module: it accepts commands and renders. It does not poll `Application` state or read `AppState`. State transitions belong entirely to the `Application` coordinator:

```cpp
void Application::onEnterConfiguring() {
    _display.carousel.suspend();
    _display.showBase(&_display.boards.configuringStatus);
}

void Application::onWifiAPStarted(const char* hostname) {
    _display.boards.wifiWizard.configure(hostname);
    _display.showModal(&_display.boards.wifiWizard);
}

void Application::onWifiConnected() { _display.clearModal(); }
void Application::onEnterRunning()  { _display.carousel.resume(); }
```

### 7.6 Board Catalog & System Boards

The framework includes a comprehensive catalog of pre-built system boards stored as member variables within `BoardCatalog` (zero heap allocations during runtime):

| Board | Purpose | Visibility | Controller |
|---|---|---|---|
| `BootBoard` | Splash logo, version string, boot-phase progress | System | `Application` via `IStatusReporter` |
| `ConfiguringStatusBoard` | Precondition status list, boot progress bar, help prompts | System | `Application` via `showBase()` |
| `GenericGateHelperBoard` | Gate-specific troubleshooting instructions and IP guidance | System | `Application` via `showModal()` |
| `DiagnosticsBoard` | Real-time heap, internal DRAM, uptime, task diagnostics | User / App | App via `carousel.add()` or modal |
| `WiFiWizardBoard` | Captive portal AP credentials, URL, connection wizard | User-surfaced | `Application` (`onWifiAPStarted`) |
| `OTAProgressBoard` | Firmware update download progress bar and verification status | User-surfaced | OTA subsystem / EventBus |
| `ErrorBoard` | Fatal system halts or warning banners with error text | User-surfaced | Any module via `reportError()` |
| `ScreensaverBoard` | Low-power clock and burn-in prevention animations | Display-intrinsic | `ScreensaverController` |

### 7.7 `IDisplayBoard` Interface & Lifecycle (U8G2 Variant)

```cpp
class IDisplayBoard {
public:
    virtual const char* getBoardName() const = 0;
    virtual void onActivate() = 0;
    virtual void onDeactivate() = 0;
    virtual void tick(uint32_t ms) = 0;
    virtual void render(U8G2& display) = 0;
    virtual UpdateStatus updateData() = 0;
    virtual bool isScrollFinished() { return true; }
    virtual bool isExpired() { return false; }
    virtual ~IDisplayBoard() = default;
};
```

- **Asymmetric Expiry Model:** `isExpired()` dictates when a board should auto-dismiss:
  - `OTAProgressBoard`: Returns `true` only when download completes successfully. Failed OTA persists to display the error.
  - `ErrorBoard`: Returns `true` only when explicitly dismissed via `dismiss()`.
  - `ScreensaverBoard`: Returns `true` when inactive (`!_active`).
  - Transient splash boards auto-expire after their timeout elapses.

### 7.8 Widget Toolkit Architecture

Located in `modules/U8G2Display/widgets/`. Provides composable UI primitives for U8G2 displays:

- **Tier 1 (Primitives):** `iGfxWidget` (concrete `x, y, width, height` members, zero vtable query overhead), `drawingPrimitives` (stateless text alignment and clipping functions), `U8g2StateSaver` (RAII font/color clip scope guard), `iBoardLayout`.
- **Tier 2 (Generic Widgets):** `labelWidget`, `scrollingTextWidget`, `clockWidget`, `progressBarWidget`, `imageWidget`.
- **Tier 3 (Module-Aware Widgets):** `wifiStatusWidget`, `systemMessageWidget`, `scrollingMessagePoolWidget` (consumes `"display.message"` EventBus topic).
- **Dynamic Dimension Queries:** Widgets query `display.getDisplayWidth()` and `display.getDisplayHeight()` dynamically at runtime with fallback defaults, avoiding hardcoded compile-time screen dimensions.

---

## 8. Configuration Architecture

> 📖 **Developer Guide:** [Configuration System](developer/config-system.md) — saving, migrating, and restoring module settings.

### 8.1 ConfigManager-as-Gateway Pattern

The framework standardises on a centralized configuration gateway model. Modules **never interact directly with LittleFS** or perform raw file I/O for configuration. Instead, `ConfigManager` (`modules/ConfigManager/ConfigManager.hpp`) serves as the sole gateway for file operations, JSON serialization, schema versioning, and atomic persistence.

#### Storage Organization & Path Conventions

Configuration files are partitioned cleanly by ownership across dedicated paths on flash storage:

| Path | Owner | Purpose & Format |
|---|---|---|
| `/etc/config.json` | Framework kernel | `FrameworkConfig` (hostname, timezone, OTA settings, publish interval) |
| `/etc/apikeys.json` | Framework kernel | `KeyManager` AES-GCM encrypted credential vault |
| `/mod/<namespace>.json` | `ConfigManager` | Module configuration files, keyed by `getConfigNamespace()` |
| `/config_manifest.json` | `ConfigManifest` | Three-tier lifecycle manifest tracking schema versions |
| `/app/<appname>.json` | Application | Application domain settings (managed via `IConfigurable`) |

#### Filesystem Mount Point (`ConfigRoot`)

When a dedicated `config` LittleFS partition exists (defined in `partitions.csv`), the framework mounts it at `/config` for maximum wear-leveling isolation. If no separate config partition is present, it falls back to the main `/littlefs` storage partition. In native GoogleTest runs, storage resolves to `test/tmp/config`.

#### Transactional File Persistence

All configuration writes follow an atomic two-phase write pattern:
1. Serialize configuration payload to a temporary file (`/mod/<namespace>.json.tmp`).
2. Call `flush()` to ensure buffers are physically written to flash.
3. Atomically rename the temporary file to `/mod/<namespace>.json` via POSIX `rename()`.

If a brownout or power loss occurs mid-write, the existing configuration remains intact, eliminating corrupted or truncated configuration files.

### 8.2 The `IConfigurable` Contract

Modules participate in configuration persistence by implementing the pure interface [`IConfigurable`](include/interfaces/IConfigurable.hpp) (see §6.3):

```cpp
class IConfigurable {
public:
    virtual ~IConfigurable() = default;

    virtual const char* getConfigNamespace() const = 0;
    virtual int getSchemaVersion() const = 0;
    virtual void applyConfig(const JsonObject& section) = 0;
    virtual void serializeConfig(JsonObject& section) const = 0;
    virtual bool migrateConfig(JsonObject& section, int fromVersion) { return true; }
    virtual void resetConfig() = 0;
};
```

- **`getConfigNamespace()`**: Returns a unique, lowercase compile-time string literal (e.g. `"wifi"`, `"display"`, `"ota"`). Determines the filename at `/mod/<namespace>.json`.
- **`getSchemaVersion()`**: Returns an integer schema version (starting at 1). Incremented whenever configuration keys or types change.
- **`applyConfig(const JsonObject& section)`**: Deserializes JSON settings into internal state. Called at boot (`beginAll()`), after mutations (`saveModuleConfig()`), or upon backup restoration.
- **`serializeConfig(JsonObject& section) const`**: Serializes current internal state into the JSON object. `ConfigManager` automatically injects the metadata field `_v`.
- **`migrateConfig(JsonObject& section, int fromVersion)`**: Migrates an older schema in-place. Returning `true` persists the migrated format; `false` triggers a fallback to defaults.
- **`resetConfig()`**: Resets all internal settings to hardcoded factory defaults.

Modules register with `ConfigManager` automatically when passed to `app.registerModule(&module)`.

### 8.3 Schema Versioning & Migration Lifecycle

Each `/mod/<namespace>.json` file maintains a root-level integer field `_v` managed exclusively by `ConfigManager`. During `ConfigManager::beginAll()` (Boot Step 8):

1. **File Read:** `ConfigManager` loads `/mod/<namespace>.json`. If missing, it invokes `resetConfig()`, serializes defaults, and writes the initial file.
2. **Version Inspection:** The stored `_v` is compared against `module.getSchemaVersion()`.
3. **Migration Execution:** If `stored_v < current_v`, `migrateConfig(section, stored_v)` is executed:
   - If migration succeeds (`true`), `_v` is updated to `current_v` and the migrated file is atomically saved back to flash.
   - If migration fails (`false`) or the JSON payload is malformed, an error is logged, `resetConfig()` is called, and factory defaults are persisted.
4. **State Application:** `module.applyConfig(section)` is called to populate runtime module memory.

### 8.4 Runtime Persistence (`saveModuleConfig`)

When a module or web route mutates settings at runtime, it triggers persistence through `ConfigManager`:

```cpp
// Save and re-apply settings for a module namespace:
configManager.saveModuleConfig("display");

// Or by pointer:
configManager.saveModuleConfig(&myModule);
```

#### Mutation Sequence
1. Allocates a bounded `JsonDocument`.
2. Invokes `module->serializeConfig(root)`.
3. Injects `root["_v"] = module->getSchemaVersion()`.
4. Writes atomically to `/mod/<namespace>.json` via write-to-temp and atomic rename.
5. Re-applies immediately: calls `module->applyConfig(root)`.

This update is **synchronous and immediate**. No device reboot or asynchronous reload dispatch is required.

### 8.5 Three-Tier Configuration Manifest (`lib/ConfigManifest/`)

The `ConfigManifest` subsystem tracks configuration schema lifecycles across firmware updates:

- **Active:** The schema version currently running on the device.
- **Previous:** The schema version that was active prior to the last OTA update.
- **Golden:** The last-known-good schema version, snapshotted when the firmware passes validation and is blessed via `snapshotActiveToGolden()`.

The manifest is maintained in `/config_manifest.json` across up to 16 module slots (`ModuleSlot`), providing an audit trail and fallback reference during rollback scenarios.

### 8.6 Configuration Backup, Export & Import

`ConfigManager` provides holistic backup and restore across all registered modules:

- **Export (`exportAllConfig`):** Iterates all registered `IConfigurable` modules and compiles their serialized JSON objects into a single backup envelope:
  ```json
  {
    "version": 1,
    "device": "esp32-app",
    "modules": {
      "wifi": { "_v": 1, "hostname": "living-room" },
      "display": { "_v": 1, "brightness": 255 }
    }
  }
  ```
- **Import (`importAllConfig`):** Validates schema versions, imports sections into their respective `/mod/<namespace>.json` files, and re-applies configuration across all modules.

### 8.7 JSON Memory Safety (ArduinoJson v7)

The framework standardises strictly on **ArduinoJson v7**:

- **`JsonDocument` Unified Document Type:** Replaces v6 `DynamicJsonDocument` and `StaticJsonDocument<N>`. Allocation is memory-pool based and handles both small and large documents safely.
- **Banned Types:** `DynamicJsonDocument` and `StaticJsonDocument` are **strictly prohibited** across the codebase.
- **Safe Extraction:** Always use the fallback operator `config["key"] | defaultValue` rather than `.as<T>()` to prevent crashes on missing or null keys.

---

## 9. Application Lifecycle

> 📖 **Developer Guide:** [Architecture Overview](developer/architecture-overview.md) — boot sequence, tick loop, and state transitions.

### 9.1 Three-State Bidirectional Lifecycle Machine

The application lifecycle is governed by an explicit 3-state machine defined in `include/types/AppState.hpp`:

```cpp
enum class AppState : uint8_t {
    Boot        = 0, ///< Hardware init, module begin() — only valid during setup()
    Configuring = 1, ///< Kernel up, awaiting user configuration / preconditions
    Running     = 2  ///< Normal tick() loop; all prerequisites satisfied
};
```

Legacy states `WIFI_SETUP` and `APP_SETUP` are completely removed, unified into `AppState::Configuring`.

```
┌────────────────────────┐
│       AppState::Boot   │ (setup / hardware init)
└───────────┬────────────┘
            │
            ├──────────────────────────────────────┐
            │ (startup preconditions unsatisfied)  │ (all startup preconditions satisfied)
            ▼                                      ▼
┌────────────────────────┐  all startup met   ┌────────────────────────┐
│  AppState::Configuring │ ─────────────────> │   AppState::Running    │
│  (portal up, waiting)  │ <───────────────── │   (normal operation)   │
└────────────────────────┘ startup suppressed └────────────────────────┘
                             (regression)
```

- **`AppState::Boot` (0):** Ephemeral hardware bootstrap phase. DeviceCrypto initializes, filesystems mount, configuration loads, and modules are wired. Valid only during `Application::begin()`.
- **`AppState::Configuring` (1):** Kernel services, web portal, captive AP, and background workers are active, but the device is waiting for mandatory configuration or startup preconditions to be satisfied. The display presents `ConfiguringStatusBoard`.
- **`AppState::Running` (2):** Normal operation. All startup preconditions are satisfied. Background data sources fetch on their declared schedules, and the display carousel cycles user boards.

#### Bidirectional State Transitions & Regression

Transitions between `Configuring` and `Running` are fully bidirectional:
- **`Boot → Configuring`:** At the conclusion of `begin()`, one or more `Startup`-role preconditions remain unsatisfied.
- **`Boot → Running`:** At the conclusion of `begin()`, all `Startup`-role preconditions are already satisfied.
- **`Configuring → Running`:** During `tick()`, `PreconditionEngine::allStartupMet()` becomes true. The framework publishes `ApplicationEvents::APP_RUNNING` (`"app.running"`).
- **`Running → Configuring` (Regression):** During `tick()`, if any `Startup` or `Both`-role precondition becomes suppressed (e.g. WiFi connection drops and exceeds flap damping tolerance), the application regresses back to `Configuring`. The framework publishes `ApplicationEvents::APP_CONFIGURING` (`"app.configuring"`), suspends the display carousel, and presents the configuration screen.

### 9.2 The PreconditionEngine Gate Subsystem

The framework replaces coarse boolean gates with the `PreconditionEngine` (`modules/PreconditionEngine/PreconditionEngine.hpp`), providing fine-grained dependency modeling, structured causes, and automated flap suppression.

#### 9.2.1 Three-Question Split & Precondition Roles

One predicate cannot answer whether a device may boot, whether it should be serving, and whether a fault is severe enough to regress. Preconditions declare their operational scope via `PreconditionRole`:

```cpp
enum class PreconditionRole : uint8_t {
    Operational = 0,  ///< Features degrade while unmet. Does NOT block boot. (default)
    Startup,          ///< Must be met to leave boot / enter Running.
    Both              ///< Blocks startup AND causes regression if suppressed at runtime.
};
```

**The default role is `Operational`.** Declaring a precondition is cheap and non-blocking by default; blocking boot requires deliberate design intent.

#### 9.2.2 Frequency-Based Status & Description Split

To prevent per-tick string formatting and heap allocations, status evaluation and description formatting are decoupled by frequency:

```cpp
struct PreconditionStatus {
    bool  met   = false;
    Cause cause = Cause::NotConfigured;
};

// HOT PATH — evaluated every tick. Zero allocation, no formatting.
virtual PreconditionStatus getStatus() const = 0;

// COLD PATH — invoked only when publishing or rendering into a CALLER-SUPPLIED buffer.
virtual void describe(Cause cause, char* buf, size_t len) const {}
```

- Single-cause preconditions do not need to override `describe()` (zero cost).
- Multi-cause preconditions format human-readable descriptions into caller-owned memory, eliminating dangling pointers and `mutable` member variables.

#### 9.2.3 Structured Cause Vocabulary

```cpp
enum class Cause : uint8_t {
    Met = 0,
    NotConfigured,    ///< Missing credentials or user inputs. Person must act.
    Connecting,       ///< In progress; expected to resolve automatically.
    Rejected,         ///< Configuration rejected by peer (e.g. invalid password/token).
    Unreachable,      ///< Target endpoint configured but peer does not answer.
    RateLimited,      ///< Peer is reachable but requesting throttling.
    Implausible,      ///< Data received but failed domain validation (e.g. flatline sensor).
    DependencyUnmet,  ///< Upstream precondition is unsatisfied.
    FallbackEngaged,  ///< Degraded but serviceable operating mode (e.g. AP portal mode).
    Unavailable,      ///< Feature intentionally unconfigured or hardware absent. Not a fault.
    Indeterminate,    ///< Dependency was not wired into module (programming error, loud).
    InternalError     ///< System defect or hardware fault.
};
```

#### 9.2.4 Derived Severity & Trust History

Severity is derived by the framework to guide UI presentation and escalation:

| Cause | Derived Severity | Meaning |
|---|---|---|
| `Connecting`, `Unreachable`, `RateLimited` | **Transient** | Temporary glitch; keep retrying without user alarm. |
| `Implausible` | **Transient** or **NeedsUser** | Discriminated by trust history (`everMet`). |
| `NotConfigured`, `Rejected` | **NeedsUser** | Actionable only by the user. |
| `DependencyUnmet` | **Blocked** | Upstream precondition owns user notification. |
| `Indeterminate`, `InternalError` | **Persistent** | Developer or system defect; reported loudly. |
| `FallbackEngaged` | **Degraded** | Operating under degraded fallback mode. |
| `Unavailable` | **Inactive** | Intentionally omitted; reported once, never nags. |

**Trust History:** Framework-owned `bool everMet` and `uint32_t identityEpoch` track whether a precondition has ever succeeded. A LAN peer that worked previously and disconnected is `Transient`; a LAN peer that has never once succeeded is `NeedsUser` (flagging a likely mistyped IP address).

#### 9.2.5 Typed Dependency Edges & Cycle Rejection

Preconditions define dependency edges using typed strengths:
- **`Wants`:** Presentation hint only; does not block evaluation.
- **`Requires`:** Strict dependency; dependent cannot be met unless prerequisite is met.
- **`BindsTo`:** `Requires` plus runtime linkage; if prerequisite becomes unmet, dependent immediately un-satisfies.

All edges are validated once during boot (`resolveEdges()`). Unknown precondition names and dependency cycles are rejected loudly at startup.

#### 9.2.6 Flap Damping & Recovery Probes

To prevent network glitches from bouncing the device between states, failures accrue penalty points per second:
- Penalty accrues per second based on severity; decays exponentially over a runtime-configurable half-life.
- **Asymmetric Thresholds:** Crossing `suppressThreshold` (e.g. 2000) causes regression; returning requires falling below `reuseThreshold` (e.g. 750), preventing hysteresis flapping.
- **Half-Open Recovery:** When retries exhaust, fallback engages via the EventBus (`precondition.fallback.requested`). The resource owner executes half-open trial probes after a cooldown period. Manual user retries bypass the breaker.

#### 9.2.7 Dual Status Publication

The framework publishes both undamped and damped statuses on `ApplicationEvents::PRECONDITION_HEALTH` (`"precondition.health"`). Subsystems select the view suited to their consequences: irreversible integrations (such as power accumulation counters) read undamped status to cut off integration immediately, while displays read damped status to prevent cosmetic UI flicker.

#### 9.2.8 Registration Pattern (`IPreconditionContributor`)

Modules register preconditions using the visitor pattern to prevent silent omission:

```cpp
class IPreconditionContributor {
public:
    virtual ~IPreconditionContributor() = default;
    virtual void registerPreconditions(IPreconditionRegistry& registry) = 0;
};
```

### 9.3 Framework Deterministic `begin()` Sequence

The framework executes boot across 20 deterministic steps in `Application::begin()`:

1. **Early Boot Banner:** Log `BUILD_VERSION` and framework identifier to serial.
2. **Install Log Pipeline (Step 1b):** Register `CompositeSanitizer` with `Redactor` (and `StructuralScrubber` on verbose builds) via `esp_log_set_vprintf`. All subsequent logs are redacted. Check PSRAM if configured.
3. **Enter BOOT State:** Set `AppState::Boot` and initialize early `_statusReporter`.
4. **Initialize EventBus:** Call `_eventBus.begin()`. First service ready; enables event dispatch.
5. **Mount Filesystem:** Mount the dedicated `/config` partition (if present) and `/littlefs` storage partition.
6. **Load FrameworkConfig:** Store internal `FrameworkConfig` and configure publishing throttles.
7. **Security Subsystems:** Initialize `DeviceCrypto.begin()` and `KeyManager.begin(&_deviceCrypto)`.
8. **Dependency Injection & Framework Registrations:** Wire dependencies and register core modules (`WifiManager`, `AuthManager`, `ConfigManager`, `KeyManager`, `WebServerManager`, `PreconditionWebContributor`, `DataSourceWebContributor`, `AuditModule`, WiFi preconditions).
9. **ConfigManager Gateway Boot:** Call `_configManager.beginAll()`. Load all `/mod/<ns>.json`, execute migrations, and apply configuration to all registered modules.
10. **Custom Module Registration:** Register application-specific modules passed via `app.registerModule()`.
11. **WifiManager Begin:** Call `_wifiManager.begin()`. Initiate network interface with loaded credentials.
12. **WiFi Event Binding:** Subscribe to `wifi.ap_started` to automatically enable `AuthManager` AP-mode authentication bypass.
13. **DataManager Begin:** Start background data worker FreeRTOS task and register all `IDataSource` workers.
14. **Time Sync:** Initialize asynchronous NTP synchronization via `TimeManager` if present.
15. **Display Hardware Init:** Initialize display adapter and show early boot splash.
16. **WebServerManager Begin:** Start `PsychicHttpServer`, bind static PROGMEM assets, and register all contributor routes and tabs.
17. **MCP Server Init:** Register built-in tools and all `IMCPContributor` tools with `MCPToolRegistry`.
18. **Lock Registration Vectors:** Call `_compactAndLock()` to seal module arrays and execute `_preconditionEngine.resolveEdges()`.
19. **Firmware Self-Test Logging (Step 17b):** Log self-test pass. (App validation is delegated to the consumer; see §9.5).
20. **Heap & Gate Evaluation (Steps 18–20):** Publish `system.heap`, clear status overlay, evaluate `PreconditionEngine::allStartupMet()`, and transition to either `AppState::Running` or `AppState::Configuring`.

### 9.4 Framework `tick()` Sequence

Executed continuously on Core 1 in the main loop:

1. **State Guard:** Return immediately if `_state` is neither `Configuring` nor `Running`.
2. **PreconditionEngine Tick:** Call `_preconditionEngine.tick(currentMillis)`.
3. **Throttled Gate Check (500ms):** If in `Configuring`, evaluate blocking gates and re-publish `app.configuring` if gate status or IP address changed. If all startup preconditions are met, transition to `Running` and emit `app.running`.
4. **Native Simulation:** Under `NATIVE_BUILD`, call `_dataManager.processNext()` synchronously.
5. **Tickable Module Dispatch:** Iterate registered `ITickable` modules in registration order. In debug builds, monitor execution duration against `module->getTickBudgetMs()` (default 10ms) and emit warnings on budget overruns.
6. **Precondition Health & Regression Monitor:** Detect status changes across all preconditions and publish `precondition.health`. If in `Running` and any `Startup` or `Both` precondition becomes suppressed, immediately regress to `Configuring` and publish `app.configuring`.
7. **Periodic Heartbeat (~30s):** Emit system telemetry log: `♥ uptime=%lus free_heap=%luB dram=%luB dram_min=%luB` (uptime, free heap, internal DRAM, and minimum DRAM watermark since boot).
8. **Service Maintenance:** Pump periodic background maintenance for `AuthManager` session expiration, `WifiManager` connection state, and `AuditModule` flash sync.

### 9.5 Firmware Blessing & Soft Reset

#### Consumer-Delegated Firmware Blessing (`markAppValid`)

The framework does NOT automatically bless new OTA firmware images upon successful boot. Downstream applications must explicitly validate that their domain dependencies are working (e.g. successful cloud communication or valid sensor acquisition) and call:

```cpp
otaManager.markAppValid();
```

If the device reboots or watchdog-resets before `markAppValid()` is called, the ESP-IDF anti-rollback system marks the partition invalid and automatically reverts to the previous operational firmware slot.

#### Soft Reset (`softReset`)

```cpp
void Application::softReset();
```

Logs a warning identifying the reset trigger and calls `esp_restart()` directly. In native unit test builds, `esp_restart()` delegates to a test harness wrapper to verify graceful teardown without crashing the test process.

---

## 10. Web Portal Architecture

> 📖 **Developer Guide:** [Extending the Web Portal](developer/extending-the-web-portal.md) — custom tabs, REST endpoints, and JWT authentication.

### 10.1 Single Page Application (SPA) & PsychicHttp Server

The framework web portal operates as a PROGMEM-hosted Single Page Application (SPA) served by **`PsychicHttpServer`** (`PsychicHttp.h`). The legacy `ESPAsyncWebServer` has been completely replaced.

#### Server Features & Dual-Server TLS Mode
- **Asynchronous HTTP (Port 80):** Serves static PROGMEM assets at ~0ms latency and handles JSON REST APIs.
- **Dual-Server TLS Mode (Port 443):** When authentication is enabled, a certificate is loaded, and the device is not in AP mode, a `PsychicHttpsServer` serves HTTPS on port 443 while port 80 automatically redirects incoming requests to HTTPS. In AP mode or when TLS is unconfigured, all routes run open over HTTP port 80.
- **Heap Guard Semaphore:** `WebServerManager` maintains an atomic counter of concurrent high-memory requests (`MAX_CONCURRENT_HIGH_MEM = 2`). If concurrency exceeds this limit or the largest free DRAM block drops below `HEAP_GUARD_MIN_BLOCK_BYTES` (16 KB), incoming requests receive HTTP 503 "Low memory — retry".
- **Socket Throttling:** Limited by `BOARD_HTTP_MAX_OPEN_SOCKETS` (default 4; 2–3 browser connections + 1 MCP connection) to protect internal DRAM.

### 10.2 PROGMEM Static Asset Inventory (20 Assets)

The framework SPA shell is compiled directly into flash memory (`rodata`) by `portalBuilder.py` as gzip-compressed byte arrays (`include/portal_assets/*.hpp`), eliminating any LittleFS dependency for base portal functionality:

| # | Asset Header | Purpose & Content |
|---|---|---|
| 1 | `index_html.hpp` | Compressed HTML5 SPA shell, responsive viewport, mount container |
| 2 | `app_css.hpp` | Core CSS styles, dark-theme custom properties, BEM component library |
| 3 | `app_js.hpp` | Application bootstrapper, DOM router, `ApiQueue`, modal manager |
| 4 | `utils_js.hpp` | Shared frontend utilities, toast alerts, escaping, formatting |
| 5 | `wifi_js.hpp` | WiFi scanner, network selection, AP configuration, signal gauges |
| 6 | `system_js.hpp` | System info, real-time heap gauges, uptime counter, restart modal |
| 7 | `ota_js.hpp` | OTA firmware updates, upload forms, progress bars, version check |
| 8 | `display_js.hpp` | Backlight sliders, display status metrics, screenshot stream viewer |
| 9 | `auth_js.hpp` | Session login, password rotation, bearer token generator |
| 10 | `keys_js.hpp` | KeyManager vault editor, secret registration, credential table |
| 11 | `mcp_js.hpp` | MCP tool inspector, JSON-RPC endpoint viewer, read-only mode toggle |
| 12 | `time_js.hpp` | Timezone selector, NTP server configuration, sync status |
| 13 | `backup_js.hpp` | Full configuration JSON export, file upload restore, factory reset |
| 14 | `appconfig_js.hpp` | Generic schema-driven application configuration tab |
| 15 | `hardware_js.hpp` | Hardware pinouts, peripheral status, hardware configuration viewer |
| 16 | `carousel_js.hpp` | U8G2 display carousel board list, interval settings, active board control |
| 17 | `homeassistant_js.hpp` | Home Assistant MQTT discovery state, entity monitor |
| 18 | `screenshot_html.hpp` | Standalone full-screen screenshot preview page |
| 19 | `screenshot_js.hpp` | Continuous screenshot polling and stream rendering logic |
| 20 | `credits_js.hpp` | Open-source licenses, framework credits, build metadata |

### 10.3 Navigation Hierarchy: Pills & Sub-Pills

Navigation tabs are organized hierarchically using pill and sub-pill selectors registered via `ITabRegistry`:

```cpp
class IWebContributor {
public:
    virtual ~IWebContributor() = default;
    virtual void registerRoutes(PsychicHttpServer& server) = 0;
    virtual void registerTabs(ITabRegistry& registry) = 0;
};
```

#### Multi-Tab Registration (`ITabRegistry`)

Modules register one or more tabs in a single `registerTabs()` call:

```cpp
void MyModule::registerTabs(ITabRegistry& registry) {
    // Top-level pill
    registry.addTab("Sensors", nullptr, 100, ICON_SENSOR_SVG, "/js/sensors.js");

    // Nested sub-pill under "Sensors"
    registry.addTab("Calibration", "Sensors", 110, nullptr, "/js/calibration.js");
}
```

The portal queries `GET /api/tabs` on startup and dynamically renders top-level navigation pills and nested sub-pill toolbars.

### 10.4 REST Route Registration & PsychicHttp Patterns

Modules install routes on `PsychicHttpServer` during the `registerRoutes()` phase:

```cpp
void MyModule::registerRoutes(PsychicHttpServer& server) {
    // GET endpoint:
    server.on("/api/sensors", HTTP_GET, [this](PsychicRequest* request) {
        JsonDocument doc;
        doc["temperature"] = _temp;
        doc["humidity"] = _humidity;
        String response;
        serializeJson(doc, response);
        return request->reply("application/json", response.c_str());
    });

    // POST endpoint with body handling:
    server.on("/api/sensors/config", HTTP_POST, [this](PsychicRequest* request) {
        if (request->contentLength() > 1024) {
            return request->reply(413, "text/plain", "Payload too large");
        }
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, request->body());
        if (err) {
            return request->reply(400, "text/plain", "Malformed JSON");
        }
        _tempOffset = doc["offset"] | 0.0f;
        return request->reply("application/json", "{\"ok\":true}");
    });
}
```

### 10.5 POST Body Handling & Memory Safety (ADR-15)

In compliance with ADR-15 and embedded memory budgets, POST request bodies are strictly guarded:

- **Per-Route Payload Ceilings:** Web routes inspect `request->contentLength()` and reject oversize requests immediately with HTTP 413:
  - System restart/actions: ≤256 B
  - Sensor/WiFi configuration: ≤1 KB
  - Full configuration restore: ≤8 KB
- **ArduinoJson v7 Deserialization:** Handlers deserialize directly from `request->body()` using a scoped `JsonDocument`.
- **Heap Protection:** Memory allocations for incoming payloads are strictly bounded before JSON parsing begins.

### 10.6 Portal Design System & Frontend Architecture

The web frontend follows the **BEM (Block Element Modifier)** methodology and lightweight component design:

- **Component Classes:** `.card`, `.card__header`, `.card__body`, `.alert`, `.alert--warning`, `.alert--danger`, `.btn`, `.btn--primary`, `.btn--danger`, `.form-group`, `.form-control`.
- **CSS Architecture:** Dark-theme first, custom CSS variables (`--bg-primary`, `--accent-color`), responsive grid layouts for mobile and desktop screens.

#### `ApiQueue` Priority Lanes

To prevent saturating the ESP32's limited TCP sockets (`BOARD_HTTP_MAX_OPEN_SOCKETS = 4`), all frontend requests route through `window.apiFetch(url, options, priority)`. The `ApiQueue` enforces **exactly one request inflight at a time** across three prioritized lanes:

1. **Priority Lane:** Immediate user interactions (e.g. saving settings, triggering reboot, initiating WiFi connection).
2. **Normal Lane:** Navigation tab loads and DOM fragment fetches.
3. **Background Lane:** Non-critical background polling (heap metrics, RSSI, telemetry).

### 10.7 AuthManager Security Integration

All `/api/*` endpoints are protected by centralized authentication middleware in `WebServerManager`:

- **Session Cookies:** The web browser authenticates via `POST /api/auth/login` and receives an `esp32_session` cookie.
- **Bearer Tokens:** External API clients (scripts, automation) pass credentials via `Authorization: Bearer <token>`.
- **Security Tab:** Provides a built-in interface for changing administrator passwords, rotating keys, and invalidating active sessions.
- **Automatic AP Mode Bypass:** When the device starts in AP mode (`wifi.ap_started`), `Application` automatically notifies `AuthManager::setAPMode(true)`. In AP mode, authentication checks on the portal are bypassed, allowing owners to connect and configure the device without pre-existing credentials.

### 10.8 AP Mode Inactivity Timeout

When operating in captive AP mode, `WifiManager` continuously tracks API interaction:
- Each HTTP request updates `lastApiRequestTime` via `notifyApiActivity()`.
- After **1 hour** (3,600,000 ms) of API inactivity, AP mode transitions to a power-saving shutdown state to preserve battery and minimize wireless exposure.

---

## 11. Inter-Module Communication

> 📖 **Developer Guide:** [Event Bus](developer/event-bus.md) — decoupled, event-driven cross-module communication.

### 11.1 The Core Decoupling Rule

**Modules never call each other directly.** All inter-module communication is mediated through framework mechanisms:

1. **`EventBus`:** Asynchronous publish/subscribe messaging for cross-module notifications.
2. **Centralized Gateways:** `ConfigManager` for persistence, `DataManager` for background data fetching, `PreconditionEngine` for lifecycle gating.
3. **Pure Interfaces:** Polymorphic contracts (`ITickable`, `IConfigurable`, `IStatusReporter`, `IWebContributor`, `IMCPContributor`).

### 11.2 EventBus Architecture

The `EventBus` (`modules/EventBus/EventBus.hpp`) provides lightweight, decoupled publish/subscribe event distribution.

#### Execution Context & Concurrency Model
- **Target Hardware (ESP32/FreeRTOS):** `publish()` posts events to the `esp_event` default event loop via `esp_event_post()`. Subscriber callbacks execute **asynchronously on the default event loop task** — not within the publisher's task. There is no synchronous ordering guarantee between `publish()` returning and callbacks executing.
- **Native Unit Test Builds:** `publish()` executes callbacks synchronously and inline within the caller's thread for deterministic assertions.

#### Topic Interning (FNV-1a Hash)
Topics are represented as `const char*` string literals. Upon subscription and publication, topic strings are interned to 32-bit FNV-1a hashes (`uint32_t`) via `EventBus::hashTopic(topic)`. Hot-path vector lookups match integer hashes directly, incurring **zero string comparisons**.

#### Dynamic Capacity & Vector Storage
There are **no fixed subscriber ceilings or topic length limits**. Subscribers are maintained in `std::vector<SubscriberEntry>`. Soft-deletion (tombstoning) guarantees iterator stability if callbacks subscribe or unsubscribe during an active dispatch; inactive entries are compacted once top-level dispatch completes.

#### Static Callback Function Pointer Pattern
To prevent heap allocations and memory fragmentation, subscriptions use static function pointers with a user context pointer:

```cpp
typedef void (*EventCallback)(const char* topic, const void* payload, void* ctx);
```

```cpp
// Subscriber registration:
static void onOtaProgress(const char* topic, const void* payload, void* ctx) {
    auto* self = static_cast<MyModule*>(ctx);
    const auto* p = static_cast<const OtaProgressPayload*>(payload);
    self->handleProgress(p->progress, p->status);
}

app.getEventBus().subscribe("ota.progress", onOtaProgress, this);
```

#### Concurrency & Payload Lifetime Rules
- **Subscriber-Side Rule:** The `const void* payload` pointer is valid **ONLY during callback execution**. If subscriber logic needs payload data beyond the callback (e.g. processing in `tick()`), it MUST copy the data into internal member storage.
- **Publisher-Side Rule:** Because dispatch on hardware is asynchronous, **stack-allocated payloads are strictly prohibited on target**. A stack-local payload will be corrupted before the `esp_event` task executes the callback. Publishers must use:
  - (a) A persistent class member variable (e.g. `Application::_systemHeapPayload`), or
  - (b) A ring buffer sized to the maximum publish cadence (e.g. `ButtonModule::_eventRing`).
- **Thread Safety for Single-Threaded Libraries:** Because callbacks execute on the `esp_event` task, callbacks MUST NEVER invoke single-threaded rendering APIs (such as `lv_obj_*`, `lv_timer_*`, or U8G2 draw routines) directly. Instead, set an atomic flag or copy data, and process the change during the module's main-loop `tick()`.

### 11.3 `std::function` Exception Policy

While `std::function` is prohibited on the inter-module EventBus to prevent heap fragmentation on microcontrollers, it is classified as a **documented exception** for point-to-point bindings and third-party library adapters per ADR-001 and `docs/developer/coding-patterns.md` [PA-6]:

- **Permitted Use Cases:**
  - One-time setup callbacks between tightly coupled kernel services (e.g., `DisplayServiceHelper::setSaveCallback`, OTA update progress delegates).
  - Third-party library interfaces requiring `std::function` (e.g., `PsychicHttpRequestCallback` in `PsychicHttpServer`).
- **Prohibited Use Cases:** High-frequency event dispatch, inter-module pub/sub, or per-tick execution loops.

### 11.4 `IStatusReporter` (Structured UI Status)

Defined in `include/interfaces/IStatusReporter.hpp`. Provides a clean abstraction for long-running operations (boot progress, WiFi connection, OTA firmware downloads) to report status without coupling to a concrete display module:

```cpp
class IStatusReporter {
public:
    virtual ~IStatusReporter() = default;
    virtual void reportStatus(const char* message, float progress = -1.0f) = 0;
    virtual void reportError(const char* errorMessage) = 0;
    virtual void clearStatus() = 0;
};
```

- **`reportStatus(msg, progress)`:** Displays a one-line status string and optional progress fraction (`[0.0, 1.0]`; negative values indicate indeterminate progress).
- **`reportError(errorMessage)`:** Displays a fatal or persistent error overlay.
- **`clearStatus()`:** Dismisses active status overlays from the screen.

When no display is attached, `Application` uses a silent `NullStatusReporter` no-op implementation.

### 11.5 Standard Framework Event Topics Registry

The framework kernel and standard modules publish the following topics across the `EventBus`:

| Topic String | Topic Constant | Publisher | Payload Type | Description |
|---|---|---|---|---|
| `"system.heap"` | `ApplicationEvents::SYSTEM_HEAP` | `Application` | `const uint32_t*` | Free heap memory in bytes (emitted periodically and at boot). |
| `"app.running"` | `ApplicationEvents::APP_RUNNING` | `Application` | `nullptr` | Emitted when all startup preconditions are met and system enters Running. |
| `"app.configuring"` | `ApplicationEvents::APP_CONFIGURING` | `Application` | `const AppConfiguringPayload*` | Emitted when entering Configuring or when a blocking gate changes. |
| `"precondition.health"` | `ApplicationEvents::PRECONDITION_HEALTH` | `Application` | `const PreconditionHealthPayload*` | Emitted when any precondition status, cause, or severity changes. |
| `"data.source.health"` | `ApplicationEvents::DATA_SOURCE_HEALTH` | `DataManager` | `const DataSourceHealthPayload*` | Emitted when a background data source health state changes. |
| `"ota.progress"` | `ApplicationEvents::OTA_PROGRESS` | `OTAUpdateManager` | `const OtaProgressPayload*` | Emitted during firmware downloads (progress, status, complete, success). |
| `"display.sleep"` | `U8G2DisplayEvents::DISPLAY_SLEEP` | Display Modules | `nullptr` | Emitted when screensaver enters sleep mode. |
| `"display.wake"` | `U8G2DisplayEvents::DISPLAY_WAKE` | Display Modules | `nullptr` | Emitted when user interaction wakes the display. |
| `"display.board.changed"` | `U8G2DisplayEvents::DISPLAY_BOARD_CHANGED` | `U8G2DisplayModule` | `const DisplayBoardChangedPayload*` | Emitted when active carousel board changes (boardName, boardIndex). |
| `"display.message"` | `U8G2DisplayEvents::DISPLAY_MESSAGE` | *Any Module* | `const DisplayMessagePayload*` | Subscribed by display modules to present scrolling message overlays. |
| `"wifi.connected"` | `WifiEvents::WIFI_CONNECTED` | `WifiManager` | `const WifiConnectedPayload*` | Emitted when station connects (IP, SSID, RSSI). |
| `"wifi.disconnected"` | `WifiEvents::WIFI_DISCONNECTED` | `WifiManager` | `nullptr` | Emitted when station connection drops. |
| `"wifi.ap_started"` | `WifiEvents::WIFI_AP_STARTED` | `WifiManager` | `const char*` | Emitted when soft-AP mode activates (AP SSID / hostname). |

---

## 11A. Logging & Observability Architecture

> 📖 **Developer Guide:** *Developer guide planned.*

### 11A.1 Design Principles

The framework standardises on **ESP-IDF's `esp_log.h`** as the single logging API.
No module invents custom macros. No module uses `Serial.print()` for logging.
The ESP-IDF logging system provides compile-time stripping, runtime per-module
verbosity control, thread safety (FreeRTOS), and pluggable backends — all of
which are lost when modules use ad-hoc alternatives.

### 11A.2 Unified Logging via `LogShim.h`

All modules include `platform/LogShim.h` instead of `esp_log.h` directly:

```cpp
// include/platform/LogShim.h
#pragma once

// --- Hardware build: delegate to the real ESP-IDF logging system ---
#include "esp_log.h"
```

- **Hardware Builds:** Delegates directly to ESP-IDF's logging subsystem, inheriting compile-time dead-code elimination (`CORE_DEBUG_LEVEL`), runtime per-module verbosity control (`esp_log_level_set`), thread-safe FreeRTOS primitives, and custom `esp_log_set_vprintf` handler hooks.
- **Native Unit Test Builds:** Following the WO1 cleanup, native builds resolve `esp_log.h` through clean test stubs located at `test/stubs/platform/esp_log.h` via PlatformIO native include path flags (`-Itest/stubs/platform`), eliminating `#ifndef NATIVE_BUILD` preprocessor clutter from production headers.

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

### 11A.4 LogPipeline & Global Redaction Architecture

The framework installs a custom `esp_log_set_vprintf()` handler via `LogPipeline::begin()` during `Application::begin()` (Step 1b). This handler intercepts all system and module logs:

```
ESP_LOGI(TAG, "...")   → esp_log_write()
  → LogPipeline (custom vprintf handler installed via esp_log_set_vprintf)
    → Step 1: thread_local re-entrancy guard — on re-entry, format into a
              STACK buffer (the shared buffer belongs to the outer call)
    → Step 2: Acquire the buffer mutex (bounded timeout; on timeout, degrade
              to a stack buffer — still redacted, never chained un-redacted)
    → Step 3: Format into the 256 B static .bss buffer; verbose builds only,
              grow a retained heap buffer if the line does not fit
    → Step 4: Redact in-place via the injected ISanitizer chain
    → Step 5: Write to the sink (Serial / stdout)
```

#### Thread Safety and Buffer Management
- **Production (`CONFIG_LOG_MAXIMUM_LEVEL <= 3`):** Uses a single, lock-free (if re-entered) 256 B static `.bss` buffer. Zero heap allocations occur on the logging path. Across 19,236 captured log lines, the longest production line was 144 B; 256 B provides ample headroom.
- **Debug/Verbose (`CONFIG_LOG_MAXIMUM_LEVEL >= 4`):** Supports opportunistic dynamic sizing. A dynamically growing (up to 16 KB) retained heap buffer accommodates large debug dumps. This branch utilizes `va_copy` and a FreeRTOS static mutex (`xSemaphoreCreateMutexStatic`) with a variable timeout derived from serial baud rate capacity `(bufSize * 300) / 1152`. If allocation fails or mutex times out, it degrades gracefully to a stack buffer.
- **Concurrency Protection:** The shared buffer is protected by a FreeRTOS mutex owned by `LogPipeline`. Under `CONFIG_LOG_VERSION=1` (the default for ESP-IDF boards), `esp_log_va()` invokes the registered print hook with no lock held; the `LogPipeline` mutex prevents cross-task corruption while the `thread_local` guard handles same-task recursion.

#### Sanitizer Components: `Redactor`, `StructuralScrubber`, and `CompositeSanitizer`
- **`Redactor` Singleton:** Maintains a bounded array of registered secrets (`MAX_SECRETS = 16`, `MAX_SECRET_LEN = 192`) in fixed `.bss` storage (`char[16][192]`). Registration executes an insertion sort to maintain descending string lengths, preventing partial masking of nested secrets (e.g. masking "password" before "pass"). `findBounded()` safely traverses non-NUL-terminated buffers without out-of-bounds reads.
- **`StructuralScrubber`:** Second-pass structural interceptor for un-registered secrets and structured spillage (HTTP headers, JSON fields). Scans for keys like `Authorization`, `Cookie`, `"password"`, `"token"`, `"api_key"` and masks their values using zero-allocation C-string scanning.
- **`CompositeSanitizer`:** Implements `ISanitizer` to chain multiple sanitizers together. Registered in `Application.cpp`, chaining `Redactor` globally and adding `StructuralScrubber` for verbose builds (`CONFIG_LOG_MAXIMUM_LEVEL >= 4`).

#### Payload Dumping API (`dumpPayload`)

For large binary or structured payloads (such as raw HTTP request/response bodies) where standard `ESP_LOGV` would truncate or risk stack exhaustion, use:

```cpp
LogPipeline::dumpPayload(esp_log_level_t level, const char* tag, const void* data, size_t length);
```

This API chunks the payload into manageable segments using an overlapping sliding window (`MAX_SECRET_LEN - 1` overlap), redacts each chunk independently, and writes directly to the output sink, ensuring secrets spanning chunk boundaries are never leaked.

> [!IMPORTANT]
> **Sink-level redaction is a backstop, not a guarantee.** It cannot cover:
> - **`ESP_EARLY_LOG*` / `ESP_DRAM_LOG*`** — these use `ets_printf` and bypass the hook entirely, so early-boot and ISR logs are not redacted.
> - **Raw `printf()`** — not an ESP-IDF log call, so it never reaches the hook.
> - **Encoded payloads** — hex or base64 output never matches a plaintext needle.
> - **Secrets split across separate log calls** — redaction is per-call, so a secret spanning two separate `ESP_LOG*` invocations cannot be matched across the boundary.
> - **Secret length** — masking with asterisks preserves string length, which discloses secret length.

### 11A.5 `ILogSink` Interface *(Deferred)*

> [!NOTE]
> **Status: DEFERRED to a future release (Issue #211)**
> Pluggable log sinks (`ILogSink`) will be introduced when consumer sinks exist
> (e.g., in-memory ring buffer, MCP log tools, or web portal log viewer). For now,
> `LogPipeline` outputs directly to the serial console (`vprintf`).

```cpp
class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void write(const char* formattedLine, size_t len) = 0;
};
```

When implemented, registration will be via `app.registerLogSink(ILogSink*)`. The LogPipeline
will dispatch formatted, redacted log lines to all registered sinks after writing to Serial.
This follows the same Hollywood Principle pattern as `IWebContributor` and
`IMCPContributor`.

### 11A.6 RAM Ring Buffer & Crash Persistence *(Deferred)*

> [!NOTE]
> **Status: DEFERRED to a future release (Issue #211)**
> Developer workflow currently relies on the serial monitor and local capture. A RAM ring
> buffer consumes memory (~4–8 KB) that is lost on hard crash without panic dumping. Ring buffer
> support and crash dump persistence will be added in a follow-up PR alongside MCP log tooling.

The proposed architecture will maintain a **RAM ring buffer** (~4-8KB) that contains
the most recent log output for:

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

The `Application::tick()` loop emits a periodic heartbeat log every **~30 seconds** containing:
- Free heap (`heap_caps_get_free_size(MALLOC_CAP_8BIT)`)
- Internal DRAM (`heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)`)
- Minimum internal DRAM watermark (`heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)`)
- Uptime in seconds
- **No RSSI** — on PSRAM boards, `free_heap` is dominated by PSRAM and can report megabytes free while internal DRAM is near exhaustion. `dram_min` tracks the lifetime low-water mark since boot.

```
♥ uptime=120s  free_heap=4238120B  dram=214880B  dram_min=198420B
```

This provides continuous runtime observability without requiring any external monitoring infrastructure — just a serial monitor connection.

### 11A.8 Optional MQTT Telemetry Module

An optional module (`mqttTelemetry`) can be added to any application for remote
log forwarding. It implements `ILogSink` and:

- Reads from the LogPipeline's RAM ring buffer (when ring buffer is implemented)
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

> 📖 **Developer Guide:** [Build System](developer/build-system.md) — PlatformIO environments, version injection, partitions, and LittleFS.

### 12.1 Repository Topology

> 📖 See [Build System § PlatformIO Environments](developer/build-system.md#1-platformio-environments) for the project structure walkthrough.

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
    interfaces/                 ← Pure abstract interfaces (ITickable, ISanitizer, etc.)
    platform/                   ← Platform abstraction headers
      LogShim.h                 ← Unified ESP-IDF ↔ native logging bridge
    portal_assets/              ← Generated PROGMEM gzip headers (build output)
    AppFoundation.h             ← Canonical single-include façade
    AppState.hpp                ← BOOTING, WIFI_SETUP, APP_SETUP, RUNNING
    FrameworkConfig.hpp         ← Kernel config struct
  lib/                          ← Low-level utilities
    deviceCrypto/
    LogPipeline/
    md5Utils/
    Redactor/
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

> 📖 See [Build System § The Release Pipeline](developer/build-system.md#7-the-release-pipeline) for script usage in the release workflow.

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

> 📖 **Developer Guide:** [Security](developer/security.md) — threat model, hardware encryption, credential storage, and TLS tradeoffs.

### 13.1 DeviceCrypto

> 📖 See [Security § Hardware Encryption (DeviceCrypto)](developer/security.md#3-hardware-encryption-devicecrypto) for the API and usage rules.

The framework includes a hardware-backed cryptographic engine (`DeviceCrypto`)
that derives a unique device key from the ESP32's hardware MAC/eFuse and uses it
for symmetric encryption of sensitive configuration values (API tokens, passwords).

- Keys are **never stored in plaintext** on LittleFS
- Encrypted values use AES-256 with the device-unique key
- The device key is non-exportable — encrypted config is device-specific
- All cryptographic operations use **ESP-IDF/MbedTLS** hardware-accelerated APIs
  (not standard C++ library crypto)

### 13.2 KeyManager & API Key Registry

> 📖 See [Security § Credential Storage (KeyManager)](developer/security.md#4-credential-storage-keymanager) for the CRUD API and storage architecture.

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

### 13.3 OTA Production System

> 📖 **Developer Guide:** [OTA Update System](developer/ota-update-system.md) — complete OTA lifecycle, state machine, release channels, and deployment procedures.

The framework provides a complete, production-grade OTA firmware update pipeline:
semantic version comparison → channel-aware release discovery → multi-architecture
manifest resolution → binary download → cryptographic signature verification →
flash → self-test → firmware blessing → config snapshot. The system is designed
with defence-in-depth: fail-closed signature enforcement, task watchdog supervision,
auto-rollback via the ESP-IDF bootloader, and application-layer anti-rollback via
an NVS security version counter.

#### 13.3.1 Architecture Overview

The OTA subsystem is composed of five libraries and one orchestrator module:

| Layer | Location | Responsibility |
|-------|----------|----------------|
| **`OTAUpdateManager`** | `modules/otaUpdateManager/` | Orchestrator — state machine, FreeRTOS task, EventBus progress, REST API routes, IConfigurable channel/autoCheck persistence |
| **`OTADownloadEngine`** | `lib/OTADownloadEngine/` | Utility — redirect-aware HTTP download, binary header verification, chunked streaming flash, verifier integration |
| **`IOTAVerifier` / `MbedTLSSignatureVerifier`** | `include/interfaces/` + `lib/OTADownloadEngine/` | Interface + default impl — RSA+SHA256 signature verification via mbedtls |
| **`SemVer`** | `lib/SemVer/` | Firmware version parsing, packed 32-bit comparison, channel-aware ordering |
| **`OTAManifest`** | `lib/OTAManifest/` | Multi-architecture manifest parser — maps board IDs to firmware assets |
| **`ConfigManifest`** | `lib/ConfigManifest/` | Three-Tier config lifecycle — Active/Previous/Golden schema version tracking |

The layers follow the Hollywood Principle: `OTAUpdateManager` (a module)
depends on the `lib/` libraries, but the libraries have no knowledge of the
module. The download engine accepts a `WiFiClient&` from the caller and never
creates its own network client.

```
SemVer → OTAManifest → OTAUpdateManager → OTADownloadEngine → IOTAVerifier
                              ↕
                      ConfigManifest ↔ ConfigManager
                              ↕
                      GithubClient (release queries)
```

#### 13.3.2 SemVer Engine

The `SemVer` library (`lib/SemVer/`) provides zero-allocation firmware version
comparison on MCU using packed 32-bit integers.

**`FirmwareVersion` struct:**
```cpp
struct FirmwareVersion {
    uint8_t major   = 0;
    uint8_t minor   = 0;
    uint8_t patch   = 0;
    Channel channel = Channel::STABLE;
    uint8_t preNum  = 0;
};
```

**`Channel` enum** — numeric values produce correct ordering via `packVersion()`:

| Channel | Value | Example Tag |
|---------|-------|-------------|
| `DEV` | 0 | `v3.1.0-dev.3` |
| `BETA` | 2 | `v3.1.0-beta.1` |
| `RC` | 3 | `v3.1.0-rc.1` |
| `STABLE` | 15 | `v3.1.0` |

**`packVersion()`** — bit layout: `MAJOR(8) | MINOR(8) | PATCH(8) | CHANNEL(4) | PRE_NUM(4)`.
A single 32-bit integer comparison replaces multi-field comparison logic.

**`parseVersionTag()`** — parses git tag strings (`"v3.1.0"`, `"v3.1.0-beta.1"`,
`"3.1.0-dev.3"`) into `FirmwareVersion` using `strtoul()` and `strncmp()` — zero
heap allocation. Leading `v` prefix is optional.

**Channel ordering:** `DEV < BETA < RC < STABLE`. A stable release always
sorts higher than any pre-release of the same major.minor.patch.

#### 13.3.3 Release Channels

The OTA system supports three release channels for firmware distribution:

| Channel | Description | Tag Convention |
|---------|-------------|----------------|
| `stable` | Production releases | `v3.1.0` (no pre-release suffix) |
| `beta` | Pre-release testing | `v3.2.0-beta.1` |
| `dev` | Development builds | `v3.2.0-dev.3` |

**Channel persistence:** The active channel is stored as an `IConfigurable`
field in `/mod/ota.json` (namespace: `"ota"`, schema version: 2). Fields:

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `channel` | `char[16]` | `"stable"` | Active release channel |
| `autoCheck` | `bool` | `true` | Whether to auto-check for updates on boot |
| `lastCheckedChannel` | `char[16]` | `""` | Channel used for the previous update check |

**Implicit downgrade:** When the user changes channel (e.g., `beta` → `stable`),
`checkForUpdate()` detects that `_channel != _lastCheckedChannel` and signals
"update available" even if the remote version is numerically lower than the
running version. This allows seamless channel switching without manual intervention.

**`lastCheckedChannel` tracking:** Updated after each successful check. Enables
detection of channel changes between checks. Persisted via `ConfigManager`.

**`GithubClient::getLatestReleaseForChannel()`:** For `"stable"`, delegates to
the standard `getLatestRelease()` endpoint. For `"beta"` or `"dev"`, queries
`/repos/{owner}/{repo}/releases` (all releases including pre-releases) and finds
the first release whose tag contains the channel string.

#### 13.3.4 Multi-Architecture Manifest

The OTA system supports multiple board architectures via a `manifest.json` file
shipped as a release asset alongside firmware binaries.

**Manifest schema (`OTAManifest`):**
```json
{
  "version": "v3.1.0",
  "channel": "stable",
  "securityVersion": 1,
  "assets": [
    {
      "board": "esp32dev",
      "firmware": "firmware-esp32dev.bin",
      "signature": "firmware-esp32dev.bin.sig",
      "size": 1048576,
      "sha256": "abc123..."
    },
    {
      "board": "esp32s3nano",
      "firmware": "firmware-esp32s3nano.bin",
      "signature": "firmware-esp32s3nano.bin.sig",
      "size": 1097152,
      "sha256": "def456..."
    }
  ]
}
```

**`OTAManifest` struct** (`lib/OTAManifest/`):
- `version[32]` — release version tag
- `channel[16]` — release channel
- `securityVersion` — anti-rollback counter
- `assets[kMaxBoards]` — array of per-board `OTAAsset` entries (max 4 boards)
- All storage is stack-allocated using fixed-size char arrays — no heap, no String

**`OTAAsset` struct** — per-board firmware asset descriptor:
- `board[24]` — board identifier (e.g., `"esp32dev"`)
- `firmware[64]` — firmware binary filename
- `signature[64]` — signature filename
- `size` — binary size in bytes
- `sha256[65]` — SHA-256 hex digest

**`BOARD_ID` compile-time injection:** Each PlatformIO environment defines
`-DBOARD_ID=\"esp32dev\"` (or `\"esp32s3nano\"`, `\"native\"`) in `build_flags`.
The orchestrator uses this macro to call `findAssetForBoard(manifest, BOARD_ID)`
for board-specific asset resolution.

**Legacy fallback:** If no `manifest.json` is present in a release (pre-manifest
releases), the orchestrator falls back to a linear scan of release assets,
matching by `.bin` and `.sig` file extensions. This ensures backward compatibility
with older releases.

**`parseManifest()` / `findAssetForBoard()`** — free functions: parse raw JSON
into the struct and perform a linear scan for the board-matching asset.

#### 13.3.5 Cryptographic Verification Pipeline

The `IOTAVerifier` interface defines a three-step verification contract:

```cpp
class IOTAVerifier {
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

#### 13.3.6 Fail-Closed OTA Enforcement

When `FrameworkConfig.otaPublicKey` is configured (non-null), the OTA pipeline
operates in **fail-closed mode**:

- If the release has **no `.sig` asset**, the update is **aborted** with:
  `"Missing .sig file — update rejected (fail-closed)"`
- If the `.sig` file downloads but contains **zero bytes**, the update is
  **aborted** with: `"Signature empty — update rejected (fail-closed)"`
- If signature verification **fails**, the download engine returns
  `OTAError::SignatureVerifyFailed`

This is a **security invariant**: when a public key is present, there is no
code path that accepts unsigned firmware. The check occurs at two levels:

1. **Asset presence check** — before any download begins (fast rejection)
2. **Post-download verification** — after the full binary is written and hashed

**Degraded mode (no key):** If `FrameworkConfig.otaPublicKey` is `nullptr`,
signature verification is skipped entirely. A warning is logged at `ESP_LOGW`.
This is a dev convenience; production deployments must configure a public key.

#### 13.3.7 Download Engine

`OTADownloadEngine` handles the complete binary download pipeline:

1. **Redirect-following HTTP GET** — up to 5 hops (`kMaxRedirects`), using
   HTTP/1.0 to avoid chunked-transfer encoding complications on the ESP32
2. **Separate `.sig` file fetch** — `fetchSignature()` downloads the detached
   signature file with its own redirect chain
3. **ESP32 binary header validation** — verifies the first byte is the ESP32
   magic byte `0xE9`, rejecting non-firmware payloads before any flash write
4. **Partition space validation** — checks `Content-Length` against
   `ESP.getFreeSketchSpace()`, failing early with `OTAError::InsufficientSpace`
   if the binary exceeds the available OTA partition
5. **Chunked streaming write** — reads the binary in 1024-byte chunks
   (`kChunkSize`), writing each chunk to the flash via `Update.write()` and
   feeding it to `IOTAVerifier::update()` for hash accumulation
6. **Post-download signature verification** — calls `IOTAVerifier::verify()`
   against the fetched `.sig` file contents
7. **Progress callbacks** — invokes `OTAProgressCallback(bytesWritten, totalSize)`
   after each chunk for UI integration

**Error codes** (`OTAError` enum):

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

#### 13.3.8 Orchestrator State Machine

`OTAUpdateManager` manages the OTA lifecycle through the `OTAState` enum:

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
`FrameworkConfig`, queries the latest release for the configured `_channel`,
compares versions using `SemVer`, and returns `true` if a new release is
available or if the channel has changed since the last check (implicit downgrade).
Guards: must be in `Idle` state with a configured `otaGithubRepo`.

**`performUpdate()`** launches a FreeRTOS task (8KB stack, priority 1) that
executes the full pipeline:

1. Guard checks (firmware updates enabled, GitHub repo configured)
2. Query GitHub for releases via `GithubClient::getLatestReleaseForChannel()`
3. Download and parse `manifest.json` (or fall back to legacy asset scan)
4. Match `BOARD_ID` to manifest assets via `findAssetForBoard()`
5. Fail-closed check: reject missing/empty `.sig` when public key is configured
6. Snapshot config manifest: Active → Previous (via `ConfigManifest`)
7. Initialise `MbedTLSSignatureVerifier` (if `otaPublicKey` configured)
8. Allocate `WiFiClientSecure` on the heap (per §14.3)
9. Create and configure `OTADownloadEngine`
10. Fetch the `.sig` signature file
11. Download and flash the firmware binary (with per-chunk TWDT feeding)
12. Finalise and reboot (2-second delay for event propagation)

The FreeRTOS task ensures the `tick() ≤ 10ms` constraint is never violated by
blocking download I/O. On native builds, `performUpdate()` sets `OTAState::Error`
(no FreeRTOS available).

#### 13.3.9 Task Watchdog Supervision

The OTA FreeRTOS task subscribes to the ESP-IDF Task Watchdog Timer (TWDT):

```cpp
esp_task_wdt_add(NULL);   // Subscribe at task start
// ... download chunks, feed watchdog implicitly via chunk processing ...
esp_task_wdt_delete(NULL); // Unsubscribe before reboot or error return
```

**`CONFIG_ESP_TASK_WDT_PANIC=y`** in `sdkconfig.defaults` means a TWDT timeout
triggers a kernel panic → reboot. If the download hangs (network timeout,
infinite redirect loop), the watchdog fires → panic → reboot → bootloader auto-
rollback reverts to the previous firmware.

This creates a closed safety loop: hung download → watchdog → panic → reboot →
auto-rollback — with no human intervention required.

#### 13.3.10 Auto-Rollback

The ESP-IDF bootloader provides hardware-level auto-rollback via
`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`:

1. After OTA flash, the device reboots into the new firmware in **pending
   verification** state
2. The **`Application` hub** calls `markAppValid()` at **Step 17b** of the boot
   sequence — after `_compactAndLock()` (Step 17). This means the self-test
   barrier covers ALL subsystem initialisation:
   - Display hardware bootstrap
   - EventBus, LittleFS mount, FrameworkConfig
   - DeviceCrypto, KeyManager
   - ConfigManager (register + beginAll)
   - WiFi, Time, Data, OTA, Web, MCP
   - Display integration, carousel board registration
   - Registry lock (`_compactAndLock`)
3. If any step crashes or hangs before `markAppValid()`, the bootloader
   automatically reverts to the previous firmware partition on the next reboot

> **Prohibited:** Blessing the partition at download/write time. This defeats the
> rollback purpose — firmware could still have fatal startup or runtime crashes.

#### 13.3.11 Anti-Rollback (Application Layer)

The application-layer anti-rollback mechanism uses an NVS security version counter
to prevent downgrade past security-critical firmware boundaries:

- **`SECURITY_VERSION` macro** — defined in `platformio.ini` `build_flags`
  (e.g., `-DSECURITY_VERSION=1`). Incremented when a release contains a
  security fix that must not be rolled back past.
- **NVS storage** — `Preferences` namespace `"ota_security"`, key `"sec_ver"`
- **Check logic** (`checkSecurityVersion()`, called from `markAppValid()`):
  - If `SECURITY_VERSION > stored` → update NVS to the new value
  - If `SECURITY_VERSION < stored` → firmware is running a version below the
    security floor (this scenario is prevented by the orchestrator checking the
    manifest's `securityVersion` field before download)

The `OTAManifest.securityVersion` field enables the orchestrator to compare the
remote release's security version against the stored NVS value before initiating
a download, rejecting downgrades at the network layer before any flash write.

#### 13.3.12 Config Lifecycle — Three-Tier Model

The `ConfigManifest` library (`lib/ConfigManifest/`) manages configuration
versioning across OTA updates using three tiers:

| Tier | Description | Snapshot Trigger |
|------|-------------|-----------------|
| **Active** | Currently running config schema version per module | Updated by `ConfigManager` during `beginAll()` |
| **Previous** | Config state before the last OTA download | `snapshotActiveToPrevious()` — called before OTA download |
| **Golden** | Last-known-good config (firmware blessed) | `snapshotActiveToGolden()` — called from `markAppValid()` |

**Persistence:** Stored as `/config_manifest.json` on LittleFS, alongside the
per-module config files at `/mod/<namespace>.json`.

**OTA integration points:**
1. Before download: `_configManager.getManifest().snapshotActiveToPrevious()` +
   `save()` — preserves the pre-update config state
2. After firmware blessing: `_configManager.getManifest().snapshotActiveToGolden()`
   + `save()` — records the last-known-good configuration

**`ModuleSlot` struct** — per-module slot tracking Active/Previous/Golden schema
versions. Up to `kMaxModules` (16) namespaces are tracked.

**`ConfigManifest` API:**
- `load()` / `save()` — JSON persistence on LittleFS
- `updateActive()` / `updatePrevious()` / `updateGolden()` — per-namespace setters
- `snapshotActiveToPrevious()` / `snapshotActiveToGolden()` — bulk copy all slots
- `getSlot()` / `getActiveVersion()` / `getPreviousVersion()` / `getGoldenVersion()`

#### 13.3.13 FrameworkConfig OTA Secrets

`FrameworkConfig` carries only compile-time OTA secrets. The runtime master
kill-switch moved to OTAUpdateManager's persisted config as `updatesEnabled`
(see §13.3.14); `otaQuietHour` was removed (never implemented — daily
scheduling is handled by `checkHour`).

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `otaPublicKey` | `const char*` | `nullptr` | PEM-encoded RSA public key for signature verification. `nullptr` = skip verification (degraded mode) |
| `otaGithubRepo` | `const char*` | `nullptr` | GitHub repository in `"owner/repo"` format for OTA releases |
| `otaGithubToken` | `const char*` | `nullptr` | Optional GitHub access token for private repos |

These secrets have safe defaults (`nullptr`) so the framework boots without
requiring any OTA configuration. The consuming app populates them during the
`BOOTING` phase via `FrameworkConfig`.

#### 13.3.14 IConfigurable OTA Fields

`OTAUpdateManager` implements `IConfigurable` (namespace: `"ota"`, schema v4):

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `channel` | `char[16]` | `"stable"` | Active release channel (`"stable"`, `"beta"`, `"dev"`) |
| `autoCheck` | `bool` | `true` | Auto-check for updates on boot |
| `updatesEnabled` | `bool` | `true` | Master kill-switch for applying firmware updates (moved from `FrameworkConfig`) |
| `checkHour` | `uint8_t` | `3` | Hour (0–23) for the daily scheduled update check |
| `lastCheckedChannel` | `char[16]` | `""` | Last channel used for an update check (implicit downgrade detection) |

Persisted at `/mod/ota.json` via `ConfigManager`. Schema migrations: v1→v2 adds
`lastCheckedChannel`; v2→v3 adds `checkHour`; v3→v4 adds `updatesEnabled`
(defaults to `true`).

#### 13.3.15 EventBus Integration

The orchestrator publishes `OTAProgressEvent` on topic `"ota.progress"`:

```cpp
struct OTAProgressEvent {
    float progress;       // 0.0 to 1.0 download fraction
    const char* status;   // Human-readable status message
    bool complete;        // True when OTA cycle is finished
    bool success;         // True if OTA succeeded (only valid when complete)
};
```

`OTAProgressEvent` is **layout-compatible** with `OTAProgressPayload` in
`U8G2DisplayModule.hpp` — the display module subscribes to `"ota.progress"` and
can safely cast the payload to either type for rendering the `OTAProgressBoard`.

Events are published:
- At each major pipeline step (checking, downloading, verifying, complete, error)
- On every progress callback from the download engine (per-chunk granularity)
- With `complete=true` on both success and failure (check `success` to distinguish)

#### 13.3.16 CI/CD Pipeline

Firmware is built and released via a two-stage pipeline:

**Stage 1 — GitHub Actions CI** (`.github/workflows/build.yml`):
- Triggers on tag push (`v*`)
- Matrix build: `esp32dev` + `esp32s3nano`
- Each job: PlatformIO build → rename binary → generate SHA-256 hash → upload
  unsigned artifacts
- Artifacts are named `firmware-{board}.bin` + `firmware-{board}.bin.sha256`

**Stage 2 — Local release** (`scripts/deploy_release.py`):
1. Download CI artifacts: `gh run download <run-id>`
2. Sign each board's firmware: `openssl dgst -sha256 -sign private_key.pem`
3. Generate `manifest.json`: `scripts/generate_manifest.py`
4. Create GitHub Release: `gh release create <tag>`
5. Upload all assets: `.bin`, `.bin.sig`, `.bin.sha256`, `manifest.json`

**Design rationale:** Signing is performed locally (not in CI) because the
private key must never be stored in GitHub Secrets or any remote system. The
CI pipeline produces unsigned artifacts; the human operator signs them with the
locally-held private key before publishing the release.

#### 13.3.17 REST API

The `OTAUpdateManager` registers the following routes via `IWebContributor`:

| Method | Route | Body | Description |
|--------|-------|------|-------------|
| `GET` | `/api/ota` | — | Returns `{ "status": "ready" }` |
| `POST` | `/api/ota` | `{ "action": "checkGithub" }` | Check GitHub + perform update if available |
| `POST` | `/api/ota` | `{ "action": "status" }` | Returns state, version, channel, boardId, autoCheck, lastError |
| `POST` | `/api/ota` | `{ "action": "setChannel", "channel": "beta" }` | Change update channel (persisted via ConfigManager) |
| `POST` | `/api/ota` | `{ "action": "rollback", "confirm": "ROLLBACK" }` | Rollback firmware (requires typed confirmation) |
| `POST` | `/api/ota` | `{ "action": "markValid" }` | Bless the running firmware |

**Rollback confirmation:** The `rollback` action requires `"confirm": "ROLLBACK"`
in the POST body. Without this exact string, the server responds with HTTP 403
and an error message. This prevents accidental rollbacks from automated tools.

**Status response** includes: `state`, `version` (major.minor.patch), `channel`,
`boardId`, `autoCheck`, and `lastError` (if any).

#### 13.3.18 Web Portal Integration

The OTA tab in the web portal (parent: "System", order: 50) provides:

- **Channel dropdown** — allows switching between `stable`, `beta`, and `dev`
  channels. Calls `POST /api/ota { "action": "setChannel" }` on change.
- **Check for update** button — triggers `POST /api/ota { "action": "checkGithub" }`
- **Rollback** button — requires the user to type "ROLLBACK" in a confirmation
  dialog before sending `POST /api/ota { "action": "rollback", "confirm": "ROLLBACK" }`
- **Enhanced status display** — shows current firmware version, active channel,
  board ID, and OTA state

#### 13.3.19 Security Considerations

- **Public key management:** The consuming app provides the PEM-encoded RSA
  public key via `FrameworkConfig.otaPublicKey`. The framework never hardcodes
  key material — this is an app-side responsibility
- **Fail-closed enforcement:** When a public key is configured, missing or empty
  `.sig` files cause an immediate abort — not a warning (see §13.3.6)
- **WiFiClientSecure allocation:** Always heap-allocated via
  `std::unique_ptr<WiFiClientSecure>(new (std::nothrow) WiFiClientSecure())`
  per §14.3 memory rules — never on the stack
- **GitHub CDN TLS:** Uses `setInsecure()` for CDN redirect hops because GitHub's
  CDN endpoints use different certificate chains than `api.github.com`. The
  mandatory signature verification (not TLS certificate pinning) is the security
  boundary — GitHub CDN uses rotating certs that cannot be reliably pinned
- **Anti-rollback:** NVS `security_version` counter prevents downgrade past
  security-critical firmware (see §13.3.11)
- **Task watchdog:** Hung downloads are terminated by TWDT → panic → reboot →
  auto-rollback (see §13.3.9)
- **Rollback safety:** The blessing ceremony (§13.3.10) ensures that bad firmware
  is automatically reverted by the bootloader if core services fail to initialise
- **Legacy direct-upload path:** The web-upload OTA route
  (`beginUpdate`/`writeStream`/`endUpdate`) is preserved alongside the new
  GitHub pipeline. It uses MD5 verification via `Update.setMD5()` — this path
  does not use RSA+SHA256 signature verification

### 13.4 Web Portal Security

> 📖 See [Security § Web Portal & MCP Authentication](developer/security.md#6-web-portal--mcp-authentication) for the auth middleware flow.

- The portal is only accessible on the local network (no cloud dependency)
- No authentication is implemented by default (local network trust model)
- Future: optional HTTP Basic Auth configurable via `FrameworkConfig`

### 13.5 MCP Security

> 📖 See [Security § Web Portal & MCP Authentication](developer/security.md#6-web-portal--mcp-authentication) and [MCP Server § Configuration Persistence](developer/mcp-server.md#7-configuration-persistence) for read-only mode details.

- `FrameworkConfig.mcpReadOnly` toggle (default `false`)
- Each tool in `McpToolEntry` has a `bool mutatesState` metadata field (default `false`)
- When `mcpReadOnly == true`, tools with `mutatesState == true` return JSON-RPC
  error code `-32000` ("Read-only mode")
- HTTP transport (`POST /api/mcp`) inherits the same security checks as Serial
  transport — both call the same `processRequest()` core

---

## 14. Resource Budget & Constraints

> 📖 **Developer Guides:**
> - [Performance & Optimization](developer/performance-guide.md) — heap budgeting, PSRAM, flash size, TLS memory costs
> - [Embedded Safety](developer/embedded-safety.md) — memory rules, thread safety, DRAM constraints

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

> 📖 See [Performance & Optimization § RAM Budgeting](developer/performance-guide.md#2-ram-budgeting) for budgeting targets and measurement.

The kernel (without any optional modules) should target:

| Resource | Budget | Notes |
|---|---|---|
| Flash (binary) | < 600KB | Leaves room for app + LittleFS partition |
| Static RAM | < 40KB | Globals, BSS, static arrays |
| Heap at boot | > 120KB free | Required for WiFi stack + async web server |

### 14.3 Key Memory Rules

> 📖 See [Embedded Safety § Memory Rules](developer/embedded-safety.md#1-memory-rules) for the full rule set with examples.

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

> 📖 See [Embedded Safety § Memory Rules](developer/embedded-safety.md#1-memory-rules) and [Architecture Overview § Two-Tier Vector Model](developer/architecture-overview.md#two-tier-vector-model--_compactandlock-lifecycle) for the allocation discipline.

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

> 📖 **Developer Guides:**
> - [Creating a Module](developer/creating-a-module.md) — step-by-step module implementation tutorial
> - [Coding Patterns](developer/coding-patterns.md) — SOLID principles, Service Module Pattern, style guide

### 15.1 Creating a New Module

> 📖 See [Creating a Module](developer/creating-a-module.md) for the complete step-by-step tutorial.

A minimal module that participates in the framework tick loop:

**`myModule.hpp`**
```cpp
#pragma once
#include <AppFoundation.h>  // Canonical single-include façade

class MyModule : public ITickable, public IConfigurable, public IWebContributor {
public:
    void begin(Application* ctx);
    void tick(uint32_t ms) override;
    void reapplyConfig(const FrameworkConfig& cfg) override;
    void registerRoutes(PsychicHttpServer& server) override;
    void registerTabs(ITabRegistry& registry) override {
        registry.addTab("My Module", nullptr, 100);
    }
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

> 📖 See [Configuration System § Config File Ownership Model](developer/config-system.md#1-config-file-ownership-model) for the directory and naming conventions.

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

> 📖 See [Creating a Module § Developer Checklist & Best Practices](developer/creating-a-module.md#5-developer-checklist--best-practices) for the interactive checklist.

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

> 📖 **Developer Guide:** [Build System](developer/build-system.md) — testing tiers, native tests, and CI integration.

### 16.1 Three Testing Tiers

| Tier | Runs Where | Purpose |
|---|---|---|
| **Tier 1 — Native Unit Tests** | Host CPU via `[env:native]` + GoogleTest | Logic, state machines, registration, scheduling |
| **Tier 2 — Hardware Integration** | Physical ESP32 board | WiFi, OTA, LittleFS, crypto, display |
| **Tier 3 — End-to-End Boot** | Physical board, full stack | Boot-to-`RUNNING` all services |

### 16.2 `NATIVE_BUILD` Guard Pattern

> 📖 See [Creating a Module § NATIVE_BUILD Compilation Guards](developer/creating-a-module.md#native_build-compilation-guards) and [Build System § Native Environment](developer/build-system.md#native-environment) for guard strategies.

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

> 📖 See [Build System § Building and Testing](developer/build-system.md#4-building-and-testing) for test execution and mock patterns.

- `MockDeviceCrypto` subclass for `encrypt()`/`decrypt()` virtual methods
- `AsyncWebServer` dummy class in `webServer.hpp` (canonical mock — never define
  inline duplicates)
- `AsyncWebServerResponse` stub with `beginResponse_P`, `addHeader`, `send`
- `DNSServer` dummy class in `wifiManager.hpp` (resolves incomplete type for
  `std::unique_ptr<DNSServer>`)
- `LogPipeline` / `LogShim` uses `PIO_UNIT_TESTING` stubs for native testing
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
| D28 | Global secret redaction via `esp_log_set_vprintf()` pipeline (`LogPipeline`) | `ESP_LOG*` output intercepted at the sink and redacted via an injected `ISanitizer` chain, under a `LogPipeline`-owned FreeRTOS mutex (ESP-IDF holds no lock across the callback at `CONFIG_LOG_VERSION=1`). 256 B static buffer; verbose builds add a retained heap buffer and `dumpPayload()`. Zero call-site churn; legacy `Logger` deleted. Backstop, not a guarantee — see §11A.4. (Issue #211) |
| D29 | RAM ring buffer for log capture *(Deferred)* | Deferred to follow-up PR (Issue #211); serial monitor + local capture suffices for current developer workflows. |
| D30 | `ILogSink` interface for pluggable log destinations *(Deferred)* | Deferred to follow-up PR (Issue #211) until ring buffer and remote sinks are implemented. |
| D31 | Heartbeat telemetry every ~15s in `Application::tick()` | Provides continuous runtime observability (heap, uptime, RSSI) with zero external infrastructure |
| D32 | MQTT telemetry as optional module, not kernel | Not all apps need remote logging; kernel stays minimal |
| D33 | ArduinoJson v7 `JsonDocument` replaces v6 `StaticJsonDocument`/`DynamicJsonDocument` | Unified API; v6 types are deprecated |
| D24 | `build_fonts.py`/`generate_layouts.py` as standalone CLI, not PIO hooks | Avoids breaking builds if input files are missing |
| D25 | `U8G2_DISPLAY_TYPE` macro for display controller selection | Overridable via `build_flags`; default SSD1306 128x64 |
| D26 | Dual framework: Arduino + ESP-IDF | OTA rollback, IDF logging, DeviceCrypto require IDF; display/web libs require Arduino |
| D34 | Display module redesigned as pure service: `Application*` removed from member variables | Display module was polling `AppState`, `WifiManager`, and `ConfigGates` inside `tick()` — business logic in a hardware presenter. Application layer now owns all "when to show what" decisions; display module owns only rendering, carousel timing, and screensaver inactivity policy |
| D35 | Hierarchical COM-style sub-object API for display module: `display.carousel`, `display.boards.*`, `display.screensaver` | Flat `getBootBoard()` / `getWifiWizard()` / etc. accessors exposed private concrete types with no hierarchy and required callers to three-step orchestrate (configure + take address + pushModal). Sub-objects with domain meaning are now navigable directly |
| D36 | Concrete board types at caller layer; `IDisplayBoard*` only at engine boundaries | Callers need board-specific APIs (`setProgress()`, `configure()`); abstracting to `IDisplayBoard&` destroys the value of the hierarchy. Interface is used only where polymorphism is genuinely needed: `showModal(IDisplayBoard*)`, `carousel.add(IDisplayBoard*)` |
| D37 | Screensaver is a distinct display-intrinsic policy layer (not a modal) | Pushing `ScreensaverBoard` via `showModal()` assigned it MODAL priority, allowing it to suppress OTA progress and wizard overlays. Screensaver is the lowest of three explicit rendering layers (Modal → Base/Carousel → Screensaver), driven by inactivity timer, not application decision |
| D38 | Board taxonomy split: system boards (internal), user-surfaced boards (in `display.boards`), display-intrinsic boards (in `display.screensaver`) | Not all boards should be externally configurable. `BootBoard` and `ConfiguringStatusBoard` are driven by Application state transitions; exposing them via getters invited callers to bypass the module's lifecycle management |
| D39 | Unified secret redaction via `Redactor` Singleton + `ISanitizer` DIP interface | `Redactor` promoted from static class to Meyer's Singleton with fixed-capacity lock-free **append-only** array (16 secrets, atomic count, writer spinlock; published slots are never reordered or rewritten, which is what makes the lock-free read path safe). Implements `ISanitizer` for DI into `AuditModule`. `LogPipeline` consumes Singleton directly via C callback. Replaces and deletes legacy `Logger` and `LOG_SECURE`. (Issue #211) |

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
v7 migration. Updated 2026-06-03 to add §7.8 (Display Module Object Model target
architecture) and D34–D38 (display module OO refactoring decisions).*
