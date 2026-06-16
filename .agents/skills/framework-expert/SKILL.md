---
name: framework-expert
description: >
  Answers questions about building applications on the ESP32 App Foundation.
  Trigger on: "how do I create a module", "register a module", "registerModule",
  "ITickable", "IConfigurable", "IWebContributor", "IMCPContributor",
  "IConfigGate", "IDataSource", "EventBus", "ConfigManager", "web portal tab",
  "config system", "extension point", "framework architecture", "AppFoundation.h",
  "tick function", "boot sequence", "LittleFS layout", "FrameworkConfig".
  Also trigger when an agent is working on code that extends the framework
  and needs guidance on interface contracts or registration patterns.
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
Call `app.registerModule(&myModule);` inside `setup()`. The framework uses compile-time type introspection to auto-register it with all implemented subsystems.

### What include do I use?
Always use `#include "AppFoundation.h"`. Do not couple directly to internal framework header paths.

### What interfaces are available?

| Interface Class | Purpose | Key Callback |
| :--- | :--- | :--- |
| [ITickable](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/include/interfaces/ITickable.hpp) | Runs periodic, non-blocking logic in the main tick loop. | `tick(uint32_t currentMillis)` |
| [IConfigurable](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/include/interfaces/IConfigurable.hpp) | Persists and loads module settings via `ConfigManager` gateway. | `applyConfig(const JsonObject&)` |
| [IWebContributor](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/include/interfaces/IWebContributor.hpp) | Exposes REST API endpoints and appends navigation tabs. | `registerRoutes(PsychicHttpServer&)` |
| [IMCPContributor](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/include/interfaces/IMCPContributor.hpp) | Registers JSON-RPC tools with the local Model Context Protocol. | `registerMCPTools(MCPToolRegistry&)` |
| [IConfigGate](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/include/interfaces/IConfigGate.hpp) | Blocks transition to RUNNING state until requirements are satisfied. | `isSatisfied()` |
| [IDataSource](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/include/interfaces/IDataSource.hpp) | Scheduled background data fetch worker managed by `DataManager`. | `fetchUpdate()` |
| [IStatusReporter](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/include/interfaces/IStatusReporter.hpp) | Displays boot-phase progress status and unrecoverable errors. | `reportStatus(const char*, float)` |
| [IDisplayBoard](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/include/interfaces/IDisplayBoard.hpp) | Renders a telemetry pane inside the monochrome display carousel. | `render(U8G2&)` |

## Detailed References

Based on your query, load and read the corresponding reference file:

| Topic | Read |
|---|---|
| Framework architecture, boot lifecycle, 3-tier model | [architecture.md](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/framework-expert/references/architecture.md) |
| Creating a new module, interface selection, registration | [creating-a-module.md](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/framework-expert/references/creating-a-module.md) |
| IConfigurable, schema versioning, config persistence | [config-system.md](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/framework-expert/references/config-system.md) |
| IWebContributor, REST routes, web portal JS fragments | [web-portal.md](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/framework-expert/references/web-portal.md) |
| EventBus, topics, subscribe/publish patterns | [event-bus.md](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/framework-expert/references/event-bus.md) |

## When to Read the Full Docs

The reference files above are optimized distillations containing syntax rules, APIs, and key facts. If you need step-by-step tutorials, walkthroughs, or architectural motivation, read the complete developer guides under `docs/developer/`:
- [Architecture Overview](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/docs/developer/architecture-overview.md)
- [Creating a Module](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/docs/developer/creating-a-module.md)
- [Configuration System](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/docs/developer/config-system.md)
- [Extending the Web Portal](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/docs/developer/extending-the-web-portal.md)
- [Event Bus Guide](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/docs/developer/event-bus.md)
