---
name: framework-expert
description: >
  Building apps, creating modules, extending, and auditing code on the ESP32 App
  Foundation framework. Covers module interfaces (registerModule, AppFoundation.h,
  ITickable, IConfigurable, IWebContributor, IMCPContributor, IConfigGate,
  IDataSource, IDisplayBoard, IOTAVerifier); core systems (EventBus, ConfigManager,
  FrameworkConfig, LittleFS, web portal, display carousel, OTA verification, MCP
  server/device tools); coding patterns (Service Module Pattern, hierarchical object
  model, orchestrator anti-pattern, Domain Meaning Test, SOLID); embedded
  safety/performance (thread safety, std::function, std::nothrow, TWDT/watchdog,
  heap/PSRAM, TLS memory, flash size); and security (DeviceCrypto, KeyManager,
  AuthManager, JWT, setInsecure, certificate bundles/esp_crt_bundle/DEFAULT_NONE,
  PAT/OTA tokens, otaManifestUrl, SECURITY_VERSION/anti-rollback, inject_version).
  Also handles bootstrapping a new project from scratch: blank folder, clone the
  framework, scaffold new app, new ESP32 project, start from scratch, set up the
  framework, I don't have the repo yet, bootstrap.
  Trigger when extending the framework or reviewing code against its interfaces,
  patterns, performance, or safety rules.
---

# Framework Expert

Senior developer assistant specializing in building applications, creating modules, and extending the ESP32 App Foundation framework.

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

### What interfaces are available?

| Interface Class | Purpose | Key Callback |
| :--- | :--- | :--- |
| `ITickable` | Runs periodic, non-blocking logic in the main tick loop. | `tick(uint32_t)` |
| `IConfigurable` | Persists and loads module settings via `ConfigManager` gateway. | `applyConfig(const JsonObject&)` |
| `IWebContributor` | Exposes REST API endpoints and appends navigation tabs. | `registerRoutes(PsychicHttpServer&)` |
| `IMCPContributor` | Registers JSON-RPC tools with the local Model Context Protocol. | `registerMCPTools(MCPToolRegistry&)` |
| `IConfigGate` | Blocks transition to RUNNING state until requirements are satisfied. | `isSatisfied()` |
| `IDataSource` | Scheduled background data fetch worker managed by `DataManager`. | `fetchUpdate()` |
| `IStatusReporter` | Displays boot-phase progress status and unrecoverable errors. | `reportStatus(const char*, float)` |
| `IDisplayBoard` | Renders a telemetry pane inside the monochrome display carousel. | `render(U8G2&)` |
| `IOTAVerifier` | Streaming cryptographic firmware verification for OTA updates. | `verify()` |

## Detailed References

This skill uses a progressive disclosure model. Based on your query, load and read the corresponding reference file or directory:

| Topic | Read |
|---|---|
| **Bootstrap** (blank folder → first build) | `guides/bootstrap.md` |
| **App Developer Quickstart** (Building a new IoT app) | `guides/app-developer-quickstart.md` |
| **Module Developer Guide** (Adding a kernel module) | `guides/module-developer-guide.md` |
| **Compliance Audit** (Systematic codebase review against all principles) | `guides/compliance-audit.md` |
| **Interface Contracts** (Full API, rules, lifecycle for each interface) | `interfaces/` (One file per interface) |
| **Architecture & Boot** (Lifecycle, 3-tier model, LittleFS) | `references/architecture.md` |
| **Registration System** (registerModule, compact-and-lock) | `references/registration.md` |
| **Build System** (PlatformIO, version injection, LittleFS) | `references/build-system.md` |
| **Display System** (Carousel, custom boards, replacing U8G2) | `systems/display/` |
| **Config System** (Persistence, FrameworkConfig, ConfigManifest) | `systems/config/` |
| **Event Bus** (Topics, pub/sub, custom topics) | `systems/event-bus/` |
| **Web Portal** (SPA, routes, HTML/JS fragments, security) | `systems/web-portal/` |
| **OTA Update System** (State machine, channels, config lifecycle, deploy) | `systems/ota/` |
| **MCP Server** (Device interaction, JSON-RPC protocol, tool discovery, scripts) | `systems/mcp-server/` |
| **Security** (TLS tradeoffs, credentials, OTA security, portal auth) | `security/` |
| **PAT & Repository Config** (Token creation, repo split, blast radius) | `security/pat-management.md` |
| **Coding Patterns** (Service Module Pattern, HOM, SOLID, modules vs libs) | `coding-patterns/` |
| **Performance** (PSRAM, flash size, heap, profiling) | `performance/` |
| **TLS Memory Optimization** (sdkconfig tuning, measured results, cert bundles) | `performance/tls-memory-optimization.md` |
| **Certificate Bundle Config** (CMN/FULL/none, disable guide, DEFAULT_NONE bug) | `security/tls-tradeoffs.md` § Certificate Bundle Configuration |
| **Versioning Policy** (SemVer tags, dev/beta/stable tiers, build gate) | Read `.agents/rules/03-versioning.md` (build gate) and `docs/developer/versioning-policy.md` (full reference) |
| **Embedded Safety** (Thread safety, memory rules, power, rejected approaches)| `embedded-safety/` |

## When to Read the Full Docs

These references are optimized distillations for AI agents. If the human developer needs step-by-step tutorials, walkthroughs, or architectural motivation, direct them to the complete developer guides under `docs/developer/`.

## Trigger Reference

> The `description` frontmatter is intentionally lean — it is loaded into every
> session and is what selects this skill. This section is the exhaustive
> vocabulary catalogue; it is read only *after* the skill is invoked, so it
> carries no per-session cost and is never truncated. Use it to confirm scope
> and to recognise the full range of phrasings this skill covers.

- **Getting started / app building**: "how do I create a module", "register a module",
  "registerModule", "build an app on the foundation", "weather station",
  "sensor dashboard", "IoT device", "why does registration fail after begin",
  "is it safe to allocate in tick".
- **Bootstrap / new project setup**: "blank folder", "start from scratch",
  "new ESP32 project", "clone the framework", "scaffold", "bootstrap",
  "I don't have the repo yet", "set up a new app", "set up the framework",
  "generate starter code", "initialise project".
- **Interfaces**: "ITickable", "IConfigurable", "IWebContributor", "IMCPContributor",
  "IOTAVerifier", "IConfigGate", "IDataSource", "IStatusReporter", "IDisplayBoard".
- **Core systems**: "EventBus", "ConfigManager", "FrameworkConfig", "config system",
  "extension point", "framework architecture", "AppFoundation.h", "tick function",
  "boot sequence", "LittleFS layout".
- **Display**: "display board", "carousel", "screensaver", "replace the display",
  "use a TFT", "different screen", "e-ink display".
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
  "scan for violations", "check compliance", "naming conventions".
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
  "list_tools", "device diagnostics", "device IP", "system_info", "capture_display",
  "wifi_get_status", "ota_get_status", "display_status", "config_factory_reset",
  "readOnly mode", "JSON-RPC", "call a tool on the device", "talk to the device",
  "query the device", "device telemetry".

Also trigger when an agent is working on code that extends the framework and needs
guidance on interface contracts, patterns, performance, or embedded safety.
