# Developer Guide — Building on ESP32 App Foundation

> ESP32 App Foundation (c) 2026 CB Labs
>
> This guide walks you through building custom applications on top of the ESP32 App Foundation framework.
> Follow these tutorials sequentially to understand the architecture, module lifecycle, configuration persistence, web-portal customization, and inter-module communication.

---

## Reading Order

For the best developer experience, read the core guides in order:

1. [Architecture Overview](architecture-overview.md) — Understand the framework's structure, lifecycle, and core interfaces.
2. [Sample App Walkthrough](sample-app-walkthrough.md) — End-to-end guided tour of the Weather Station sample app, showing every framework feature in action.
3. [Creating a Module](creating-a-module.md) — A step-by-step tutorial on implementing custom application modules.
4. [Coding Patterns](coding-patterns.md) — Modules vs libraries, SOLID principles, the Hierarchical Object Model, the Service Module Pattern, and the project style guide.
5. [Configuration System](config-system.md) — Learn how to save, migrate, and restore module settings via the ConfigManager.
6. [Extending the Web Portal](extending-the-web-portal.md) — Add custom bottom-nav tabs, page layouts, REST API endpoints, and JWT authentication to the device web portal.
7. [Event Bus](event-bus.md) — Implement decoupled, event-driven cross-module communication.
8. [Using Optional Modules](using-optional-modules.md) — The display subsystem (carousel, custom boards, screensaver), replacing the display hardware, and the `ESP_PLATFORM` guard pattern.
9. [Scheduling Data Fetches](scheduling-data-fetches.md) — Background data workers via `IDataSource` and `DataManager`, including thread safety patterns.
10. [FetchContext Contract](fetch-context-contract.md) — The data source resilience contract, cooperative deadlines, TLS client lifecycle, and health state machine (#128).
11. [Build System](build-system.md) — PlatformIO environments, version injection, partition schemes, and the LittleFS filesystem.

## Advanced Topics

Once you are building or shipping an application, read these for production readiness:

12. [Performance & Optimization](performance-guide.md) — RAM/flash budgeting, heap management, TLS memory costs, PSRAM usage, and profiling techniques.
13. [Embedded Safety](embedded-safety.md) — Thread safety rules, memory allocation constraints, power management, and the design graveyard (10 rejected approaches with full rationale).
14. [Security](security.md) — Threat model, hardware encryption, credential storage, TLS tradeoffs, web portal authentication, and OTA firmware integrity.
15. [MCP Server](mcp-server.md) — The Model Context Protocol server, tool registration via `IMCPContributor`, built-in tools, and HTTP JSON-RPC transport.
16. [OTA Update System](ota-update-system.md) — The complete OTA lifecycle: state machine, release channels, config snapshots, multi-board manifests, error codes, and deployment procedures.
17. [Custom Board Authoring Guide](custom-boards.md) — Author custom board definitions and pinouts for downstream repos with zero framework edits (HC-11).
18. [Reporting Framework Issues](issue-management.md) — How to raise, label, and track defects on the framework repo; the consumer-pointer convention for audit files.
19. [Migration Guide: fetchUpdate](migration-guide-fetchupdate.md) — Downstream migration guide for the breaking `fetchUpdate(const FetchContext&)` and `queueDataJob()` APIs.

## Quick Reference

- **Single Include Façade:** Downstream applications should only include a single header: `#include "AppFoundation.h"`. Do not couple application code to internal framework paths.
- **Universal Registration:** Register your modules in `setup()` using `app.registerModule(&yourModule)`. The framework auto-detects which interfaces your module implements.
- **Contract-Based Extension:** Custom modules extend the framework by inheriting from pure interface classes located in `include/interfaces/`.
- **Static Scope Rule (ADR-16):** All module instances MUST be declared at global or static scope. The ESP32 has no MMU — stack-allocated modules will corrupt memory.
- **Sample App:** A complete Weather Station at `sample-app/` demonstrates all 7 app-facing interfaces. For a minimal starting point, see `template-app/`.

