# Developer Guide — Building on ESP32 App Foundation

> ESP32 App Foundation (c) 2026 CB Labs
>
> This guide walks you through building custom applications on top of the ESP32 App Foundation framework.
> Follow these tutorials sequentially to understand the architecture, module lifecycle, configuration persistence, web-portal customization, and inter-module communication.

---

## Reading Order

For the best developer experience, read the core guides in order:

1. [Architecture Overview](architecture-overview.md) — Understand the framework's structure, lifecycle, and core interfaces.
2. [Creating a Module](creating-a-module.md) — A step-by-step tutorial on implementing custom application modules.
3. [Coding Patterns](coding-patterns.md) — Modules vs libraries, SOLID principles, the Hierarchical Object Model, the Service Module Pattern, and the project style guide.
4. [Configuration System](config-system.md) — Learn how to save, migrate, and restore module settings via the ConfigManager.
5. [Extending the Web Portal](extending-the-web-portal.md) — Add custom bottom-nav tabs, page layouts, REST API endpoints, and JWT authentication to the device web portal.
6. [Event Bus](event-bus.md) — Implement decoupled, event-driven cross-module communication.
7. [Using Optional Modules](using-optional-modules.md) — The display subsystem (carousel, custom boards, screensaver), replacing the display hardware, and the `NATIVE_BUILD` guard pattern.
8. [Scheduling Data Fetches](scheduling-data-fetches.md) — Background data workers via `IDataSource` and `DataManager`, including thread safety patterns.
9. [Build System](build-system.md) — PlatformIO environments, version injection, partition schemes, and the LittleFS filesystem.

## Advanced Topics

Once you are building or shipping an application, read these for production readiness:

10. [Performance & Optimization](performance-guide.md) — RAM/flash budgeting, heap management, TLS memory costs, PSRAM usage, and profiling techniques.
11. [Embedded Safety](embedded-safety.md) — Thread safety rules, memory allocation constraints, power management, and the design graveyard (10 rejected approaches with full rationale).
12. [Security](security.md) — Threat model, hardware encryption, credential storage, TLS tradeoffs, web portal authentication, and OTA firmware integrity.
13. [MCP Server](mcp-server.md) — The Model Context Protocol server, tool registration via `IMCPContributor`, built-in tools, and HTTP JSON-RPC transport.

## Quick Reference

- **Single Include Façade:** Downstream applications should only include a single header: `#include "AppFoundation.h"`. Do not couple application code to internal framework paths.
- **Universal Registration:** Register your modules in `setup()` using `app.registerModule(&yourModule)`. The framework auto-detects which interfaces your module implements.
- **Contract-Based Extension:** Custom modules extend the framework by inheriting from pure interface classes located in `include/interfaces/`.
- **Static Scope Rule (ADR-16):** All module instances MUST be declared at global or static scope. The ESP32 has no MMU — stack-allocated modules will corrupt memory.

