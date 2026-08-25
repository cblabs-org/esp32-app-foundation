---
name: 148-web-tab-registry-wo1
description: Handoff instructions for Work Order 1 — TabRegistry type, IWebContributor extension, multi-tab module migrations
complexity: standard
---

# WO1 — TabRegistry Type + Multi-Tab Module Migrations

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Introduce the `TabRegistry` abstract class and extend `IWebContributor` with a `registerTabs(TabRegistry&)` method (with a backward-compatible default that reads existing scalar getters). Migrate the three multi-tab modules — `AuthManager`, `LVGLDisplayModule`, `U8G2DisplayModule` — to use `registerTabs()`, deleting all 8 inner stub structs. Remove manual stub registrations from `Application::begin()`. Update test stubs to remove deleted accessor calls.

This WO must compile and pass all tests — the backward-compatible default means unmigrated single-tab modules continue working. WO2 completes the clean break.

## Context
This is PR #150, stacked on PR #149 (`146-config-gate/wo7-consumer-cleanup`). The branch for this WO is `148-web-tab-registry/wo1-types-multi-tab` based on `146-config-gate/wo7-consumer-cleanup`.

**Reference implementations to follow:**
- `IPreconditionContributor` + `PreconditionRegistry` (`include/interfaces/IPreconditionContributor.hpp`, `include/utils/PreconditionRegistry.hpp`) — the newest registry pattern in the codebase, delivered by PR #149.
- `IMCPContributor` + `MCPToolRegistry` (`include/interfaces/IMCPContributor.hpp`, `modules/MCPServer/MCPToolRegistry.hpp`) — the original registry pattern.

**Issue #147 note:** The helpText borrowed-pointer bug (issue #147) is already fully resolved by PR #149. `_helpTextBuf` is already a member variable in `Application.hpp:505`. No additional work needed. Issue #147 will be closed with a comment noting PR #149 resolved it.

## Strict Boundary Box
- **DO NOT** remove scalar tab getters from `IWebContributor` — that is WO2's scope.
- **DO NOT** migrate single-tab modules (ConfigManager, KeyManager, WifiManager, etc.) — that is WO2.
- **DO NOT** refactor `WebServerManager`'s tab collection loop — that is WO2.
- **DO NOT** touch `getApiPrefix()` or `getMaxPostBody()` — these serve body-limit middleware and are retained.
- **DO NOT** modify any display rendering code, event payloads, or PreconditionEngine code.
- **DO NOT** create or modify any branches beyond what was pre-created by the PA.

## Tooling Constraints
- Use `platformio` MCP tools (`build_project`, `run_tests`) for build verification — not raw `pio` CLI commands.
- Use `check_project` or `agent_build_diagnose` if build issues arise.
- Commit messages should follow conventional commits: `feat(web):`, `refactor(web):`, etc.

## Line-Item Instructions

### 1. Create `TabRegistry.hpp`
- [ ] Create `include/utils/TabRegistry.hpp`
- Define `TabEntry` struct with fields: `name` (const char\*), `parent` (const char\*), `order` (int), `icon` (const char\*), `script` (const char\*).
- Define abstract `TabRegistry` class with a single pure virtual method:
  ```cpp
  virtual void addTab(const char* name,
                      const char* parent = nullptr,
                      int         order  = 100,
                      const char* icon   = nullptr,
                      const char* script = nullptr) = 0;
  ```
- All `const char*` parameters must have static or application lifetime (document in comment).
- Include a file-level Doxygen comment following the project's house style (see other headers in `include/utils/` for examples).

### 2. Extend `IWebContributor` with `registerTabs()`
- [ ] Modify `include/interfaces/IWebContributor.hpp`
- Add `#include "utils/TabRegistry.hpp"` (or forward-declare `TabRegistry` if you prefer — but the include is cleaner since the method signature uses it).
- Add a **virtual non-pure** method with a backward-compatible default implementation:
  ```cpp
  /**
   * @brief Register all navigation tabs this module contributes.
   *
   * Override to register multiple tabs from a single module.
   * Default reads scalar getters for backward compatibility;
   * will become pure virtual once all modules migrate.
   */
  virtual void registerTabs(TabRegistry& registry) {
      const char* name = getTabName();
      if (name) {
          registry.addTab(name, getParentTab(), getTabOrder(),
                          getTabIcon(), getTabScript());
      }
  }
  ```
- Do NOT remove or deprecate scalar getters yet — WO2 handles that.

### 3. Migrate `AuthManager`
- [ ] Modify `modules/AuthManager/AuthManager.hpp`
- **Delete** the `PasswordTabStub` struct (lines ~498–503).
- **Delete** the `TlsTabStub` struct (lines ~505–510).
- **Delete** the member instances `_passwordTabStub` and `_tlsTabStub` (lines ~512–513).
- **Delete** the `passwordTabStub()` accessor method (line ~277) and its Doxygen comment block (lines ~268–277).
- **Delete** the `tlsTabStub()` accessor method (line ~288) and its Doxygen comment block (lines ~279–288).
- **Delete** the section comment block above the stubs (lines ~490–496).
- **Add** a `registerTabs()` override in the public `IWebContributor` section:
  ```cpp
  void registerTabs(TabRegistry& registry) override {
      registry.addTab("Security", nullptr, 5, kLockIcon);
      registry.addTab("Password", "Security", 10);
      registry.addTab("TLS",      "Security", 20);
  }
  ```
  Note: find the existing `kLockIcon` or equivalent SVG string constant that `getTabIcon()` currently returns. If it's inline, extract it to a `static constexpr const char*` or keep it inline — match the existing style.
- Add `#include "utils/TabRegistry.hpp"` if not already included via `IWebContributor.hpp`.

### 4. Migrate `LVGLDisplayModule`
- [ ] Modify `modules/LVGLDisplay/LVGLDisplayModule.hpp`
- **Delete** the `SettingsTabStub` struct (lines ~397–402).
- **Delete** the `ScreenshotTabStub` struct (lines ~404–409).
- **Delete** the `HardwareContributor` struct (lines ~411–417).
- **Delete** the member instances for all three stubs.
- **Delete** the `settingsTabStub()`, `screenshotTabStub()`, `hardwareTabStub()` accessor methods.
- **Add** a `registerTabs()` override:
  ```cpp
  void registerTabs(TabRegistry& registry) override {
      registry.addTab("Display", nullptr, 50, /* existing SVG icon */, "/display.js");
      registry.addTab("Settings",   "Display", 10);
      registry.addTab("Screenshot", "Display", 20);
      registry.addTab("Hardware",   "Display", 30, nullptr, "/hardware.js");
  }
  ```
  Use the existing display SVG icon constant from `getTabIcon()`.

### 5. Migrate `U8G2DisplayModule`
- [ ] Modify `modules/U8G2Display/U8G2DisplayModule.hpp`
- **Delete** the `SettingsTabStub` struct (lines ~569–574).
- **Delete** the `ScreenshotTabStub` struct (lines ~576–581).
- **Delete** the `CarouselContributor` struct (lines ~583–588).
- **Delete** the member instances for all three stubs.
- **Delete** the `settingsTabStub()`, `screenshotTabStub()`, `carouselTabStub()` accessor methods.
- **Add** a `registerTabs()` override:
  ```cpp
  void registerTabs(TabRegistry& registry) override {
      registry.addTab("Display", nullptr, 50, /* existing SVG icon */, "/display.js");
      registry.addTab("Settings",   "Display", 10);
      registry.addTab("Screenshot", "Display", 20);
      registry.addTab("Carousel",   "Display", 30, nullptr, "/carousel.js");
  }
  ```
  Use the existing display SVG icon constant from `getTabIcon()`.

### 6. Remove stub registrations from `Application::begin()`
- [ ] Modify `modules/Application/Application.cpp`
- In `begin()` (around line 180), **delete** the two stub registration lines:
  ```cpp
  registerModule(_authManager.passwordTabStub());  // IWebContributor (sub-pill)
  registerModule(_authManager.tlsTabStub());       // IWebContributor (sub-pill)
  ```
- **Do not** remove `registerModule(&_authManager)` itself — that stays.

### 7. Update test stubs
- [ ] Modify `test/test_unit/test_u8g2Display/u8g2_module_native_stubs.cpp` — remove any calls to deleted stub accessor methods (`settingsTabStub()`, `screenshotTabStub()`, `carouselTabStub()`). If the stub file instantiates an `U8G2DisplayModule` and calls these, replace with equivalent `registerTabs()` calls or remove entirely if the stubs were only used for tab registration.
- [ ] Modify `test/test_unit/test_mcpServer/u8g2_module_mcp_stubs.cpp` — same as above.
- [ ] Check `test/test_unit/test_application/test_application.cpp` — if any tests reference `passwordTabStub()`, `tlsTabStub()`, or display stub accessors, update or remove them.

### 8. Build verification
- [ ] Run `build_project` for all hardware environments — must compile cleanly.
- [ ] Run `run_tests` for the `native` environment — all tests must pass (expect ≥ 828, matching PR #149 baseline).

## Verification Requirement
The worker must verify the project compiles and all tests pass using the `platformio` MCP tools. **CRITICAL:** Before committing, the worker MUST ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
✅ **COMPLETE** — All 8 checklist items implemented, all builds green, 828/828 native tests pass.

### Changes Made

| File | Action | Summary |
|---|---|---|
| `include/utils/TabRegistry.hpp` | **Created** | `TabEntry` struct + abstract `TabRegistry` with pure virtual `addTab()` |
| `include/interfaces/IWebContributor.hpp` | Modified | Added `#include "utils/TabRegistry.hpp"` + `registerTabs()` virtual with backward-compatible default |
| `modules/AuthManager/AuthManager.hpp` | Modified | Deleted `PasswordTabStub`/`TlsTabStub` inner structs, members, and accessors; added `registerTabs()` override |
| `modules/AuthManager/AuthManager.cpp` | Modified | Added `registerTabs()` impl: Security (parent, order=5, lock icon), Password (10), TLS (20) |
| `modules/LVGLDisplay/LVGLDisplayModule.hpp` | Modified | Deleted `SettingsTabStub`/`ScreenshotTabStub`/`HardwareContributor` inner structs, members, and accessors; added `registerTabs()` override |
| `modules/LVGLDisplay/LVGLDisplayModule.cpp` | Modified | Added `registerTabs()` impl: Display (parent, order=50, icon, `/display.js`), Settings (10), Screenshot (20), Hardware (30, `/hardware.js`) |
| `modules/U8G2Display/U8G2DisplayModule.hpp` | Modified | Deleted `SettingsTabStub`/`ScreenshotTabStub`/`CarouselContributor` inner structs, members, and accessors; added `registerTabs()` override |
| `modules/U8G2Display/U8G2DisplayModule.cpp` | Modified | Added `registerTabs()` impl: Display (parent, order=50, icon, `/display.js`), Settings (10), Screenshot (20), Carousel (30, `/carousel.js`) |
| `modules/Application/Application.cpp` | Modified | Removed 2 `registerModule()` calls for AuthManager password/TLS stubs |
| `src/main.cpp` | Modified | Removed 7 lines of display stub registrations (Settings, Screenshot, Carousel/Hardware) |
| `test/test_unit/test_u8g2Display/u8g2_module_native_stubs.cpp` | Modified | Added `registerTabs()` stub implementation |
| `test/test_unit/test_mcpServer/u8g2_module_mcp_stubs.cpp` | Modified | Added `registerTabs()` stub implementation |
| `test/test_unit/test_lvglDisplay/test_web_contributor.cpp` | Modified | Rewrote `TabMetadata` test to use `MockTabRegistry` instead of deleted stub accessors |

**Net diff:** 13 files changed, 174 insertions(+), 141 deletions(-)

### Decisions Made
1. **`src/main.cpp` cleanup** — The WORKORDER only mentioned `Application.cpp` for stub registration removal, but `src/main.cpp` also registered display tab stubs. Both were cleaned up to avoid build errors.
2. **Test stub updates** — The WORKORDER expected test stubs to need "no changes", but `test_web_contributor.cpp` directly tested via the deleted stub accessors. Introduced a lightweight `MockTabRegistry` to verify `registerTabs()` output instead.
3. **`TabEntry` as a plain struct** — Kept it simple with raw `const char*` pointers and documented the static-lifetime requirement, matching the existing pattern used by `PreconditionRegistry`.

### Dead Ends & Warnings
None.

### Out-of-Scope Findings
None — all changes stayed within the WO1 boundary box.

### Blockers
None.

### Verification

| Environment | Result | Duration |
|---|---|---|
| `esp32dev` | ✅ SUCCESS | 1m 03s |
| `esp32s3nano` | ✅ SUCCESS | 1m 41s |
| `crowpanel-128-rotary` | ✅ SUCCESS | 2m 17s |
| `native` (tests) | ✅ 828/828 PASSED | 36s |

**Commit:** `dd06d109` on branch `148-web-tab-registry/wo1-types-multi-tab`
