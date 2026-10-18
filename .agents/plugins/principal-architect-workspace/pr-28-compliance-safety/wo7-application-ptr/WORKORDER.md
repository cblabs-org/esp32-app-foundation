---
name: pr28-wo7
description: Handoff instructions for Work Order 7 — apply Transient Service Locator pattern to OTAUpdateManager + delete orphaned IBoardLayout
complexity: architect
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

# Objective

1. Refactor `OTAUpdateManager` to use the **Transient Service Locator pattern**
   (ADR-39): keep `begin(Application& app)` but stop storing `Application*`.
   Instead, extract and store narrow references to the 3 services actually used.
2. Delete the orphaned `IBoardLayout` interface (no production code inherits
   from it — only test stubs).

# Background: ADR-39 — Transient Service Locator Pattern

Read the authoritative documentation in:
- `.agents/skills/framework-expert/coding-patterns/overview.md` (DI Conventions)
- `.agents/skills/framework-expert/coding-patterns/service-module-pattern.md`

**The pattern:** `Application&` is passed to `begin()` as a transient service
locator. The module extracts narrow refs to only the services it needs, then
discards the `Application` reference. `Application*` is **never stored as a
member**. `#include "Application.hpp"` appears only in the `.cpp`.

# Strict Boundary Box

- **DO** modify `OTAUpdateManager.hpp`, `OTAUpdateManager.cpp`,
  and delete `IBoardLayout.hpp`.
- **DO NOT** modify `Application.hpp` or `Application.cpp` — the `begin()`
  signature stays as `begin(Application& app)`, so the call site does not change.
- **DO NOT** touch TAG definitions, naming conventions, or `String` usage.
- **DO NOT** modify any other modules.

# Checklist

## A. OTAUpdateManager — Transient Service Locator Refactor

The module currently accesses `app_` for exactly 3 services:
- `app_->getConfig()` → `FrameworkConfig` (OTA secrets)
- `app_->getConfigManager()` → `ConfigManager` (config persistence + manifest)
- `app_->getEventBus()` → `EventBus` (progress events)

- [ ] **`modules/OTAUpdateManager/OTAUpdateManager.hpp`**
  - Keep the forward declaration: `class Application;`
  - Keep `begin()` signature: `void begin(Application& app);`
  - **Remove** the stored member:
    ```cpp
    // DELETE THIS LINE:
    Application*  app_          = nullptr;             // Composition root reference
    ```
  - **Add** narrow stored refs (using leading `_` prefix):
    ```cpp
    const FrameworkConfig* _config        = nullptr;
    ConfigManager*         _configManager = nullptr;
    EventBus*              _eventBus      = nullptr;
    ```
  - Remove `#include "Application.hpp"` from the header if present (it should
    only be in the `.cpp`)

- [ ] **`modules/OTAUpdateManager/OTAUpdateManager.cpp`**
  - Ensure `#include "Application.hpp"` is present at the top
  - Update `begin()` to extract narrow refs:
    ```cpp
    void OTAUpdateManager::begin(Application& app) {
        _config        = &app.getConfig();
        _configManager = &app.getConfigManager();
        _eventBus      = &app.getEventBus();
        // ... rest of existing begin() logic (unchanged)
    }
    ```
  - Replace **every** `app_->getConfig()` with `*_config`:
    - L274: `app_->getConfig().otaGithubRepo` → `_config->otaGithubRepo`
    - L284: `const FrameworkConfig& cfg = app_->getConfig();` → `const FrameworkConfig& cfg = *_config;`
    - L440, L975: same pattern
  - Replace **every** `app_->getConfigManager()` with `*_configManager`:
    - L319: `app_->getConfigManager().saveModuleConfig(this);` → `_configManager->saveModuleConfig(this);`
    - L664-665: `app_->getConfigManager().getManifest()...` → `_configManager->getManifest()...`
    - L878-879: same pattern
    - L1169, L1181, L1193, L1207: `this->app_->getConfigManager().saveModuleConfig(this);` → `_configManager->saveModuleConfig(this);`
  - Replace **every** `app_->getEventBus()` with `*_eventBus`:
    - L782: `app_->getEventBus().publish(...)` → `_eventBus->publish(...)`

## B. Delete Orphaned IBoardLayout

- [ ] **`modules/U8G2Display/boards/interfaces/IBoardLayout.hpp`** — **DELETE**
  this file entirely.
  - First verify: `grep -rn "IBoardLayout" modules/ lib/ include/ test/` —
    confirm no production code references it. If test stubs exist, update or
    remove them.
  - If any file `#include`s `IBoardLayout.hpp`, remove that include.

# Important Notes

- The `begin()` signature does NOT change — `Application.cpp` call site is
  untouched. This is the key advantage of the Transient Service Locator pattern.
- After this refactor, `OTAUpdateManager.hpp` forward-declares `Application`
  but never `#include`s it. The module has zero header-level coupling to the
  god-object.
- The `_config` pointer is valid for the device lifetime because
  `FrameworkConfig` is populated once in `setup()` and never mutated.

# Verification

Use the PlatformIO MCP server tools (not the CLI directly):
1. `build_project` with `environment: "native"` — compiles cleanly
2. `build_project` with `environment: "esp32dev"` — compiles cleanly
3. `run_tests` with `environment: "native"` — all tests pass

If any build fails, use `agent_build_diagnose` for structured error analysis.

Additionally, run this compliance check:
```bash
# Must return zero hits — no Application* stored in any module header
grep -rn "Application\*" modules/ --include="*.hpp"
```

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are
happy. Only after approval should you invoke the `@distillery` skill.
