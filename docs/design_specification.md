# ESP32 App Foundation — Design Specification

**Status:** v2.0  
**Version:** 2.0  
**Author:** CB Labs  
**Last Updated:** 2026-09-03

---

> [!NOTE]
> **This specification was rebuilt as v2.0 on 2026-09-03.** Sections marked ✅ have been rewritten
> against the delivered architecture. See the
> [design spec audit report](conversation://8b733dfa-645a-478e-b6cc-e08c50620730)
> for the full audit findings that motivated this rebuild.

| Section | Status | Notes |
|---|---|---|
| §1 Purpose & Vision | ✅ v2.0 | Rewritten against delivered architecture |
| §2 Design Principles | ✅ v2.0 | Stale identifiers cleaned |
| §3 System Architecture | ✅ v2.0 | Component diagram rewritten |
| §4 Framework Kernel | ✅ v2.0 | Kernel table: 18 components |
| §5 Module System | ✅ v2.0 | Module catalog: 18 modules |
| §6 Extension Points | ✅ v2.0 | 14 kernel interfaces + 3 module-local + 3 payloads |
| §7 Display Architecture | ✅ v2.0 | Dual-display, LVGL v9, DisplayServiceHelper |
| §8 Configuration | ✅ v2.0 | ConfigManager-as-Gateway, config-gate folded |
| §9 Application Lifecycle | ✅ v2.0 | 3-state machine, PreconditionEngine gate |
| §10 Web Portal | ✅ v2.0 | PsychicHttp, AuthManager integration |
| §11 Inter-Module Communication | ✅ v2.0 | EventBus esp_event, std::function policy |
| §11A Logging & Observability | ✅ v2.0 | Written 2026-09-01, updated 2026-09-03 |
| §12 Repository & Build | ✅ v2.0 | Directory structure, library.json |
| §13 Security | ✅ v2.0 | AuthManager, split-repo OTA, DeviceCrypto |
| §14 Resource Budget | ✅ v2.0 | Measurement procedures, not stale numbers |
| §15 Module Authoring | ✅ v2.0 | ConfigManager-as-Gateway, registerModule() |
| §16 Testing | ✅ v2.0 | Stub-header architecture, 37 test suites |
| §17 Decision Log | ✅ v2.0 | D40–D54 backfilled (15 new decisions) |

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

Embedded ESP32 development repeatedly requires the same foundational infrastructure: robust networking, configuration persistence, authenticated web administration, structured logging, health gating, audit logging, OTA firmware distribution, and display rendering pipelines.

Without a shared foundation, each new project either starts from scratch or inherits a fragile copy-paste of a previous project's internals. This creates divergent, hard-to-maintain codebases where security fixes, memory optimizations, and stability improvements must be manually backported across multiple repositories.

### 1.2 What This Framework Is

The **ESP32 App Foundation** is a production-ready **C++ application framework** for ESP32 microcontrollers built with PlatformIO (Arduino + ESP-IDF dual framework). It delivers a complete, reusable foundational infrastructure so developers never start a device project from scratch.

Key architectural pillars delivered by the framework include:

- **Deterministic Lifecycle & Boot Gating**: A structured 3-state application lifecycle (`Booting` → `Configuring` ↔ `Running`) governed by **`PreconditionEngine`**, which evaluates system prerequisites, gates normal execution, continuously monitors runtime health, and attributes failures with Cause and Severity to prevent wedged device states.
- **Dual Display Subsystems**: First-class support for both monochrome and high-resolution colour user interfaces:
  - **U8G2**: Low-overhead widget toolkit and rotating carousel engine for monochrome OLED and e-paper displays.
  - **LVGL v9**: Modern, touch-driven colour TFT display pipeline featuring observer-based data binding (`LV_USE_OBSERVER`), subject reactivity, and seamless integration with the LVGL Pro Editor XML workflow.
- **Hardened Web Administration**: High-performance PROGMEM Single Page Application (SPA) served by `PsychicHttpServer`, backed by **`AuthManager`** for robust portal authentication, secure session cookies, JWT bearer tokens, and captive portal provisioning.
- **Observability & Secret Scrubbing**: Structured **`LogPipeline`** coupled with **`Redactor`** (`ISanitizer`) for real-time in-flight scrubbing of Wi-Fi credentials, API keys, and sensitive tokens before writing to serial or remote log ring buffers.
- **Tamper-Evident Audit Trail**: **`AuditModule`** recording critical system events, boot transitions, auth attempts, and configuration mutations into a compact, flash-wear-mitigated, tamper-evident log (`BOARD_AUDIT_BYTES`).
- **AI Agent Integration**: Built-in **Model Context Protocol (MCP)** server enabling AI coding agents and automated tooling to inspect device state, query logs, audit configurations, and execute diagnostic tools directly over standard JSON-RPC / HTTP.
- **HomeAssistant & Smart Home Ecosystem**: Native MQTT integration with **Home Assistant** MQTT auto-discovery for automated sensor reporting, diagnostic entities, and remote device management.
- **Cryptographically Verified Split-Repo OTA**: Secure, two-stage over-the-air firmware updates with cryptographic payload verification, Ed25519 signature checks, anti-rollback protection (`SECURITY_VERSION`), and asset filesystem synchronization.
- **Extensible Module Model**: Defined extension points (`IConfigurable`, `ITickable`, `IDataSource`, `IWebContributor`, `IMCPContributor`, `IPreconditionContributor`) allowing domain modules to self-register via `registerModule()` without touching the kernel.

### 1.3 What This Framework Is Not

- **Not ESPHome**: This is a code-first C++ framework, not a YAML/config-driven device compiler. It targets bespoke, complex embedded applications — intricate state machines, custom communication protocols, high-framerate rendering pipelines, and advanced sensor fusion — that cannot be represented in declarative configuration files.
- **Not a Monolith**: The framework is modular. Modules like displays (U8G2 / LVGL), MQTT, Home Assistant, or custom hardware peripherals are optional. Applications compose only the components they require, keeping flash and RAM footprints strictly aligned with hardware capabilities.
- **Not a HAL or Bare-Metal RTOS**: The framework operates above the Arduino/ESP-IDF layer. FreeRTOS task scheduling, interrupts, and low-level peripheral drivers remain cleanly decoupled or managed by application concerns.

### 1.4 Primary Use Case

Building production-ready, bespoke embedded IoT devices on ESP32 that demand enterprise-grade reliability, secure network management, authenticated web interfaces, deep observability, and verifiable remote updates — while providing complete freedom to implement custom application logic and proprietary hardware interactions.

**Reference applications**:
- `departures-board`: Real-time public transit departure display (rail, tube, bus) leveraging U8G2/LVGL display pipelines, background data fetching, and web configuration.
- Custom IoT sensor nodes, touch-screen controllers, and automation gateways requiring Home Assistant auto-discovery, MCP agent diagnostics, and cryptographic OTA verification.

### 1.5 Goals

- **Zero-Scratch Project Scaffolding**: New apps instantiate from the `template/` skeleton and write only application-specific code, inheriting battle-tested infrastructure immediately.
- **Zero Kernel Changes**: Adding, configuring, or removing optional modules requires only changes to `platformio.ini` and app setup code via the module registration bus.
- **Upstream Maintainability**: Framework improvements and security fixes flow cleanly to all derived apps via git submodule SHA pinning.
- **Native Host Testability**: Complete test portability across target hardware (`esp32dev`, `esp32s3nano`) and native host CPU (`native` environment with GoogleTest) through stub headers and platform abstraction.

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
    virtual UpdateStatus updateData() = 0;             // Legacy signature (superseded by IDataSource::fetchUpdate)
    virtual bool isScrollFinished() { return true; }   // Legacy signature (replaced by canAdvance)
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
4. **Native Simulation:** On non-`ESP_PLATFORM` builds, call `_dataManager.processNext()` synchronously.
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

The framework is a **monorepo** — kernel, optional modules, hardware definitions,
adapters, UI projects, scripts, templates, and documentation all live in a single repository.

#### Architectural Rationale: Monorepo vs Multi-repo

The framework deliberately uses a monorepo topology rather than splitting into separate repositories (e.g., `esp32-app-foundation`, `esp-module-display`, `esp-module-mqtt`). This architectural decision solves several critical challenges inherent to embedded C++ development in PlatformIO:

1. **Avoiding the "Version Matrix of Hell":** Multi-repo architectures require tight version orchestration. If a kernel interface changes, all module repos must be updated. App developers easily end up with incompatible version combinations in their `lib_deps`. A monorepo guarantees that every commit is a fully compatible, API-synchronised snapshot.
2. **Rock-Solid OTA Reproducibility:** Pinning the monorepo via a single git submodule SHA ensures that the exact state of the kernel *and* all optional modules is recorded. If a deployed firmware crashes, the developer knows exactly what was compiled into it.
3. **Atomic Refactoring:** Sweeping architectural changes can be executed and tested atomically in a single PR, rather than requiring synchronized merges across multiple repositories.
4. **Linker Efficiency:** The monorepo does *not* cause firmware bloat. PlatformIO and the C++ linker only compile and link the modules explicitly `#include`'d in the application code. Unused modules take up zero bytes on the ESP32 flash.

*Decision:* Keep it as a Monorepo for `v1.x`/`v2.x`. Separating mature modules into third-party repositories prematurely creates unnecessary fragmentation and versioning friction.

```
esp32-app-foundation/           ← Monorepo root
  include/                      ← Public façade and contracts
    AppFoundation.h             ← Canonical single-include façade
    Audit.h                     ← Structured audit macros (AUDIT, AUDIT_SYNC)
    build_version.h             ← Compile-time version injection
    publicKey.hpp               ← Public key PEM definitions
    types/                      ← Concrete framework types (AppState, FrameworkConfig, PriorityTier)
    interfaces/                 ← Pure abstract interfaces (ITickable, IConfigurable, IDataSource, etc.)
    events/                     ← EventBus event payload structs
    platform/                   ← Platform abstraction headers (LogShim.h)
    portal_assets/              ← Generated PROGMEM gzip headers (build output)
    utils/                      ← Header-only utilities
  hardware/                     ← Pure declarative board definitions (pin maps, partitions, sdkconfigs)
    _common/                    ← Shared hardware definitions and base headers
    _TEMPLATE/                  ← Porting template for new hardware targets
    esp32dev/                   ← Espressif ESP-WROOM-32 / NodeMCU (board.h, partitions.csv)
    esp32s3nano/                ← Waveshare ESP32-S3-Nano (board.h, partitions.csv, sdkconfig.defaults)
    crowpanel-128-rotary/       ← Elecrow CrowPanel 1.28" Rotary Display (board.h, partitions.csv)
    native/                     ← Host CPU mock board header
  adapters/                     ← Concrete hardware adapters (Ports & Adapters pattern)
    lvgl/                       ← ILVGLHardwareAdapter implementations (e.g. CrowPanel128Adapter)
  uidesign/                     ← Declarative UI projects
    crowpanel-128-rotary/       ← LVGL Pro Editor XML project files, screens, components, subjects
  lib/                          ← Low-level foundation utilities and libraries
    ConfigManifest/             ← Three-tier config snapshot tracking (Active/Previous/Golden)
    DeepStackExecutor/          ← PSRAM-backed worker execution for stack-heavy callables
    DeviceCrypto/               ← AES-256-CBC hardware encryption with NVS master key
    DisplayServiceHelper/       ← Tick-safe, non-blocking display service bridge
    LogPipeline/                ← Global log redaction pipeline via esp_log_set_vprintf()
    MD5Utils/                   ← MD5 hash verification for firmware uploads
    OTACerts/                   ← Embedded X.509 certificates and verification helpers
    OTADownloadEngine/          ← Redirect-aware streaming HTTP OTA download engine
    OTAManifest/                ← Multi-architecture manifest parser and asset resolver
    PngWriter/                  ← Display frame snapshot and compression utilities
    Redactor/                   ← Lock-free Meyer's Singleton secret redaction engine
    SemVer/                     ← Zero-allocation packed 32-bit SemVer comparison
  modules/                      ← Framework modules (kernel and optional)
    Application/                ← Kernel: Application hub, lifecycle state machine, registerModule()
    AuditModule/                ← Optional: LittleFS JSONL security and operational audit trail
    AuthManager/                ← Kernel: Session cookies, EC P-256 HTTPS certs, MCP bearer tokens
    ButtonModule/               ← Optional: Debounced push-button input event dispatch
    ConfigManager/              ← Kernel: Gateway for LittleFS JSON config and schema migration
    DataManager/                ← Kernel: Background fetch scheduler with FetchContext & DeadlineToken
    EncoderModule/              ← Optional: Rotary encoder hardware input event dispatch
    EventBus/                   ← Kernel: FreeRTOS esp_event C++ publish-subscribe wrapper
    HomeAssistant/              ← Optional: Home Assistant MQTT discovery contributor
    KeyManager/                 ← Kernel: AES-256 encrypted credential store in /etc/keys.json
    LVGLDisplay/                ← Optional: Modern LVGL v9 graphical display module
    MCPServer/                  ← Kernel: Model Context Protocol JSON-RPC server and tool registry
    OTAUpdateManager/           ← Kernel: Split-repo OTA, manifest resolution, signature verification
    PreconditionEngine/         ← Kernel: Boot gating, dependency DAG, and service readiness engine
    TimeManager/                ← Kernel: SNTP time synchronization and POSIX timezone
    U8G2Display/                ← Optional: Monochrome OLED display and widget module
    WebServer/                  ← Kernel: PsychicHttp / PsychicHttpsServer REST API and SPA host
    WifiManager/                ← Kernel: WiFi station connection and captive portal AP
  scripts/                      ← SCons build hooks and standalone CLI tools
  sample-app/                   ← Reference application demonstrating full framework usage
  template-app/                 ← Blank starter template for downstream projects
  web/portal/                   ← Web portal Single Page Application source (HTML/CSS/JS)
  test/                         ← Unified test suite
    test_unit/                  ← GoogleTest native unit test suites (host CPU)
    test_hardware/              ← Physical hardware integration tests
    test_e2e/                   ← End-to-end boot-to-RUNNING integration tests
    stubs/platform/             ← Stub-header architecture mocking Arduino and ESP-IDF APIs
  platformio.ini                ← Multi-environment build configuration
  library.json                  ← PlatformIO library manifest for downstream consumption
  sdkconfig.defaults            ← Baseline ESP-IDF SDK configuration
```

### 12.2 How Apps Consume the Framework

Apps consume the framework either as a **git submodule** pinned to a commit SHA or as a PlatformIO library:

```bash
git submodule add https://github.com/matthewmcneill/esp32-app-foundation.git framework
git submodule update --init
```

Consuming application `platformio.ini`:
```ini
[env:esp32dev]
platform = espressif32
framework = arduino, espidf
lib_compat_mode = strict
lib_ldf_mode = deep
lib_extra_dirs =
    framework/modules
    framework/lib
    framework/adapters
lib_deps =
    framework/                  ; local submodule path — no registry, deterministic SHA
    bblanchon/ArduinoJson@^7.4.3
    hoeken/PsychicHttp@^3.0.0
    olikraus/U8g2@^2.36.18      ; conditional on WITH_DISPLAY_U8G2
extra_scripts =
    pre:framework/scripts/sync_sdkconfig.py
    pre:framework/scripts/portalBuilder.py
    pre:framework/scripts/inject_version.py
build_flags =
    -DAPP_FOUNDATION_VERSION=\"0.1.0\"
    -I framework/include
```

> **Warning:** `build_src_filter` does NOT work with ESP-IDF components (ESP-IDF relies on `CMakeLists.txt` for component discovery). Use `lib_extra_dirs` + explicit `lib_deps` instead.

### 12.3 Library Distribution via `library.json`

The framework is published as a PlatformIO library described by `library.json`. Downstream consumers who install the framework via PlatformIO's package manager or registry obtain a self-contained library package:

```json
{
  "name": "esp32-app-foundation",
  "version": "0.1.0",
  "description": "Reusable ESP32 application framework — Arduino + ESP-IDF dual-framework.",
  "export": {
    "include": ["include", "modules", "lib"],
    "exclude": ["src", "test", "docs", ".agents", "scripts", "web", "template-app", "sample-app"]
  },
  "build": {
    "flags": ["-I include"]
  },
  "dependencies": {
    "bblanchon/ArduinoJson": "^7.4.3",
    "hoeken/PsychicHttp": "^3.0.0",
    "olikraus/U8g2": "^2.36.18"
  },
  "frameworks": ["arduino", "espidf"],
  "platforms": ["espressif32"]
}
```

The export filter ensures that test fixtures, developer documentation, and portal build tools are excluded from downstream consumer distributions, keeping the package lean.

### 12.4 Local Development Workflow

When actively developing the framework alongside an application, the submodule or `lib_deps` path points directly to the local checkout. Push changes to the framework repository; update the submodule commit SHA in the consuming app when ready to adopt a new framework release.

### 12.5 Build Scripts

Build scripts reside under `scripts/` and operate either as automated SCons hooks in PlatformIO or as standalone CLI utilities:

| Script | Type | Purpose |
|---|---|---|
| `sync_sdkconfig.py` | PIO hook (`extra_scripts`) | Synchronizes board-specific `sdkconfig.defaults` into the build directory |
| `portalBuilder.py` | PIO hook (`extra_scripts`) | Bundles and minifies `web/portal/` assets → gzip headers in `include/portal_assets/` |
| `run_web_tests.py` | PIO hook (`extra_scripts`) | Executes frontend JavaScript/HTML unit tests before compiling firmware |
| `inject_version.py` | PIO hook (`extra_scripts`) | Extracts Git tag / commit metadata and generates `include/build_version.h` |
| `build_fonts.py` | Standalone CLI | Compiles BDF font files into C++ header structures for U8G2 displays |
| `generate_layouts.py` | Standalone CLI | Translates declarative screen layout definitions into C++ header constants |
| `deploy_release.py` | Standalone CLI | Releases signed firmware binaries with detached RSA signatures and SHA-256 digests |
| `generate_manifest.py` | Standalone CLI | Generates `manifest.json` describing multi-board firmware release assets |

> **Standalone vs PIO hook:** Heavy code generators such as `build_fonts.py` and `deploy_release.py` are deliberately NOT wired as compile hooks. This prevents build failures in downstream projects that lack BDF font sources or private signing keys.

### 12.6 CI Workflow

The framework provides automated continuous integration via `.github/workflows/`:

1. **Multi-environment compilation**: Builds `esp32dev`, `esp32s3nano`, and `crowpanel-128-rotary` firmware targets.
2. **Native test execution**: Executes the complete GoogleTest suite on host runners via `pio test -e native`.
3. **Include-graph linting**: Enforces strict separation ensuring kernel headers never import optional module headers.

### 12.7 PlatformIO LDF Constraints & Stub Architecture

PlatformIO's Library Dependency Finder (LDF) scans `#include` directives textually. To ensure deterministic resolution and prevent unwanted transitive library compilation:

- `lib_ldf_mode = deep` is mandated in `platformio.ini` across all hardware environments.
- `lib_compat_mode = strict` prevents PlatformIO from attempting to compile desktop libraries on target microcontrollers.
- **Stub-Header Architecture**: Native host tests resolve hardware dependencies via stub headers in `test/stubs/platform/`, completely eliminating the need for invasive preprocessor guards in production source files.
- `lib_ignore` selectively disables unused display drivers per target environment (e.g. ignoring `LVGLDisplay` on `esp32dev`, and ignoring `U8G2Display` on `crowpanel-128-rotary`).

---

## 13. Security Architecture

> 📖 **Developer Guide:** [Security](developer/security.md) — threat model, hardware encryption, credential storage, TLS tradeoffs, web portal authentication, and OTA firmware integrity.

### 13.1 Threat Model & Layered Security

The framework assumes devices operate on a local WiFi network, potentially shared with untrusted or compromised devices. The primary threats addressed are:
1. **Unauthorized network access** to configuration, system controls, and device APIs.
2. **Firmware tampering and supply-chain corruption** during over-the-air updates.
3. **Credential extraction** from flash storage or network transit.
4. **Accidental system state mutation** via automated or misconfigured tooling.

The framework enforces defense-in-depth through five layered security controls:

| Layer | Component | Mechanism |
|---|---|---|
| **Hardware Encryption** | `DeviceCrypto` | AES-256-CBC encryption using a hardware RNG master key in NVS |
| **Credential Storage** | `KeyManager` | Encrypted key registry in `/etc/keys.json` with ownership metadata |
| **Identity & Authentication** | `AuthManager` | Session cookies, SHA-256 salted hashes, PAT bearer tokens, EC P-256 HTTPS certs |
| **Firmware Integrity** | `IOTAVerifier` | Streaming RSA-2048/SHA-256 signature verification with fail-closed enforcement |
| **Anti-Rollback** | Bootloader & NVS | Monotonic `SECURITY_VERSION` counter in `esp_app_desc_t` and NVS |

### 13.2 DeviceCrypto

The `DeviceCrypto` library (`lib/DeviceCrypto/`) provides hardware-accelerated cryptographic primitives using ESP-IDF and mbedTLS:

- **Master Key Generation**: On first boot, a 256-bit cryptographic key is generated using the ESP32 hardware True Random Number Generator (`esp_random()`).
- **NVS Non-Exportable Storage**: The master key is stored in the NVS partition under the `"device_crypto"` namespace. It persists across firmware updates and is never exported over any network or serial interface. If NVS is wiped, stored encrypted credentials become permanently unrecoverable by design.
- **Symmetric Encryption**: Provides AES-256-CBC encryption and decryption with PKCS#7 padding and random IV generation. Output is Base64-encoded for safe JSON serialization.
- **Hardware Acceleration**: Leverages the ESP32's internal cryptographic hardware engine via mbedTLS APIs rather than software emulation.

### 13.3 KeyManager & Encrypted Credential Store

`KeyManager` (`modules/KeyManager/`) is a kernel service that decouples secret management from general application configuration:

- **Isolated Storage**: Stored at `/etc/keys.json` on LittleFS. Plaintext credentials never touch configuration files (`/mod/*.json`).
- **Encryption at Rest**: All secret values are encrypted via `DeviceCrypto` before writing to the filesystem.
- **CRUD Contract**: Provides `addKey(id, secret, managedBy, exclusive)`, `getKeyById(id)`, `deleteKey(id)`, and `listKeys()`.
- **Ownership Metadata & Access Control**:
  - `managedBy`: Identifies the subsystem that registered the secret (e.g. `"WiFi"`, `"AuthManager"`).
  - `exclusive`: When `true`, prevents the secret from being modified, overwritten, or deleted via the Web Portal REST API (`/api/keys`).
- **Opaque Key References**: Modules reference keys using opaque identifiers (e.g. `"openweather_api"`) rather than storing raw secrets in module config structs.

### 13.4 AuthManager & Web Portal Authentication

The framework integrates a complete authentication subsystem (`AuthManager`) that rejects unauthenticated access across the device's web interfaces and REST APIs:

#### 13.4.1 Password Hashing & Salted Storage
- Passwords are salted with 16 bytes of hardware-generated entropy (`esp_random()`).
- Stored as `SHA-256(salt || password)` inside `KeyManager` under the keys `"portal_password_salt"` and `"portal_password_hash"`.
- Password verification performs constant-time memory comparisons to prevent timing attacks.

#### 13.4.2 Session Management
- Successful login issues an authenticated session token consisting of 16 random bytes (32 hex characters).
- Delivered to the client via an HTTP response cookie:
  ```http
  Set-Cookie: esp32_session=<32-hex-characters>; Path=/; HttpOnly; SameSite=Strict
  ```
- Validated via constant-time comparison on all protected routes.
- Sessions are invalidated explicitly on logout or upon reboot.

#### 13.4.3 Global Authentication Middleware
`WebServerManager` registers an authentication middleware callback on `PsychicHttpServer` that inspects all incoming HTTP requests:

| Route / Path Pattern | Access Rule | Rationale |
|---|---|---|
| Static Assets (`/`, `*.css`, `*.js`, `*.ico`, `*.png`) | Public | Static frontend shell must load before authentication |
| `/api/auth/status`, `/api/auth/login` | Public | Login endpoints to establish session |
| `/api/tabs`, `/api/app` | Public | Read-only bootstrap data for the SPA shell |
| `/api/system` (GET only) | Public | Non-sensitive system status telemetry |
| `/api/system/restart`, `/api/system/factory-reset` | **Authenticated** | Destructive system actions require active session |
| `/api/mcp/*` | MCP Guard | Dedicated MCP bearer token / session cookie validation |
| All other `/api/*` endpoints | **Authenticated** | Protected by `AuthManager::checkPortalAuth()` |

If an unauthenticated request targets a protected endpoint, the middleware halts request processing and returns HTTP 401 Unauthorized JSON immediately.

#### 13.4.4 AP Mode First-Boot Provisioning
When the device boots in Access Point mode (`AppState::Configuring`), `AuthManager::setAPMode(true)` activates a temporary provisioning bypass, allowing initial password configuration and WiFi setup without pre-existing credentials. As soon as the device connects to station mode, AP mode is disabled and full authentication is enforced.

#### 13.4.5 On-Device EC P-256 Certificate Generation
To secure transport without relying on external Public Key Infrastructure (PKI), `AuthManager` can generate an on-device EC P-256 self-signed X.509 certificate:
- **Algorithm**: Elliptic Curve Cryptography using curve `secp256r1` (NIST P-256) with SHA-256 signatures.
- **Subject**: Common Name matches `<hostname>.local`.
- **Storage**: DER-encoded key and certificate saved to `/etc/server.key` and `/etc/server.crt`.
- **Memory Optimization**: Certificate generation uses fixed static buffers (1024 bytes for cert, 256 bytes for private key) and an atomic guard (`_certGenRunning`) to eliminate dynamic heap fragmentation.
- **HTTPS Enforcement**: `PsychicHttpsServer` listens on port 443 with an automatic HTTP-to-HTTPS redirect on port 80.

### 13.5 Model Context Protocol (MCP) Security

The Model Context Protocol server (`MCPServer`) provides structured JSON-RPC tool integration for AI agents with strict security defaults:

- **Read-Only Mode Default**: `_mcpReadOnly` defaults to `true` (Security fix #214). Any attempt to invoke a tool with `mutatesState = true` while read-only mode is active is rejected with JSON-RPC error `-32000` ("Read-only mode").
- **Bearer Token Authentication**: When authentication is enabled, MCP requests over HTTP (`POST /api/mcp`) must supply an authorization header:
  ```http
  Authorization: Bearer <mcp_token>
  ```
  The bearer token matches the device's password hash, retrievable by an authenticated administrator via `GET /api/auth/mcp-token`.
- **Serial Port Trust Model**: MCP requests arriving over the physical hardware UART / USB-CDC serial interface bypass authentication, treating direct physical access as trusted.

### 13.6 OTA Production Update System

The framework provides an end-to-end, cryptographically verified OTA update pipeline.

#### 13.6.1 Split-Repo Architecture
The framework uses a **split-repo OTA model** (PR-37) that decouples firmware binary distribution from source code repositories:
- Firmware release assets and manifests are hosted on a dedicated release endpoint or CDN.
- Eliminates the need for personal access tokens (PATs) with source-code read permissions embedded in firmware binaries.
- Avoids GitHub API unauthenticated rate limits (60 requests/hour) by performing direct HTTP/HTTPS manifest fetching.
- Enables multi-architecture distribution where a single manifest references builds for distinct hardware profiles.

```
+-------------------------------------------------------------+
|                  Dedicated Releases Server                  |
|  manifest.json (v2)                                         |
|    - esp32dev:            firmware-esp32dev.bin (+ .sig)    |
|    - esp32s3nano:         firmware-esp32s3nano.bin (+ .sig) |
|    - crowpanel-128-rotary:firmware-crowpanel.bin (+ .sig)   |
+-------------------------------------------------------------+
                               |
                        Direct HTTPS GET
                               |
                               v
   +-------------------------------------------------------+
   |                  OTAUpdateManager                     |
   |  1. Fetch & parse manifest.json                       |
   |  2. Compare version (SemVer engine)                   |
   |  3. Check anti-rollback security version              |
   |  4. Match BOARD_ID via findAssetForBoard()            |
   |  5. Download firmware + .sig via OTADownloadEngine    |
   |  6. Stream to IOTAVerifier (RSA-2048 / SHA-256)       |
   |  7. Flash write -> reboot -> self-test -> blessing    |
   +-------------------------------------------------------+
```

#### 13.6.2 SemVer Engine & Packed Tag Comparison
The `SemVer` library (`lib/SemVer/`) provides zero-allocation firmware version comparison using packed 32-bit integers:

- **Version Struct**:
  ```cpp
  struct FirmwareVersion {
      uint8_t major   = 0;
      uint8_t minor   = 0;
      uint8_t patch   = 0;
      Channel channel = Channel::STABLE;
      uint8_t preNum  = 0;
  };
  ```
- **Bit Packing**: `packVersion()` packs versions into:
  `MAJOR(8) | MINOR(8) | PATCH(8) | CHANNEL(4) | PRE_NUM(4)`
  allowing instantaneous single-word numeric comparisons.
- **Channel Ordering**: `DEV (0) < BETA (2) < RC (3) < STABLE (15)`. A stable release always supersedes any pre-release with matching major, minor, and patch values.
- **Anti-Rollback Version Packing**: The SemVer tag format packs `CHANNEL(2) | PRE_NUM(6)` into the `SECURITY_VERSION` metadata within `esp_app_desc_t`.

#### 13.6.3 Release Channels & Implicit Downgrades
The active release channel is configured via `IConfigurable` (`/mod/ota.json`, schema v5):
- Channels: `stable` (production releases), `beta` (pre-release testing), `dev` (bleeding-edge builds).
- **Implicit Downgrade**: If a user switches channels (e.g. `beta` to `stable`), `checkForUpdate()` detects that `channel != lastCheckedChannel` and flags an update even if the remote stable version number is lower than the installed beta build.

#### 13.6.4 Multi-Architecture Manifest (v2)
`OTAManifest` (`lib/OTAManifest/`) parses release metadata without heap allocation using fixed stack buffers:
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
The orchestrator matches the build-time macro `BOARD_ID` (e.g. `"esp32dev"`, `"crowpanel-128-rotary"`) using `findAssetForBoard(manifest, BOARD_ID)` to resolve the correct firmware binary and signature.

#### 13.6.5 Cryptographic Verification Pipeline & Fail-Closed Policy
Firmware integrity is verified via the `IOTAVerifier` interface implemented by `MbedTLSSignatureVerifier`:
```cpp
class IOTAVerifier {
public:
    virtual bool begin(const char* publicKeyPem) = 0;
    virtual void update(const uint8_t* data, size_t length) = 0;
    virtual bool verify(const uint8_t* signature, size_t signatureLength) = 0;
};
```
- **Streaming Hash**: Chunks are processed through `IOTAVerifier::update()` as they are received from the network, maintaining an $O(1)$ RAM footprint during multi-megabyte downloads.
- **Fail-Closed Security Invariant**: When an RSA public key is configured in `FrameworkConfig.otaPublicKey`:
  - If the release lacks a `.sig` signature asset, the update is rejected immediately.
  - If the signature payload is empty or truncated, the update aborts before flash operations.
  - If cryptographic signature validation fails, flash writes are halted and the active partition remains untouched.
- **Degraded Mode**: If `otaPublicKey` is `nullptr`, signature validation is skipped and a warning is logged (`ESP_LOGW`). This is permitted only in local development builds.

#### 13.6.6 Download Engine & Flash Safety
`OTADownloadEngine` manages streaming transfers with comprehensive safety checks:
1. **Redirect Handling**: Follows up to 5 HTTP/HTTPS redirects.
2. **Magic Byte Validation**: Validates that the first downloaded byte is the ESP32 firmware magic byte `0xE9`, aborting immediately on invalid binaries.
3. **Partition Space Check**: Compares `Content-Length` against available sketch partition space before starting flash writes.
4. **Task Watchdog Feeding**: Explicitly feeds the ESP-IDF Task Watchdog Timer (TWDT) during chunk processing to prevent panics during network jitter.
5. **DRAM Stack Constraint**: The OTA FreeRTOS worker task stack is allocated strictly in internal DRAM (`MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT`). Because SPI flash operations disable the CPU instruction cache, any task stack allocated in external PSRAM would cause an immediate, fatal cache fault.

#### 13.6.7 Auto-Rollback & Firmware Blessing Ceremony
The framework leverages ESP-IDF hardware bootloader rollback (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`):
1. Upon flashing a new firmware image, the bootloader boots into the newly flashed slot in a `pending verification` state.
2. The firmware is **not blessed** during download or reboot.
3. `Application` invokes `markAppValid()` only at **Step 17b** of the boot sequence, after all hardware, filesystem, network, configuration, and security subsystems have successfully initialized.
4. If a panic, crash, or watchdog timeout occurs prior to Step 17b, the ESP32 reboots and the bootloader automatically rolls back to the previous stable firmware partition.

#### 13.6.8 Application-Layer Anti-Rollback
An NVS-backed anti-rollback monotonic counter (`Preferences` namespace `"ota_security"`, key `"sec_ver"`) prevents rollback to older, vulnerable firmware versions:
- `SECURITY_VERSION` is embedded in the firmware image (`esp_app_desc_t`).
- During boot validation, if `SECURITY_VERSION > stored_sec_ver`, the NVS counter is updated to the higher value.
- Pre-download manifest checks reject any firmware offering a `securityVersion` lower than the device's recorded hardware security floor.

#### 13.6.9 Three-Tier Configuration Lifecycle (ConfigManifest)
`ConfigManifest` (`lib/ConfigManifest/`) maintains configuration schema snapshots across OTA cycles in `/config_manifest.json`:
- **Active**: Current schema version per registered module namespace.
- **Previous**: Snapshot taken immediately before an OTA download (`snapshotActiveToPrevious()`).
- **Golden**: Snapshot recorded after successful firmware blessing at boot (`snapshotActiveToGolden()`).

#### 13.6.10 Web Portal Firmware Upload Route
The web portal exposes `POST /api/ota/upload` supporting multipart file streaming (`firmware` + `signature` fields). The upload handler streams chunks directly into `Update.write()` while accumulating the SHA-256 digest in `MbedTLSSignatureVerifier`, executing the identical cryptographic validation and blessing protocol as remote network updates.

---

## 14. Resource Budget & Constraints

> 📖 **Developer Guides:**
> - [Performance & Optimization](developer/performance-guide.md) — heap budgeting, PSRAM, flash size, TLS memory costs
> - [Embedded Safety](developer/embedded-safety.md) — memory rules, thread safety, DRAM constraints

### 14.1 Target Hardware & Partition Structure

The framework supports multiple ESP32 silicon variants (ESP32 Classic, ESP32-S3) with varying flash and PSRAM configurations. Rather than hardcoding fixed partition sizes into the specification, the definitive partition allocation for each hardware target is defined in its board configuration file at `hardware/<board>/partitions.csv`:

- `hardware/esp32dev/partitions.csv` — Generic ESP32-WROOM-32 (4MB flash)
- `hardware/esp32s3nano/partitions.csv` — Waveshare ESP32-S3-Nano (8MB+ flash, QSPI PSRAM)
- `hardware/crowpanel-128-rotary/partitions.csv` — Elecrow 1.28" Rotary Display (16MB flash, Octal PSRAM)

#### Standard Partition Layout Architecture

All supported board configurations follow a unified partition architecture designed for safe dual-slot OTA updates and reliable configuration persistence:

| Partition | Type | SubType | Purpose |
|---|---|---|---|
| `nvs` | `data` | `nvs` | Non-Volatile Storage for hardware master key (`DeviceCrypto`), WiFi credentials, and anti-rollback `SECURITY_VERSION` counter |
| `otadata` | `data` | `ota` | OTA boot selection table tracking active, candidate, and rollback boot slots |
| `app0` | `app` | `ota_0` | Primary application firmware partition slot |
| `app1` | `app` | `ota_1` | Symmetrical secondary application firmware partition slot |
| `config` | `data` | `spiffs` | Dedicated LittleFS partition for module configuration (`/mod/*.json`), keys (`/etc/keys.json`), and schema tracking |
| `storage` | `data` | `spiffs` | Secondary LittleFS partition for runtime storage, user assets, and audit logs (`/var/audit.log`) |
| `coredump`| `data` | `coredump`| Dedicated flash sector for crash dumps and panic backtraces |

#### Measurement Procedures

To inspect and verify partition allocations and binary footprints:

1. **Partition Table Sizing**: Inspect `hardware/<board>/partitions.csv` directly for exact partition boundaries, offsets, and byte allocations.
2. **Firmware Binary Size & Partition Utilization**:
   Execute the PlatformIO size target for the target environment:
   ```bash
   pio run -e <env> -t size
   ```
   Alternatively, inspect the compiled binary header using `esptool.py`:
   ```bash
   esptool.py image_info .pio/build/<env>/firmware.bin
   ```

### 14.2 Framework Kernel Budget & Sizing Methodology

The framework enforces strict budgeting principles across flash, static memory, and dynamic heap to guarantee stability across all operating conditions:

#### Budgeting Guidelines

- **Flash Binary Headroom**: Production builds must reserve sufficient flash headroom within the board's designated application partition (`app0`/`app1`) to accommodate downstream application logic and optional modules.
- **Static RAM (Data & BSS)**: Minimized by placing immutable lookup tables in flash (`.rodata`), avoiding monolithic global structs, and allocating one-off working buffers on the stack or in short-lived heap scopes.
- **Boot Heap Reserve**: The framework guarantees that upon entering `AppState::RUNNING`, sufficient free DRAM is available to service concurrent WiFi networking, HTTP request dispatching, and TLS handshakes.
- **Static Asset Footprint**: Static web portal assets are compiled into PROGMEM gzip byte arrays by `scripts/portalBuilder.py`. The canonical asset inventory is maintained in `web/portal/` and bundled into `include/portal_assets/` during build time.
- **Test Suite Metrics**: Test coverage and case counts are dynamically reported by the test runner:
  ```bash
  pio test -e native --json-output | jq '.test_count'
  ```

### 14.3 Key Memory & Execution Rules

> 📖 See [Embedded Safety § Memory Rules](developer/embedded-safety.md#1-memory-rules) for the full rule set with examples.

1. **Heap Allocation for TLS/HTTP**: `WiFiClientSecure` and `HTTPClient` must NEVER be allocated on the stack due to their large frame size (>500 bytes plus internal buffers). Always allocate dynamically via `std::unique_ptr<WiFiClientSecure>(new (std::nothrow) WiFiClientSecure())` and verify the pointer before use.
2. **Registration Array Immutability**: All subsystem registration arrays use `std::vector` and are built exclusively during the boot phase. After `Application::begin()` completes Phase 2 (`_compactAndLock()`), structural arrays are permanently locked against runtime mutation.
3. **Module Scope & Lifecycle**: Module instances must be declared `static` at file scope or owned as long-lived member variables within the application class. Stack-allocated modules are strictly prohibited because their destructors leave dangling pointers in the hub's dispatch arrays.
4. **DRAM vs PSRAM Constraints**:
   - **SPI Flash Inaccessibility**: During SPI flash erase and write operations (such as OTA updates or LittleFS writes), the CPU cache is disabled, rendering external PSRAM inaccessible. Any access to PSRAM during a flash write results in a fatal cache fault.
   - **Task Stack Placement**: The FreeRTOS task stacks for tasks performing flash operations (e.g. the OTA download task or HTTP server task) MUST reside strictly in internal DRAM (`MALLOC_CAP_INTERNAL`).
   - **DeepStackExecutor**: Stack-intensive worker tasks (such as JSON parsing or cryptographic operations) that do not perform flash writes may utilize PSRAM task stacks via `DeepStackExecutor` (`lib/DeepStackExecutor/`) to preserve internal DRAM.
   - **Mutual Exclusion**: `DisplayServiceHelper` provides a mutual-exclusion gate that suspends display rendering routines during active OTA flash operations.

**Required SDK configuration:**
- `CONFIG_AUTOSTART_ARDUINO=y` in `sdkconfig.defaults` — mandatory for dual-framework (Arduino + ESP-IDF) projects.
- `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` — enables bootloader auto-rollback.

**Known platform constraint:**
- `Arduino.h` defines macros `#define LOW 0x0` and `#define HIGH 0x1`, which collide with `PriorityTier` enum values. Any translation unit referencing `PriorityTier` alongside Arduino headers must `#undef LOW` and `#undef HIGH`.

### 14.4 Registration Array Model

There are **no `MAX_*` compile-time ceilings**. The framework sizes arrays dynamically during initialisation to fit registered components:

**Structural arrays** (built once at boot, `shrink_to_fit()` called during compaction, immutable in steady state):

| Array | Element Type | Mutation Policy |
|---|---|---|
| `_tickables` | `ITickable*` | Soft-reset only |
| `_displayBoards` | `IDisplayBoard*` | Soft-reset only |
| `_dataSources` | `IDataSource*` | Soft-reset only |
| `_mcpContributors` | `IMCPContributor*` | Soft-reset only |

**Lightweight arrays** (append permitted at any time):

| Array | Element Type | Mutation Policy |
|---|---|---|
| `_configConsumers` | `IConfigurable*` | Append any time |
| `_webContributors` | `IWebContributor*` | Append any time |
| `KeyManager` entries | `KeyInfo` | Append any time |

After Phase 2 compaction, the framework logs heap statistics and publishes an event on `"system.heap"`.

### 14.5 Boot-Phase Allocation Pattern (Reference Implementation)

```
╔══════════════════════════════════════════════════════════════════════╗
║  PHASE 1: REGISTRATION                                           ║
║  begin() open — all registerModule() calls accepted              ║
║  std::vector grows via push_back() (amortised doubling)         ║
╠══════════════════════════════════════════════════════════════════════╣
║  PHASE 2: COMPACTION & LOCK                                      ║
║  Structural arrays: shrink_to_fit() releases excess capacity     ║
║  Heap statistics logged + 'system.heap' EventBus event fired     ║
║  _locked = true  <-- structural arrays are now immutable         ║
╠══════════════════════════════════════════════════════════════════════╣
║  PHASE 3: STEADY-STATE  (tick() loop)                           ║
║  Structural: reads only — zero allocation, zero deallocation    ║
║  Lightweight: append freely — no lock, no shrink                ║
║  Any call to register structural array -> assert / panic         ║
╚══════════════════════════════════════════════════════════════════════╝
```

#### Soft-Reset Cycle

When structural changes are required at runtime (e.g. adding a new display board or enabling a module via configuration):
1. The request sets a deferred flag: `_softResetPending = true`.
2. At the next `tick()` boundary, the framework detects the pending flag.
3. `Application::softReset()` unlocks the registry, clears structural arrays, invokes the application's re-registration hook, calls `_compactAndLock()`, and resumes execution.

### 14.6 Build Profiles & Compiler Optimization

The framework applies compiler and library optimizations configured via `sdkconfig.defaults`:

#### 14.6.1 Compiler Optimization: `-Os`

Production builds configure `CONFIG_COMPILER_OPTIMIZATION_SIZE=y` (`-Os`), enabling speed optimizations while omitting code-expanding transformations such as aggressive loop unrolling.
- The ESP32 executes code from external SPI flash via a 32KB instruction cache. Smaller binary size reduces cache thrashing, making `-Os` highly effective on cache-constrained microcontrollers.
- Workloads are primarily I/O-bound (network, display refresh, serial I/O), making execution time insensitive to unrolled loops.

#### 14.6.2 Logging Strategy & Compile-Time Stripping

Production builds specify `CONFIG_LOG_MAXIMUM_LEVEL=3` (INFO). This sets a **compile-time ceiling**:
- Calls to `ESP_LOGD` (DEBUG) and `ESP_LOGV` (VERBOSE) are completely eliminated from the binary by the preprocessor.
- `ESP_LOGE`, `ESP_LOGW`, and `ESP_LOGI` are compiled into the image.
- Runtime log level adjustments via `esp_log_level_set()` operate within the compiled ceiling (up to level 3).

#### 14.6.3 Build Profiles

| Profile | Target Use | Primary Settings |
|---|---|---|
| **Production** (Default) | Field deployment | `-Os`, `NEWLIB_NANO_FORMAT=y`, `LOG_MAXIMUM_LEVEL=3` (INFO), silent assertions |
| **Debug** | Development & tracing | `-Og`, full printf formatting, `LOG_MAXIMUM_LEVEL=5` (VERBOSE), full assertions |
| **Release** | Highly space-constrained | All production settings + `LOG_MAXIMUM_LEVEL=2` (WARN), default level ERROR |

#### 14.6.4 Observability Tradeoffs: Silent Assertions & Error Lookups

Production builds configure:
- `CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_SILENT=y`: Assertions abort without printing file/line strings to conserve flash. Backtraces are resolved via `addr2line` against the build `.elf` file.
- `CONFIG_ESP_ERR_TO_NAME_LOOKUP=n`: `esp_err_to_name()` returns numeric error codes rather than human-readable strings.

#### 14.6.5 mbedTLS & Cryptographic Pruning

To minimize the cryptographic library footprint, `sdkconfig.defaults` disables unused legacy cipher suites (3DES, Blowfish, RC4, ChaCha20) and unneeded ECC curves, retaining only `secp256r1` (P-256) and AES-GCM/CBC suites required for modern HTTPS and OTA signature verification.

#### 14.6.6 Arduino Managed Components Platform Limitation

PlatformIO's dual-framework builder compiles managed components declared in Arduino's `idf_component.yml` before CMake processes exclusion flags. Unused components are largely stripped by the linker's garbage collection (`--gc-sections`), though some static initialisers and symbol tables remain in the binary. Framework releases are monitored for improved upstream component exclusion support.

---

## 15. Module Authoring Guide

> 📖 **Developer Guides:**
> - [Creating a Module](developer/creating-a-module.md) — step-by-step module implementation tutorial
> - [Coding Patterns](developer/coding-patterns.md) — SOLID principles, Service Module Pattern, style guide
> - [Config System](developer/config-system.md) — ConfigManager gateway, schema migrations, and persistence

### 15.1 Creating a New Module

Modules in the ESP32 App Foundation follow the **Service Module Pattern**: they are autonomous, self-contained components that encapsulate their own state, declare their dependencies via abstract interfaces, and participate in kernel lifecycle loops.

#### Canonical Module Definition (`MyModule.hpp`)

```cpp
#pragma once
#include <AppFoundation.h>  // Canonical single-include façade

class MyModule : public ITickable,
                 public IConfigurable,
                 public IWebContributor,
                 public IMCPContributor,
                 public IPreconditionContributor {
public:
    MyModule() = default;

    // ── ITickable ───────────────────────────────────────────────
    void tick(uint32_t ms) override;

    // ── IConfigurable (ConfigManager Gateway Pattern) ───────────
    const char* getConfigNamespace() const override { return "mymodule"; }
    int getSchemaVersion() const override { return 1; }
    void applyConfig(const JsonObject& section) override;
    void serializeConfig(JsonObject& section) const override;
    bool migrateConfig(JsonObject& section, int fromVersion) override;
    void resetConfig() override;

    // ── IWebContributor (PsychicHttp) ───────────────────────────
    void registerRoutes(PsychicHttpServer& server) override;
    void registerTabs(ITabRegistry& registry) override;

    // ── IMCPContributor ─────────────────────────────────────────
    void registerTools(MCPServer& server) override;

    // ── IPreconditionContributor ────────────────────────────────
    void registerPreconditions(IPreconditionRegistry& registry) override;

private:
    char _host[64] = "localhost";
    uint16_t _port = 8080;
    bool _enabled = true;
};
```

#### Registration via `Application::registerModule()`

Downstream applications declare module instances as `static` or long-lived member variables and register them with the `Application` hub using template auto-detection:

```cpp
// Static or long-lived application instance
static MyModule myModule;

void MyApp::begin() {
    // Template auto-detection checks base interfaces at compile-time (if constexpr)
    // and registers with tick loop, ConfigManager, WebServer, MCP server, etc.
    app.registerModule(&myModule);
}
```

### 15.2 ConfigManager-as-Gateway Pattern

Modules **NEVER interact with LittleFS directly** for configuration persistence. Instead, `ConfigManager` acts as a centralized gateway that manages all filesystem I/O, file locks, atomic writes, and schema migrations:

1. **Namespace Ownership**: Each module implements `getConfigNamespace()`, mapping its configuration to `/mod/<namespace>.json` (e.g. `/mod/mymodule.json`).
2. **Schema Versioning**: `getSchemaVersion()` declares the active schema integer (starts at 1). `ConfigManager` automatically embeds this as a metadata field `"_v"` in the JSON payload.
3. **Applying Configuration**: On boot and configuration imports, `ConfigManager` parses `/mod/<namespace>.json` into an ArduinoJson v7 `JsonObject` and calls `applyConfig(section)`.
4. **Serializing Configuration**: When settings change, the application or web portal triggers a save. `ConfigManager` calls `serializeConfig(section)`, appends the schema version `_v`, and performs an atomic filesystem write.
5. **Schema Migration**: When firmware upgrades increase `getSchemaVersion()`, `ConfigManager` detects `stored_v < getSchemaVersion()` and calls `migrateConfig(section, stored_v)`, allowing the module to upgrade legacy fields in-place before `applyConfig()` is invoked.

#### Example Implementation (`MyModule.cpp`)

```cpp
#include "MyModule.hpp"
#include <platform/LogShim.h>

static const char* TAG = "MyModule";

void MyModule::tick(uint32_t ms) {
    // Keep tick execution <= 10ms
}

void MyModule::applyConfig(const JsonObject& section) {
    strlcpy(_host, section["host"] | "localhost", sizeof(_host));
    _port = section["port"] | 8080;
    _enabled = section["enabled"] | true;
    ESP_LOGI(TAG, "Config applied: host=%s, port=%u", _host, _port);
}

void MyModule::serializeConfig(JsonObject& section) const {
    section["host"] = _host;
    section["port"] = _port;
    section["enabled"] = _enabled;
}

bool MyModule::migrateConfig(JsonObject& section, int fromVersion) {
    if (fromVersion == 0) {
        // Upgrade legacy unversioned structure
        section["enabled"] = true;
    }
    return true;
}

void MyModule::resetConfig() {
    strlcpy(_host, "localhost", sizeof(_host));
    _port = 8080;
    _enabled = true;
}

void MyModule::registerRoutes(PsychicHttpServer& server) {
    server.on("/api/mymodule/status", HTTP_GET, [this](PsychicRequest* req, PsychicResponse* resp) {
        JsonDocument doc;
        doc["host"] = _host;
        doc["port"] = _port;
        doc["enabled"] = _enabled;
        String output;
        serializeJson(doc, output);
        return resp->send(200, "application/json", output.c_str());
    });
}

void MyModule::registerTabs(ITabRegistry& registry) {
    registry.registerTab("My Module", "mymodule", 100);
}

void MyModule::registerTools(MCPServer& server) {
    server.registerTool({
        "mymodule_set_host",
        "Configure host address for MyModule",
        true, // mutatesState: guarded when _mcpReadOnly is true
        [this](const JsonObject& params, JsonObject& result) {
            strlcpy(_host, params["host"] | "localhost", sizeof(_host));
            result["success"] = true;
            return true;
        }
    });
}

void MyModule::registerPreconditions(IPreconditionRegistry& registry) {
    registry.registerPrecondition({
        "mymodule.ready",
        "MyModule Initialisation",
        [this]() { return _enabled; },
        PreconditionSeverity::Warning
    });
}
```

### 15.3 Module Authoring Checklist

When creating or reviewing a module for the ESP32 App Foundation, verify compliance against the following standards:

- [ ] **Interface Contracts**: Implements appropriate framework interfaces (`ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor`, `IPreconditionContributor`, `IDataSource`, `ISanitizer`, `IBootReporter`).
- [ ] **ConfigManager Gateway**: Never accesses LittleFS or filesystem APIs directly for config. Exclusively uses `applyConfig()`, `serializeConfig()`, `migrateConfig()`, and `resetConfig()`.
- [ ] **Schema Versioning**: Declares a unique namespace via `getConfigNamespace()` and an integer schema version via `getSchemaVersion()`.
- [ ] **ArduinoJson v7**: Uses unified `JsonDocument` and `JsonObject` types — never legacy ArduinoJson v6 types (`StaticJsonDocument`/`DynamicJsonDocument`).
- [ ] **Registration**: Module registers cleanly with the application hub via `app.registerModule(&myModule)` with compile-time interface auto-detection.
- [ ] **Boot Preconditions**: Modules that gate system readiness implement `IPreconditionContributor` and register preconditions with appropriate severity (`Info`, `Warning`, `Critical`).
- [ ] **PsychicHttp Routing**: All HTTP endpoints use `PsychicHttpServer` (`PsychicRequest*`, `PsychicResponse*`) and register navigation tabs via `ITabRegistry`.
- [ ] **MCP Tool Safety**: Tools that modify state declare `mutatesState = true`, ensuring enforcement of the `_mcpReadOnly` security guard.
- [ ] **Background Data Sources**: Periodic data fetching implements `IDataSource` and respects `FetchContext` deadline tokens (`DeadlineToken`) to ensure non-blocking cancellation.
- [ ] **Audit & Sanitization**: Sensitive parameters (tokens, passwords) are registered with `Redactor` or sanitized via `ISanitizer` before emission via `AUDIT()` or `AUDIT_SYNC()`.
- [ ] **Observability & Logging**: Uses `#include "platform/LogShim.h"` and `ESP_LOGx(TAG, ...)` macros. Never uses raw `Serial.print()` or custom logging macros.
- [ ] **Module Lifecycle**: Declared as `static` or member-owned. Stack allocation is strictly prohibited.
- [ ] **Decoupling**: Never directly includes another optional module header. Inter-module communication is conducted via `EventBus` pub/sub topics.
- [ ] **Loop Execution Budget**: `tick()` executes in $\le 10$ ms. Heavy operations, network I/O, and blocking loops are deferred to FreeRTOS tasks.
- [ ] **Host Testability**: Compiles cleanly in the native test environment (`[env:native]`) against the Stub-Header Architecture without requiring invasive preprocessor guards in production headers.

---

## 16. Testing Architecture

> 📖 **Developer Guide:** [Build System](developer/build-system.md) — testing tiers, native tests, and CI integration.

### 16.1 Three Testing Tiers

The framework implements a tiered testing methodology designed to validate logic rapidly on host machines while reserving physical hardware for silicon verification:

| Tier | Environment | Runner | Target Scope |
|---|---|---|---|
| **Tier 1 — Native Unit Tests** | Host CPU (`[env:native]`) | GoogleTest | Core logic, state machines, registration, schedulers, parsers, redactor, config migrations |
| **Tier 2 — Hardware Integration** | Physical ESP32 hardware | Unity on-target | WiFi, LittleFS, NVS, hardware crypto, display drivers, peripheral I/O |
| **Tier 3 — End-to-End Boot** | Physical board, full stack | Automated test rig | Full cold boot through `markAppValid()` (Phase 1 → Phase 2 → Phase 3 `RUNNING`) |

### 16.2 Stub-Header Architecture & Compilation Guards

The framework has completely retired the legacy practice of scattering `#ifndef` preprocessor guards throughout production headers. Instead, it relies on a clean **Stub-Header Architecture**:

#### How the Stub-Header Architecture Works

When compiling the native test runner (`[env:native]`), PlatformIO prepends `test/stubs/platform/` to the compiler include search path with highest priority. This directory provides lightweight mock implementations of ESP-IDF and Arduino headers:
- `PsychicHttp.h`, `PsychicHttpsServer.h` — Full mock web server, request/response objects, middleware chaining, and route dispatching
- `Arduino.h`, `Print.h`, `Stream.h` — Core Arduino types, string handling, and timing stubs
- `LittleFS.h`, `Preferences.h` — Virtual in-memory filesystem and key-value storage mocks
- `esp_system.h`, `esp_timer.h`, `esp_random.h` — ESP-IDF system services, timer functions, and pseudo-random entropy generators
- `esp_log.h` — Native logging bridge routing `ESP_LOGx` macros through `LogShim.h`
- `esp_ota_ops.h`, `Update.h` — Flash partition and OTA update simulation
- `WiFi.h`, `WiFiClient.h`, `WiFiClientSecure.h` — Network transport mocks

Production code includes canonical headers (e.g. `#include <PsychicHttp.h>` or `#include <esp_timer.h>`). On target hardware, these resolve to real ESP-IDF/Arduino components; on native test builds, the compiler seamlessly links against the stubs.

#### Target Compilation Guards

When compile-time divergence is strictly necessary (e.g. including target-only FreeRTOS hardware tasks or native-only GoogleTest helpers), the codebase standardises exclusively on:
- **`ESP_PLATFORM`**: Defined by ESP-IDF when compiling for target silicon. Used to exclude physical hardware drivers and real task creation from native host builds.
- **`PIO_UNIT_TESTING`**: Defined by PlatformIO during test suite compilation. Used to expose internal test helpers (such as `DataManager::processNext()` or mock injection hooks).

### 16.3 Native Unit Test Suite Inventory

Host unit tests are organised under `test/test_unit/` by module and functional subsystem. The native test suite encompasses:

| Test Directory | Target Subsystem |
|---|---|
| `test/test_unit/test_application/` | Application hub, boot lifecycle, registration, soft-reset |
| `test/test_unit/test_audit_module/` | AuditModule, JSONL rotation, size caps, event formatting |
| `test/test_unit/test_authManager/` | Password hashing, session cookies, auth middleware, cert generation |
| `test/test_unit/test_buttonModule/` | Debounced push button inputs, press/release event dispatch |
| `test/test_unit/test_configManager/` | LittleFS JSON persistence, schema migrations, import/export |
| `test/test_unit/test_config_manifest/` | Three-tier schema tracking (Active, Previous, Golden) |
| `test/test_unit/test_dataManager/` | Priority scheduling, FetchContext, DeadlineToken cancellation |
| `test/test_unit/test_deepStackExecutor/` | Worker task offloading and stack safety |
| `test/test_unit/test_deviceCrypto/` | AES-256-CBC encryption/decryption, Base64 encoding |
| `test/test_unit/test_displayServiceHelper/` | Tick-safe async bridge, frame buffering, OTA mutual exclusion |
| `test/test_unit/test_encoderModule/` | Rotary encoder direction and rotation step events |
| `test/test_unit/test_eventbus/` | Pub/sub topic dispatching, subscription lifecycle, zero-malloc delivery |
| `test/test_unit/test_hardware_board/` | Board pin configuration and declaration validation |
| `test/test_unit/test_helpers/` | Test fixture helpers and memory leak assertions |
| `test/test_unit/test_homeassistant/` | MQTT discovery payload generation and state reporting |
| `test/test_unit/test_keyManager/` | Encrypted credential storage, exclusive ownership metadata |
| `test/test_unit/test_logpipeline/` | esp_log_set_vprintf interception, mutex locking, log routing |
| `test/test_unit/test_lvglDisplay/` | LVGL v9 display lifecycle, subject binding, screensaver |
| `test/test_unit/test_lvgl_stubs/` | LVGL headless rendering stubs and canvas tests |
| `test/test_unit/test_mcpServer/` | JSON-RPC dispatch, tool registration, read-only guard enforcement |
| `test/test_unit/test_md5Utils/` | Firmware MD5 checksum validation |
| `test/test_unit/test_otaUpdateManager/` | Update state machine, rollback logic, channel transitions |
| `test/test_unit/test_ota_channel/` | Release channel selection and implicit downgrade checks |
| `test/test_unit/test_ota_download_engine/` | Chunked download streaming, redirect handling, header validation |
| `test/test_unit/test_ota_fail_closed/` | Signature enforcement invariants and missing .sig rejection |
| `test/test_unit/test_ota_manifest/` | Multi-architecture manifest parsing and board asset matching |
| `test/test_unit/test_ota_verifier/` | MbedTLS RSA-2048 streaming signature verification |
| `test/test_unit/test_periodic/` | Periodic interval timers and clock math |
| `test/test_unit/test_pngWriter/` | Framebuffer snapshot encoding to PNG byte streams |
| `test/test_unit/test_precondition_engine/` | Boot gating DAG, severity evaluation, PreconditionHealthPayload |
| `test/test_unit/test_redactor/` | Lock-free Meyer's Singleton secret redaction, pattern replacement |
| `test/test_unit/test_semver/` | Packed 32-bit SemVer parsing, sorting, and pre-release precedence |
| `test/test_unit/test_structural_scrubber/` | Log sanitization scrubber and secret masking |
| `test/test_unit/test_timeManager/` | SNTP time synchronization, POSIX timezone parsing, DST rules |
| `test/test_unit/test_u8g2Display/` | Monochrome display boards, carousel timing, widget hierarchy |
| `test/test_unit/test_webServer/` | PsychicHttp route registration, parameter extraction, POST bodies |
| `test/test_unit/test_wifiManager/` | Station connection state machine, AP captive portal DNS routing |

### 16.4 Test Execution & Metrics Verification

Rather than hardcoding static test counts, test coverage and metrics are verified dynamically via the test runner:

```bash
# Execute full native test suite
pio test -e native

# Query total executed test count dynamically
pio test -e native --json-output | jq '.test_count'
```

### 16.5 Build Cache Hygiene

When switching between native host architectures (e.g. x86_64 vs Apple Silicon arm64) or updating platform stub headers, clear PlatformIO's cached object files to prevent linker mismatch errors:
```bash
pio run -t clean -e native
```

---

## 17. Open Questions & Decision Log

This section tracks the architectural decisions that govern the framework, alongside open questions and technical debt.

### Canonical Decision Ledger Policy

- **Section 17 (`docs/design_specification.md`)** is the canonical summary ledger and index for all framework architectural decisions.
- **Detailed ADRs (`docs/developer/adr/`)**: Decisions involving complex trade-offs, interface relocations, or external component evaluations maintain dedicated Architecture Decision Records with full historical context and rationale. Detailed ADRs are cross-linked from this ledger.

### Architectural Decision Log

| # | Decision | Status | Rationale |
|---|---|:---:|---|
| **D1** | No unified `IDisplayDriver` interface | Implemented | U8G2 and TFT/LVGL API models are fundamentally incompatible; display module variants are the correct embedded pattern. |
| **D2** | Per-module LittleFS config files, not a monolithic struct | Superseded | Enables true module independence. Superseded by D40 (ConfigManager gateway pattern). |
| **D3** | Two-tier `std::vector` model, no `MAX_*` ceilings | Implemented | Boot-phase allocation is functionally static. Avoids over-allocating and removes artificial ceilings. Structural arrays use `shrink_to_fit()` after compaction. |
| **D4** | Framework owns `setup()` and `loop()` | Implemented | Defines this as an application framework (Hollywood Principle). Consuming apps implement lifecycle hooks. |
| **D5** | Monorepo — kernel + all optional modules in one repo | Implemented | Avoids multi-repo version matrix fragmentation; ensures atomic refactoring and deterministic submodule SHA pinning. |
| **D6** | Structural array mutations require soft-reset; lightweight arrays grow freely | Implemented | Keeps `tick()` hot path completely free of vector reallocations. Structural mutations trigger a deterministic soft-reset cycle. |
| **D7** | Heap stats logged and published on `'system.heap'` EventBus topic | Implemented | Replaces artificial ceilings with observable runtime evidence. |
| **D8** | Linux-inspired LittleFS directory layout: `etc/`, `mod/`, `app/`, `www/` | Implemented | Directory-per-tier makes ownership self-evident, enables selective wipe, and prevents namespace collisions. |
| **D9** | Hybrid web portal asset model: PROGMEM shell + LittleFS app fragments | Implemented | Framework shell serves from PROGMEM at ~0ms with zero filesystem overhead. Application fragments can be updated independently. |
| **D10** | Apps consume framework via git submodule or PlatformIO library | Implemented | Deterministic SHA pinning and clean `library.json` distribution; no absolute path hacks; reproducible CI. |
| **D11** | Template in monorepo (`template-app/`, `sample-app/`) | Implemented | Single repository to clone; keeps reference applications synchronized with kernel refactors. |
| **D12** | EventBus uses static fn-ptr + `void* ctx` | Superseded | Superseded by D42 (ESP-IDF `esp_event` C++ wrapper). |
| **D13** | EventBus is payload-type-agnostic (`const void*`), not JSON-typed | Implemented | Maximum dispatch performance; type safety and deserialization are the subscriber's responsibility. |
| **D14** | DataManager is a generic scheduler, not HTTP-specific | Implemented | Network transport details belong to individual `IDataSource` implementations. |
| **D15** | `NullStatusReporter` as private nested class in Application | Implemented | Encapsulates default fallback status handling without global dependencies. |
| **D16** | Widget toolkit: three-tier composable architecture | Implemented | Primitives → Generic → Module-Aware widgets with parameter injection. |
| **D17** | MCP Server as kernel service with `IMCPContributor` extension | Implemented | Mirrors `IWebContributor` pattern following the Hollywood Principle. |
| **D18** | MCP transport-agnostic via `processRequest()` | Implemented | Decouples MCP tool dispatching from Serial UART; enables HTTP and future transports. |
| **D19** | `MCPServer` implements `IWebContributor` for HTTP route | Implemented | `WebServerManager` has zero compile-time coupling to MCP internals. |
| **D20** | OTA blessing by Application hub at Step 17b | Implemented | Application must verify hardware, filesystem, and network subsystems before blessing firmware. |
| **D21** | `setMaxRequestBodySize()` body size limits in PsychicHttp | Implemented | Upgraded to native `PsychicHttpServer` request body limits, replacing fragile per-handler checks. |
| **D22** | POSIX TZ via `setenv`/`tzset` for timezone management | Implemented | Leverages standard C runtime `localtime()` for automatic Daylight Saving Time handling. |
| **D23** | Non-blocking async NTP sync | Implemented | SNTP synchronization never blocks boot sequence or main loop execution. |
| **D24** | `build_fonts.py` and `generate_layouts.py` as standalone CLIs | Implemented | Prevents breaking downstream builds when optional font or layout sources are absent. |
| **D25** | `U8G2_DISPLAY_TYPE` macro for display controller selection | Implemented | Overridable via `build_flags`; defaults to SSD1306 128x64. |
| **D26** | Dual framework: Arduino + ESP-IDF | Implemented | Bootloader rollback, IDF logging, and crypto require ESP-IDF; display and web libraries leverage Arduino. |
| **D27** | Standardise on `ESP_LOGx` via `LogShim.h` — no custom macros | Implemented | Provides compile-time log stripping, runtime level control, thread safety, and native test stubs. |
| **D28** | Global secret redaction via `esp_log_set_vprintf()` pipeline (`LogPipeline`) | Implemented | Intercepts all `ESP_LOGx` output at the sink, routing through an injected `ISanitizer` under FreeRTOS mutex protection (Issue #211). |
| **D29** | RAM ring buffer for log capture | Deferred | Deferred to future observability sweep; serial monitor and local capture suffice for current workflows. |
| **D30** | `ILogSink` interface for pluggable log destinations | Deferred | Deferred until remote telemetry sinks and ring buffers are implemented. |
| **D31** | Heartbeat telemetry in `Application::tick()` | Implemented | Emits periodic heap, uptime, and RSSI telemetry every ~15s without external infrastructure. |
| **D32** | MQTT telemetry as optional module (`HomeAssistant`) | Implemented | Keeps kernel minimal; MQTT discovery is enabled via `WITH_HOMEASSISTANT`. |
| **D33** | ArduinoJson v7 `JsonDocument` replaces v6 | Implemented | Unified memory model; eliminates deprecated `StaticJsonDocument` and `DynamicJsonDocument`. |
| **D34** | Display module redesigned as pure service: `Application*` removed | Implemented | Eliminates hardware presenter polling application state; Application owns all display coordination. |
| **D35** | Hierarchical sub-object API for display: `display.carousel`, `display.boards.*`, `display.screensaver` | Implemented | Replaces flat getter lists with domain-rich sub-objects navigable directly by callers. |
| **D36** | Concrete board types at caller layer; `IDisplayBoard*` only at engine boundaries | Implemented | Callers interact with typed boards; abstract interfaces are restricted to container boundaries. |
| **D37** | Screensaver is a distinct display-intrinsic policy layer (not a modal) | Implemented | Screensaver runs as lowest rendering layer, driven by inactivity timers rather than modal interrupts. |
| **D38** | Board taxonomy split: system boards, user boards, display-intrinsic boards | Implemented | Encapsulates internal lifecycle boards while exposing custom boards for application configuration. |
| **D39** | Unified secret redaction via `Redactor` Singleton + `ISanitizer` interface | Implemented | Lock-free append-only array (16 secrets, atomic count, writer spinlock) implementing `ISanitizer` for `AuditModule` and `LogPipeline` (Issue #211). |
| **D40** | ConfigManager-as-Gateway pattern (PR-11) | Implemented | Centralizes all LittleFS persistence in `ConfigManager`. Modules implement `IConfigurable`, declaring namespaces and schema versions, and never perform direct filesystem operations. |
| **D41** | PsychicHttp replacement of legacy web server | Implemented | Adopts `PsychicHttp` built on ESP-IDF's native `esp_http_server`, resolving memory leaks, crash loops, and abandoned maintenance in the legacy async web server library. |
| **D42** | EventBus as `esp_event` C++ wrapper | Implemented | Replaces custom static function pointer arrays with a thin C++ wrapper around the FreeRTOS-backed ESP-IDF default event loop. |
| **D43** | Split-repo OTA update architecture (PR-37) | Implemented | Manifest-first HTTP/HTTPS update pipeline pointing to a dedicated releases repository, eliminating GitHub API rate limits and PAT source-code exposure. |
| **D44** | AuthManager with EC P-256 self-signed certificates & session cookies | Implemented | Complete authentication subsystem: SHA-256 salted hashes, `esp32_session` cookies, PAT bearer tokens for MCP, and on-device EC P-256 X.509 cert generation. |
| **D45** | AuditModule with JSONL rotation | Implemented | Structured operational and security event logging in JSON Lines format to `/var/audit.log` with size-capped rotation, sanitized via `ISanitizer`. |
| **D46** | PreconditionEngine boot gate & health reporting | Implemented | Explicit boot-gating dependency engine and `IPreconditionContributor` interface with `Info`/`Warning`/`Critical` severity evaluation and health payloads. |
| **D47** | LogPipeline and Redactor secret scrubbing (PR-211/212) | Implemented | Automatic redaction of sensitive credentials from serial, crash dumps, and sink outputs without call-site developer burden. Deletes legacy `Logger`. |
| **D48** | DisplayServiceHelper as tick-safe bridge | Implemented | Asynchronous, thread-safe bridge decoupling UI rendering workloads from the main 10ms application tick loop. |
| **D49** | Interface placement heuristic (ADR-002) | Implemented | Interfaces belong in `include/interfaces/` only if called by kernel, free of optional dependencies, and surviving module removal. See [adr-interface-placement.md](developer/adr/adr-interface-placement.md). |
| **D50** | `IDataSource` FetchContext + DeadlineToken (PR-128) | Implemented | Background data fetch contracts accept cooperative cancellation tokens and transport context, preventing task starvation and watchdog timeouts. |
| **D51** | LVGL Pro replacement of SquareLine Studio | Implemented | Adopts LVGL Pro Editor (LVGL v9 XML project format) and LovyanGFX drivers for graphical touch displays, replacing rigid C code generation with declarative XML and reactive subject binding. |
| **D52** | ButtonModule & EncoderModule replacing monolithic buttonHandler | Implemented | Splits hardware input handling into standalone `ButtonModule` and `EncoderModule` components publishing distinct EventBus events. |
| **D53** | Hardware Adapters directory placement (ADR-001) | Implemented | Concrete hardware adapter implementations (e.g. `CrowPanel128Adapter`) live in `adapters/`, maintaining Ports & Adapters separation from pure declarative hardware pin definitions. See [adr-adapters-placement.md](developer/adr/adr-adapters-placement.md). |
| **D54** | DeepStackExecutor for stack-heavy worker execution | Implemented | Offloads deep stack callables (TLS handshakes, complex JSON parsing) to a PSRAM-backed worker task, preserving internal DRAM. |

### Open Questions

| # | Question | Impact | Resolution |
|---|---|---|---|
| **Q1** | Should `DataManager` or a separate `SchedulerManager` manage task scheduling? | Medium | **Resolved**: `DataManager` provides priority-tiered scheduling with `FetchContext` and cooperative `DeadlineToken` cancellation. |
| **Q2** | What is the Web Portal authentication model? | Security | **Resolved**: Implemented by `AuthManager` (D44) using session cookies, SHA-256 salted hashes, and EC P-256 HTTPS certificates. |
| **Q3** | Should display modules provide a PSRAM framebuffer option for S3 boards? | Performance | **Resolved**: Supported on ESP32-S3 boards with `BOARD_HAS_PSRAM` via `LVGLDisplay` and LovyanGFX integration. |
| **Q4** | How should multi-board hardware configs (e.g. dual displays) be handled? | Scope | **Scoped**: Single primary display active per target environment; multi-panel support deferred to future releases. |
| **Q5** | Should BLE provisioning be supported alongside WiFi AP captive portal? | Usability | **Open**: WiFi AP mode currently handles initial onboarding; BLE transport under consideration for headless setups. |

### Known Issues & Technical Debt

| # | Issue | Status | Notes |
|---|---|---|---|
| **TD-1** | Forward declaration cleanup across modular build targets | Resolved | Resolved in PR-17 clean include hierarchy. |
| **TD-2** | Request body size validation across HTTP endpoints | Resolved | Resolved via `PsychicHttpServer::setMaxRequestBodySize()`. |
| **TD-3** | `get_logs` MCP tool integration | Active | Stub placeholder to be connected with AuditModule and RAM ring buffer. |
| **TD-4** | `portalBuilder.py` CI build automation | Resolved | Integrated as automated PlatformIO `extra_scripts` pre-hook. |
| **TD-5** | Generated portal asset header include hygiene | Resolved | Headers encapsulated with include guards and resolved via the Stub-Header Architecture without macro collisions. |

---

*This document is a living specification. Updated on 2026-09-03 for Design Specification v2.0 to reflect the complete delivered architecture across PR-01 through PR-220, including the Stub-Header test architecture, AuthManager security subsystem, ConfigManager gateway pattern, split-repo OTA pipeline, and architectural decisions D40–D54.*
