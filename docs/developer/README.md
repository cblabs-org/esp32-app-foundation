# Developer Guide — Building on ESP32 App Foundation

> ESP32 App Foundation (c) 2026 CB Labs
>
> This guide walks you through building custom applications on top of the ESP32 App Foundation framework.
> Follow these tutorials sequentially to understand the architecture, module lifecycle, configuration persistence, web-portal customization, and inter-module communication.

---

## Reading Order

For the best developer experience, read these guides in order:

1. [Architecture Overview](architecture-overview.md) — Understand the framework's structure, lifecycle, and core interfaces.
2. [Creating a Module](creating-a-module.md) — A step-by-step tutorial on implementing custom application modules.
3. [Configuration System](config-system.md) — Learn how to save, migrate, and restore module settings via the ConfigManager.
4. [Extending the Web Portal](extending-the-web-portal.md) — Add custom bottom-nav tabs, page layouts, and REST API endpoints to the device web portal.
5. [Event Bus](event-bus.md) — Implement decoupled, event-driven cross-module communication.

## Quick Reference

- **Single Include Façade:** Downstream applications should only include a single header: `#include "AppFoundation.h"`. Do not couple application code to internal framework paths.
- **Universal Registration:** Register your modules in `setup()` using `app.registerModule(&yourModule)`. The framework auto-detects which interfaces your module implements.
- **Contract-Based Extension:** Custom modules extend the framework by inheriting from pure interface classes located in `include/interfaces/`.
