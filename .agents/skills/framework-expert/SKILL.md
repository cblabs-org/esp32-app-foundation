---
name: framework-expert
description: >
  Expert assistant for the ESP32 App Foundation firmware framework. Use this
  skill for ANY of these scenarios — even when the user doesn't say "framework"
  explicitly: (1) Starting from scratch — blank folder, new ESP32 project,
  clone the framework, bootstrap, scaffold a new app, I don't have the repo yet,
  set up the framework, generate starter code. (2) Designing an app — design
  specification, design spec, application design, help me design my app, create
  a design doc, spec out my project, design before coding, application
  architecture document. (3) Building and extending — registerModule,
  AppFoundation.h, ITickable, IConfigurable, IWebContributor, IMCPContributor,
  IPreconditionContributor, IDataSource, FetchContext, DeadlineToken,
  IDisplayBoard, IOTAVerifier, EventBus, ConfigManager, LittleFS, web portal,
  display carousel, OTA verification, MCP server/device tools,
  PreconditionEngine, precondition, Cause, Severity, IPreconditionRegistry,
  ITabRegistry, registerTabs, DataSourceHealthPayload, data.source.health,
  get_data_sources. (4) Patterns and safety — Service Module Pattern, orchestrator
  anti-pattern, Domain Meaning Test, SOLID, thread safety, std::function, TWDT,
  watchdog, heap, PSRAM, TLS memory, flash size. (5) Security — DeviceCrypto,
  KeyManager, AuthManager, JWT, setInsecure, certificate bundles, esp_crt_bundle,
  DEFAULT_NONE, PAT, OTA tokens, SECURITY_VERSION, anti-rollback, inject_version.
  (6) Release quality gate — release audit, pre-release check, quality gate,
  release readiness, beta audit, stable audit, consumer compliance audit,
  audit my app, compliance check.
  Trigger whenever context includes ESP32, IoT firmware, sensor dashboard,
  embedded C++, or AppFoundation — even if those exact words aren't used.
---

# Framework Expert

Senior developer assistant specializing in building applications, creating modules, and extending the ESP32 App Foundation framework.

## Installation

If you are setting up this skill in a new project, see
`harness-adapters/README.md` for harness-specific installation instructions
(Antigravity, Claude Code).

> **Starting from a blank folder?** If this skill isn't installed yet and
> the user is starting from scratch, use the sibling `framework-setup` skill
> instead — it handles cloning the framework, installing skills, and wiring
> agent harnesses. Once complete, this skill will be available locally.

## Overview

The ESP32 App Foundation is a decoupled, contract-based framework for ESP32 devices. Downstream applications inject custom behavior by implementing one or more pure virtual C++ interfaces and registering them.

All application code interacts with the framework via the single facade include:
```cpp
#include "AppFoundation.h"
```

Modules are registered in `setup()` using the universal auto-detecting registration API:
```cpp
app.registerModule(&myModule);
```

## Quick Answers

### How do I register a module?
Call `app.registerModule(&myModule);` inside `setup()`. The framework uses compile-time type introspection to auto-register it. See `references/registration.md` for rules and lifecycle details.

### What include do I use?
Always use `#include "AppFoundation.h"`. Do not couple directly to internal framework header paths.

### Framework-Reserved Include Prefixes

The following subdirectories of `include/` are reserved by the framework:
- `events/` — EventBus payload types
- `interfaces/` — Pure interface contracts (IDataSource, ITickable, etc.)
- `platform/` — Platform abstraction layer
- `portal_assets/` — Web portal static assets
- `types/` — Framework value types (FetchContext, AppState, etc.)
- `utils/` — Framework utility headers

Consumer applications MUST NOT place headers in these directories. Use app-scoped
paths instead, e.g. `include/modules/my-module/MyTypes.hpp`.

### What interfaces are available?

| Interface Class | Purpose | Key Callback |
| :--- | :--- | :--- |
| `ITickable` | Runs periodic, non-blocking logic in the main tick loop. | `tick(uint32_t)` |
| `IConfigurable` | Persists and loads module settings via `ConfigManager` gateway. | `applyConfig(const JsonObject&)` |
| `IWebContributor` | Exposes REST API endpoints and registers navigation tabs. | `registerRoutes()`, `registerTabs(ITabRegistry&)` |
| `ITabRegistry` | Collects UI navigation tabs from `IWebContributor` modules. | `addTab()` |
| `IMCPContributor` | Registers JSON-RPC tools with the local Model Context Protocol. | `registerMCPTools(MCPToolRegistry&)` |
| `IPreconditionContributor` | Contributes preconditions to the `PreconditionEngine` via visitor pattern. | `registerPreconditions(IPreconditionRegistry&)` |
| `IPreconditionRegistry` | Collects preconditions and dependency edges from `IPreconditionContributor` modules. | `add()` |
| `IDataSource` | Scheduled background data fetch worker managed by `DataManager`. | `fetchUpdate(const FetchContext&)` |
| `IStatusReporter` | Displays boot-phase progress status and unrecoverable errors. | `reportStatus(const char*, float)` |
| `IDisplayBoard` | Renders a telemetry pane inside the monochrome display carousel. | `render(U8G2&)` |
| `IOTAVerifier` | Streaming cryptographic firmware verification for OTA updates. | `verify()` |

## Detailed References

This skill uses a progressive disclosure model. Based on your query, load and read the corresponding reference file or directory:

| Topic | Read |
|---|---|
| **Bootstrap** (blank folder → first build) | `guides/bootstrap.md` |
| **Design Spec** (guided app design → `docs/design-spec.md`) | `guides/design-spec.md` |
| **App Developer Quickstart** (Building a new IoT app) | `guides/app-developer-quickstart.md` |
| **Module Developer Guide** (Adding a kernel module) | `guides/module-developer-guide.md` |
| **Compliance Audit** (Systematic codebase review against all principles) | `guides/compliance-audit.md` |
| **Release Audit** (Structured quality gate for beta/stable releases and consumer compliance) | `guides/release-audit.md` |
| **Filing and triaging framework issues** | `references/issue-management.md` |
| **Contributing changes or proposing new capabilities** | `references/contribution-protocol.md` |
| **Interface Contracts** (Full API, rules, lifecycle for each interface) | `interfaces/` (One file per interface) |
| **Architecture & Boot** (Lifecycle, 3-tier model, LittleFS) | `references/architecture.md` |
| **Registration System** (registerModule, compact-and-lock) | `references/registration.md` |
| **Build System** (PlatformIO, version injection, LittleFS) | `references/build-system.md` |
| **Display System** (Carousel, custom boards, replacing U8G2) | `systems/display/` |
| **Input System** (buttons, rotary encoders, gestures) | `systems/input/` |
| **Config System** (Persistence, FrameworkConfig, ConfigManifest) | `systems/config/` |
| **Event Bus** (Topics, pub/sub, custom topics) | `systems/event-bus/` |
| **Web Portal** (SPA, routes, HTML/JS fragments, security) | `systems/web-portal/` |
| **Web Server Stack & DRAM Tuning** (httpd stack, DeepStackExecutor, PSRAM offload) | `systems/web-portal/dram-tuning.md` |
| **Design System** (Tokens, components, templates, UI kit for portal styling) | Sibling skill: `../framework-design-system/readme.md` |
| **OTA Update System** (State machine, channels, config lifecycle, deploy) | `systems/ota/` |
| **Data Sources & Resilience** (FetchContext, deadlines, health backoff, TLS client) | `interfaces/data-source.md` |
| **MCP Server** (Device interaction, JSON-RPC protocol, tool discovery, scripts) | `systems/mcp-server/` |
| **Device Interaction Scripts** (discovery, diagnostics, display capture) | `scripts/device-mcp/` |
| **Security** (TLS tradeoffs, credentials, OTA security, portal auth) | `security/` |
| **PAT & Repository Config** (Token creation, repo split, blast radius) | `security/pat-management.md` |
| **Coding Patterns** (Service Module Pattern, HOM, SOLID, modules vs libs) | `coding-patterns/` |
| **Performance** (PSRAM, flash size, heap, profiling) | `performance/` |
| **TLS Memory Optimization** (sdkconfig tuning, measured results, cert bundles) | `performance/tls-memory-optimization.md` |
| **Certificate Bundle Config** (CMN/FULL/none, disable guide, DEFAULT_NONE bug) | `security/tls-tradeoffs.md` § Certificate Bundle Configuration |
| **Versioning Policy** (SemVer tags, dev/beta/stable tiers, build gate) | Read `.agents/rules/03-versioning.md` (build gate) and `docs/developer/versioning-policy.md` (full reference) |
| **Embedded Safety** (Thread safety, memory rules, power, rejected approaches)| `embedded-safety/` |

- **Hardware Configuration**: For board.h ownership model (stock vs consumer), pin mapping,
  and custom board authoring, see `references/build-system.md` and
  `docs/developer/custom-boards.md`.

## Data Source Resilience Architecture (Issue #128)

The framework coordinates background data fetching via `DataManager` and `IDataSource`.

### The `FetchContext` Contract
`IDataSource::fetchUpdate(const FetchContext& ctx)` receives a `FetchContext` providing:
- **`ctx.deadline`**: A `DeadlineToken` with `isExpired()`, `isCancelled()`, `remainingMs()`. Long loops (streaming parsers, chunked reads) MUST check `ctx.deadline.isExpired()`.
- **`ctx.client`**: Pre-configured `WiFiClient*` using the ESP-IDF certificate bundle (`FrameworkSecureClient`) when `requiresTls() == true` (default), or standard `WiFiClient` when `false`. Consumers do NOT construct their own TLS clients or call `setInsecure()`.
- **Policy Constants**: `FETCH_TOTAL_TIMEOUT_MS = 15000` (15s total budget), `CONNECT_TIMEOUT_MS = 5000`.
- **Three-Layer Bounded Transport**: `DataManager` bounds the transport via `setConnectionTimeout`, temporarily unsubscribes from the TWDT (`TwdtFetchGuard`), and escalates to `shutdown(fd)` if the deadline expires while blocked. DNS resolution is bounded indirectly by the escalation timer — a slow resolver is detected and logged but cannot be interrupted from the timer context. Sources that ignore `ctx.client` are detected via the escalation timer's `fd() < 0` check.

### Health State Machine & Exponential Backoff
A failing source degrades itself; it **never reboots the device**.
- **States**: `Unknown` (0), `Healthy` (1), `Degraded` (2), `Quarantined` (3), `Suspended` (4).
- **Exponential Backoff**: 30s base delay (`HEALTH_BACKOFF_BASE_MS`), doubling up to a 15-minute cap (`HEALTH_BACKOFF_CAP_MS`).
- **Quarantined**: Reached 15m ceiling. Continues probing every 15 minutes to automatically self-heal if the remote service recovers.
- **Suspended**: Hard failure (`AuthError`). Retries cease to preserve radio and API rate limits. Exited via `DataManager::resetSourceHealth()` or an explicit `requestPriorityFetch()`.
- **neverConnected Distinction**: `SourceHealth::neverConnected()` (`!everSucceeded`) flags configuration problems (user-actionable) vs temporary outages.

### Multi-Surface Surfacing
- **EventBus**: Publishes `ApplicationEvents::DATA_SOURCE_HEALTH` (`"data.source.health"`) with payload `const DataSourceHealthPayload*`.
- **REST Endpoints**: `GET /api/data/sources` and `POST /api/data/sources/reset`.
- **Portal**: Data Sources card on Status tab with live state badges and reset action.
- **Display**: Deferred vector+mutex subscriber in `U8G2DisplayModule` and `LVGLDisplayModule`.
- **MCP Server**: `get_data_sources` built-in tool returns current health snapshots and backoff timings.

## When to Read the Full Docs

These references are optimized distillations for AI agents. If the human developer needs step-by-step tutorials, walkthroughs, or architectural motivation, direct them to the complete developer guides under `docs/developer/`.

## Trigger Reference

> The `description` frontmatter is intentionally lean — it is loaded into every
> session and is what selects this skill. This section is the exhaustive
> vocabulary catalogue; it is read only *after* the skill is invoked, so it
> carries no per-session cost and is never truncated. Use it to confirm scope
> and to recognise the full range of phrasings this skill covers.

- **Hardware configuration**: "board.h", "custom board", "hardware pins", "pin map".
- **Getting started / app building**: "how do I create a module", "register a module",
  "registerModule", "build an app on the foundation", "weather station",
  "sensor dashboard", "IoT device", "why does registration fail after begin",
  "is it safe to allocate in tick".
- **Bootstrap / new project setup**: "blank folder", "start from scratch",
  "new ESP32 project", "clone the framework", "scaffold", "bootstrap",
  "I don't have the repo yet", "set up a new app", "set up the framework",
  "generate starter code", "initialise project".
- **Design specification**: "design spec", "design specification",
  "application design", "help me design my app", "create a design doc",
  "spec out my project", "architecture document", "design before coding",
  "what should my app look like", "help me think through my app",
  "application architecture", "design session".
- **Interfaces**: "ITickable", "IConfigurable", "IWebContributor", "IMCPContributor",
  "IOTAVerifier", "IPreconditionContributor", "IPreconditionRegistry", "PreconditionEngine",
  "Cause", "Severity", "PreconditionRole", "precondition",
  "IDataSource", "FetchContext", "DeadlineToken", "DataSourceHealthPayload",
  "data.source.health", "get_data_sources", "resetSourceHealth",
  "IStatusReporter", "IDisplayBoard", "ITabRegistry", "registerTabs".
- **Core systems**: "EventBus", "ConfigManager", "FrameworkConfig", "config system",
  "extension point", "framework architecture", "AppFoundation.h", "tick function",
  "boot sequence", "LittleFS layout", "DataManager", "fetchUpdate", "queueDataJob".
- **Display**: "display board", "carousel", "screensaver", "replace the display",
  "use a TFT", "different screen", "e-ink display".
- **Input**: "button", "ButtonModule", "WITH_BUTTONS", "rotary encoder",
  "EncoderModule", "WITH_ENCODER", "PCNT", "quadrature", "gesture",
  "button.click", "encoder.rotate", "iot_button", "consumeDelta".
- **Coding patterns / architecture**: "hierarchical object model", "sub-object API",
  "Domain Meaning Test", "orchestrator anti-pattern", "service module",
  "module vs library", "global state", "dependency injection", "SOLID principles".
- **Embedded safety**: "thread safety on ESP32", "why not use std::function",
  "std::nothrow", "bare new", "std::function", "WiFiClientSecure", "TWDT",
  "watchdog", "task stack size".
- **Performance**: "TLS memory", "heap usage", "PSRAM", "out of memory",
  "running low on RAM", "firmware too large", "flash partition",
  "reduce binary size", "performance optimisation", "profiling ESP32".
- **Audit / compliance**: "audit", "compliance", "tech debt", "code review",
  "scan for violations", "check compliance", "naming conventions",
  "release audit", "release gate", "pre-release check", "quality gate",
  "release checklist", "beta audit", "stable audit", "release readiness",
  "pre-release validation", "release candidate review", "consumer audit",
  "compliance check", "audit my app".
- **Security & credentials**: "DeviceCrypto", "KeyManager", "AuthManager", "JWT",
  "credential storage", "setInsecure", "SECURITY_VERSION", "anti-rollback",
  "private_key.pem".
- **PAT & repository config**: "PAT", "personal access token", "OTA token",
  "releases repo", "split repo", "CA fallback", "dirty TLS context", "CA rotation",
  "NVS encryption", "tlsEnabled", "TLS toggle".
- **Certificate bundle**: "certificate bundle", "cert bundle", "CERTIFICATE_BUNDLE",
  "DEFAULT_CMN", "DEFAULT_FULL", "DEFAULT_NONE", "x509_crt_bundle", "gen_crt_bundle",
  "crt.S not found", "data_file_embed_asm", "setCACert", "esp_crt_bundle",
  "disable TLS bundle".
- **Versioning**: "version", "versioning", "semver", "dev tag", "beta tag",
  "stable tag", "inject_version", "deploy_release", "build gate", "tag before build",
  "version bump".
- **OTA manifest**: "manifest signing", "manifest-first", "otaManifestUrl",
  "manifest-latest release", "manifest.json.sig", "signed manifest",
  "manifest URL resilience", "learned URL", "compiled-in URL", "defence-in-depth",
  "manifest schema v2", "multi-channel manifest", "OTA verification", "firmware signing".
- **MCP server / device interaction**: "MCP server", "MCP tool", "device MCP",
  "list_tools", "get_data_sources", "device diagnostics", "device IP", "system_info", "capture_display",
  "wifi_get_status", "ota_get_status", "display_status", "config_factory_reset",
  "readOnly mode", "JSON-RPC", "call a tool on the device", "talk to the device",
  "query the device", "device telemetry".
- **REST APIs**: "/api/data/sources", "/api/data/sources/reset", "/api/mcp",
  "/api/wifi/status", "/api/system".

Also trigger when an agent is working on code that extends the framework and needs
guidance on interface contracts, patterns, performance, or embedded safety.

- **Issue management**: "raise a framework issue", "file a GitHub issue", "report a bug",
  "framework defect", "upstream issue", "consumer found a bug", "backfill issue",
  "P1-blocker", "P2-correctness", "P3-docs-dx", "found-by", "issue tracker",
  "consumer pointer", "audit pointer", "framework issue template".

- **Contributing & proposals**: "raise a PR", "submit a fix", "contribute to the
  framework", "open a pull request", "upstream a change", "patch the framework",
  "propose a feature", "suggest an improvement", "new framework capability",
  "write an ADR", "architectural decision", "enhancement proposal", "contribution protocol".
  → Read `references/contribution-protocol.md` for the full process.
