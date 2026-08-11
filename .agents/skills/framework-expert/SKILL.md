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
  IConfigGate, IDataSource, IDisplayBoard, IOTAVerifier, EventBus, ConfigManager,
  LittleFS, web portal, display carousel, OTA verification, MCP server/device
  tools. (4) Patterns and safety — Service Module Pattern, orchestrator
  anti-pattern, Domain Meaning Test, SOLID, thread safety, std::function, TWDT,
  watchdog, heap, PSRAM, TLS memory, flash size. (5) Security — DeviceCrypto,
  KeyManager, AuthManager, JWT, setInsecure, certificate bundles, esp_crt_bundle,
  DEFAULT_NONE, PAT, OTA tokens, SECURITY_VERSION, anti-rollback, inject_version.
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
| **Design Spec** (guided app design → `docs/design-spec.md`) | `guides/design-spec.md` |
| **App Developer Quickstart** (Building a new IoT app) | `guides/app-developer-quickstart.md` |
| **Module Developer Guide** (Adding a kernel module) | `guides/module-developer-guide.md` |
| **Compliance Audit** (Systematic codebase review against all principles) | `guides/compliance-audit.md` |
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
| **Design System** (Tokens, components, templates, UI kit for portal styling) | Sibling skill: `../framework-design-system/readme.md` |
| **OTA Update System** (State machine, channels, config lifecycle, deploy) | `systems/ota/` |
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
- **Design specification**: "design spec", "design specification",
  "application design", "help me design my app", "create a design doc",
  "spec out my project", "architecture document", "design before coding",
  "what should my app look like", "help me think through my app",
  "application architecture", "design session".
- **Interfaces**: "ITickable", "IConfigurable", "IWebContributor", "IMCPContributor",
  "IOTAVerifier", "IConfigGate", "IDataSource", "IStatusReporter", "IDisplayBoard".
- **Core systems**: "EventBus", "ConfigManager", "FrameworkConfig", "config system",
  "extension point", "framework architecture", "AppFoundation.h", "tick function",
  "boot sequence", "LittleFS layout".
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

- **Issue management**: "raise a framework issue", "file a GitHub issue", "report a bug",
  "framework defect", "upstream issue", "consumer found a bug", "backfill issue",
  "P1-blocker", "P2-correctness", "P3-docs-dx", "found-by", "issue tracker",
  "consumer pointer", "audit pointer", "framework issue template".

- **Contributing & proposals**: "raise a PR", "submit a fix", "contribute to the
  framework", "open a pull request", "upstream a change", "patch the framework",
  "propose a feature", "suggest an improvement", "new framework capability",
  "write an ADR", "architectural decision", "enhancement proposal", "contribution protocol".
  → Read `references/contribution-protocol.md` for the full process.
