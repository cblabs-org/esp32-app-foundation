---
name: framework-expert
description: >
  Answers questions about building applications on the ESP32 App Foundation.
  Trigger on: "how do I create a module", "register a module", "registerModule",
  "build an app on the foundation", "weather station", "sensor dashboard", "IoT device",
  "why does registration fail after begin", "is it safe to allocate in tick",
  "ITickable", "IConfigurable", "IWebContributor", "IMCPContributor", "IOTAVerifier",
  "IConfigGate", "IDataSource", "EventBus", "ConfigManager", "web portal tab",
  "display board", "carousel", "screensaver", "replace the display", "use a TFT", "different screen", "e-ink display",
  "hierarchical object model", "sub-object API", "Domain Meaning Test",
  "orchestrator anti-pattern", "service module", "module vs library",
  "global state", "dependency injection", "SOLID principles",
  "thread safety on ESP32", "why not use std::function",
  "TLS memory", "heap usage", "PSRAM", "out of memory", "running low on RAM",
  "firmware too large", "flash partition", "reduce binary size",
  "performance optimisation", "profiling ESP32", "OTA verification", "firmware signing",
  "config system", "extension point", "framework architecture", "AppFoundation.h",
  "tick function", "boot sequence", "LittleFS layout", "FrameworkConfig",
  "DeviceCrypto", "KeyManager", "AuthManager", "JWT", "credential storage",
  "setInsecure", "SECURITY_VERSION", "anti-rollback".
  Also trigger when an agent is working on code that extends the framework
  and needs guidance on interface contracts, patterns, performance, or embedded safety.
  Also trigger on: "audit", "compliance", "tech debt", "code review",
  "scan for violations", "check compliance", "naming conventions",
  "std::nothrow", "bare new", "std::function", "WiFiClientSecure",
  "TWDT", "watchdog", "task stack size", "private_key.pem".
  Also trigger on: "PAT", "personal access token", "OTA token",
  "releases repo", "split repo", "CA fallback", "dirty TLS context",
  "CA rotation", "NVS encryption", "tlsEnabled", "TLS toggle".
  Also trigger on: "certificate bundle", "cert bundle", "CERTIFICATE_BUNDLE",
  "DEFAULT_CMN", "DEFAULT_FULL", "DEFAULT_NONE", "x509_crt_bundle",
  "gen_crt_bundle", "crt.S not found", "data_file_embed_asm",
  "setCACert", "esp_crt_bundle", "disable TLS bundle".
  Also trigger on: "version", "versioning", "semver", "dev tag",
  "beta tag", "stable tag", "inject_version", "deploy_release",
  "build gate", "tag before build", "version bump".
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
