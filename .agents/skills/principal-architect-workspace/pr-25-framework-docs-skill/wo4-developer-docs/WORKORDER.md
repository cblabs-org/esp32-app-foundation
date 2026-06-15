---
name: pr-25-wo4
description: Handoff instructions for Work Order 4 — Developer documentation (5 MVP guides)
complexity: architect
---

# WO4 — Developer Documentation (5 MVP Guides)

You are an expert senior software engineer and technical writer, and an
execution-focused worker agent. You are NOT the Principal Architect. Do not
make high-level architectural decisions, do not branch, and do not plan.
Your task is strictly limited to executing the checklist below.

---

## Step 0 — Mandatory Rules Load (DO THIS FIRST)

**Before doing anything else**, read every file inside `.agents/rules/` in
filename order using file-read tools. These are unconditional operational
constraints. Do not skip this step. Do not assume they were auto-loaded.

---

## Prerequisite Context

WO1–WO3 have already landed. Before writing any documentation, you MUST read
the following files to ensure accuracy. Do NOT write documentation from memory —
every code example and API reference must be verified against the actual source:

**Critical files to read first:**
- `modules/Application/Application.hpp` — `registerModule<T>()`, lifecycle, public API
- `modules/Application/Application.cpp` — `begin()` boot sequence, `tick()` loop
- `include/interfaces/ITickable.hpp` — `tick(uint32_t currentMillis)` contract
- `include/interfaces/IConfigurable.hpp` — full config gateway contract
- `include/interfaces/IWebContributor.hpp` — web portal extension contract
- `include/interfaces/IConfigGate.hpp` — config gate contract (has excellent inline docs)
- `include/interfaces/IMCPContributor.hpp` — MCP extension contract
- `include/interfaces/IDataSource.hpp` — background data fetch contract
- `modules/EventBus/EventBus.hpp` — subscribe/publish API
- `modules/ConfigManager/ConfigManager.hpp` — config gateway API
- `template/src/myApp.hpp` and `myApp.cpp` — the expanded template (WO3 output)
- `template/src/main.cpp` — the entry point showing `registerModule()`
- `template/data/www/myapp/tab.html` and `tab.js` — web portal JS fragment
- `CONTRIBUTING.md` — the updated contributing guide
- `docs/design_specification.md` — the design spec (reference, not to be duplicated)
- `include/AppFoundation.h` — the single-include façade

---

## Objective

Create 5 developer-facing documentation guides in `docs/developer/`, plus two
index README files. These guides are tutorial-style prose aimed at a developer
who has just cloned the repo and wants to build an application on the framework.

**Writing style:**
- Tutorial prose, NOT specification prose. Warm, direct, practical.
- Every code example must be verified against the actual source files listed above.
- Cross-reference `docs/design_specification.md` sections for deeper reading,
  but do not duplicate specification content.
- Use relative links for cross-references between docs.
- Use the house-style header comment from the project (check any existing `.md`
  file in `docs/` for the pattern).

---

## Strict Boundary Box

- **DO** create: files in `docs/developer/` and `docs/README.md`
- **DO NOT** modify any source code files (`.hpp`, `.cpp`, `.h`, `.js`)
- **DO NOT** modify `CONTRIBUTING.md` (already fixed in WO3)
- **DO NOT** modify any files in `.agents/`, `template/`, or `modules/`
- **DO NOT** create branches or make git commits — the PA handles branching.

---

## Line-Item Checklist

### Part A — Index Files

- [ ] Create `docs/README.md`:
  ```markdown
  # ESP32 App Foundation — Documentation

  ## For Framework Developers (Building Apps ON the Framework)

  Start here: [Developer Guide](developer/README.md)

  ## Internal Reference

  - [Design Specification](design_specification.md) — architectural blueprint and ADRs
  - [OTA Developer Guide](OTADeveloperGuide.md) — OTA pipeline for build & release
  ```
  Adapt as needed based on what other files exist in `docs/`. List them.

- [ ] Create `docs/developer/README.md`:
  ```markdown
  # Developer Guide — Building on ESP32 App Foundation

  This guide walks you through building an application on the ESP32 App Foundation.
  Read the guides in order for the best experience.

  ## Reading Order

  1. [Architecture Overview](architecture-overview.md) — understand the framework's structure
  2. [Creating a Module](creating-a-module.md) — build your first module step by step
  3. [Configuration System](config-system.md) — persist and manage module settings
  4. [Extending the Web Portal](extending-the-web-portal.md) — add tabs and API endpoints
  5. [Event Bus](event-bus.md) — cross-module communication via publish/subscribe

  ## Quick Reference

  - Single include: `#include "AppFoundation.h"`
  - Registration: `app.registerModule(&yourModule);`
  - All modules use interface contracts from `include/interfaces/`
  ```

### Part B — `docs/developer/architecture-overview.md`

- [ ] Create `docs/developer/architecture-overview.md`
- [ ] Content MUST cover (verify all facts against source):

  1. **The 3-Tier Model**: Framework (`modules/`) → Interfaces (`include/interfaces/`) → Your App (`src/`)
  2. **Hollywood Principle**: The framework owns `setup()` and `loop()`. Your app
     never writes these. Show the actual `src/main.cpp` to prove it.
  3. **`AppFoundation.h` — the single include**: Show what it includes. Explain
     that downstream apps never include internal paths.
  4. **`registerModule<T>()` — the primary registration API**: Explain the
     compile-time interface detection. Show the before/after comparison.
  5. **Interface table**: List all extension interfaces with one-line descriptions:
     | Interface | Purpose |
     |---|---|
     | `ITickable` | Periodic non-blocking execution |
     | `IConfigurable` | Persistent configuration via ConfigManager |
     | `IWebContributor` | REST API routes + web portal tabs |
     | `IMCPContributor` | MCP JSON-RPC tool registration |
     | `IConfigGate` | Boot→Running state transition gates |
     | `IDataSource` | Background data fetching via DataManager |
     | `IStatusReporter` | Boot-phase display status updates |
     | `IDisplayBoard` | Display carousel board registration |
  6. **Boot lifecycle**: BOOT → CONFIGURING ↔ RUNNING. What happens at each phase.
     Read `Application.cpp begin()` for the actual boot sequence.
  7. **LittleFS directory layout**: `/mod/<namespace>.json` for config, `/www/` for
     web portal assets.
  8. **EventBus overview**: Brief intro — detailed guide is separate.

  Cross-reference: `docs/design_specification.md` §2 (Architecture) and §3 (Interfaces).

### Part C — `docs/developer/creating-a-module.md`

- [ ] Create `docs/developer/creating-a-module.md`
- [ ] Walk through the expanded template (`template/src/myApp.hpp` and `.cpp`)
  **line by line**. This is a tutorial, not a reference.
- [ ] Content MUST cover:

  1. **"Which interfaces should I implement?"** — decision guide based on what
     the module needs to do
  2. **Class declaration** — show the multiple inheritance pattern from the template
  3. **`registerModule()` in main.cpp** — one call does everything
  4. **`tick(uint32_t currentMillis)`** — the timing pattern, why `currentMillis`
     is injected (testability), the 10ms budget rule
  5. **Module checklist** — what every module needs:
     - Non-blocking `tick()` (no `delay()`)
     - Static or stack allocation preferred (avoid `new` in `tick()`)
     - House-style comments
     - `#ifndef NATIVE_BUILD` guards for hardware-specific code
  6. **Common mistakes** — `delay()` in tick, forgetting to guard hardware code,
     heap allocation in tick, exceeding the tick budget

### Part D — `docs/developer/config-system.md`

- [ ] Create `docs/developer/config-system.md`
- [ ] Content MUST cover (verify against `IConfigurable.hpp` and `ConfigManager.hpp`):

  1. **Config file ownership model** — `/mod/<namespace>.json`, one file per module
  2. **The five IConfigurable methods** — walkthrough of each:
     - `getConfigNamespace()` — returns the module's unique namespace string
     - `getSchemaVersion()` — integer version for migration support
     - `applyConfig(const JsonObject&)` — called at boot with persisted config
     - `serializeConfig(JsonObject&) const` — called when saving config
     - `resetConfig()` — called on factory reset, sets defaults
  3. **Registration** — automatic via `registerModule()` (if the module
     implements `IConfigurable`, it's auto-registered with ConfigManager)
  4. **Schema versioning** — `getSchemaVersion()` + `migrateConfig()`.
     Read `IConfigurable.hpp` for the migration contract.
  5. **ArduinoJson v7 patterns** — the `config["key"] | defaultValue` pattern.
     Read the template's `applyConfig()` for the exact pattern.
  6. **FrameworkConfig intent** — Document clearly:
     > `FrameworkConfig` exists solely to carry OTA compile-time secrets (GitHub
     > repo, public key, GitHub token) that must NOT be persisted to flash. All
     > other framework configuration goes through `ConfigManager` + `IConfigurable`.
  7. **ConfigManager API** — `saveModuleConfig()` for runtime saves.

### Part E — `docs/developer/extending-the-web-portal.md`

- [ ] Create `docs/developer/extending-the-web-portal.md`
- [ ] Content MUST cover:

  1. **`IWebContributor` interface walkthrough**:
     - `registerRoutes(AsyncWebServer& server)` — how to register API endpoints
     - `getTabName()` — the tab label in the portal nav
     - `getTabOrder()` — numeric ordering of tabs
  2. **The end-to-end pattern**: C++ route handler → JSON response → JS `fetch()`
     → DOM update. Walk through the template's example route and JS file.
  3. **HTML fragment pattern**: How the SPA shell (`app.js`) loads tab fragments
     from LittleFS. Reference the template's `tab.html` and `tab.js`.
  4. **API route conventions**: JSON endpoints, `AsyncWebServer` route registration,
     `request->send()` response pattern. Read existing route registrations in
     framework modules for the actual patterns.
  5. **The `/api/tabs` contract**: Read `WebServerManager.cpp` or `Application.cpp`
     to find how the tab list is built and served. Document the response format.
  6. **Authentication**: Brief mention that routes may require auth tokens when
     AuthManager is enabled. Reference the Security tab if applicable.

### Part F — `docs/developer/event-bus.md`

- [ ] Create `docs/developer/event-bus.md`
- [ ] Content MUST cover:

  1. **EventBus API**: `subscribe()`, `publish()`, `unsubscribe()`, `begin()`.
     Show exact function signatures from `EventBus.hpp`.
  2. **Module-owned topic conventions**: Explain the `namespace ModuleEvents {}`
     pattern from WO2. Show how a module declares and publishes topics.
  3. **Payload lifetime rule**: The `void* payload` pointer is valid ONLY during
     the synchronous callback. Subscribers must copy data they need to keep.
  4. **Framework-published topics reference table**: Read ALL modules for their
     event topic namespaces (created in WO2). Build a reference table:
     | Topic Constant | String Value | Publisher | Payload Type |
     |---|---|---|---|
  5. **How to declare your own events**: Template for app-specific topics.
  6. **Common patterns**: State change notifications, cross-module messaging.

---

## Verification Requirement

After completing all checklist items, verify:
1. All 7 files exist: `docs/README.md`, `docs/developer/README.md`, plus 5 guides
2. All internal cross-references use correct relative paths
3. All code examples are verified against actual source (not written from memory)
4. No source code was modified
5. Each guide has a coherent reading flow and follows tutorial prose style

**STOP** after verification and send a message back to the Principal Architect
reporting your results. Do not distill or commit until the PA approves.
