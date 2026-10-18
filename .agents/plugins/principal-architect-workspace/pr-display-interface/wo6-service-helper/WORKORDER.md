---
name: display-interface-wo6
description: Handoff instructions for Work Order 6 — DisplayServiceHelper extraction
complexity: standard
---

# WO6 — DisplayServiceHelper Extraction

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Extract duplicated contract-driven REST routes and MCP tools from `U8G2DisplayModule` and `LVGLDisplayModule` into a shared `DisplayServiceHelper` utility class in `lib/DisplayServiceHelper/`. Both modules compose the helper for common functionality, retaining only module-specific routes/tools.

## Context
`IDisplayModule` (`include/interfaces/IDisplayModule.hpp`) provides a unified contract. Both display modules independently implement identical web endpoints and MCP tools that simply delegate to contract methods. This WO extracts those into a reusable helper.

Read these files first to understand the contract surface and current implementations:
- `include/interfaces/IDisplayModule.hpp` — the interface
- `include/utils/BufferStream.h` — shared Print→vector utility
- `modules/U8G2Display/U8G2DisplayModule.cpp` lines 744–990 (registerRoutes) and 997–1182 (registerMCPTools)
- `modules/LVGLDisplay/LVGLDisplayModule.cpp` lines 687–866 (registerRoutes) and 873–1060 (registerMCPTools)

## Strict Boundary Box
- **DO NOT** modify `include/interfaces/IDisplayModule.hpp` — the contract is frozen
- **DO NOT** modify `web/portal/*.js` or `web/portal/*.html` — portal changes are WO7
- **DO NOT** modify `lib/PngWriter/` — it is complete
- **DO NOT** file issues or create branches

## Tooling Constraints
- Use the `platformio` MCP server's `build_project` tool for compilation verification
- Build targets: `esp32dev`, `esp32s3nano`, `crowpanel-128-rotary`
- Run native tests via `platformio` MCP `run_tests` tool with environment `native`

## Line-Item Instructions

### Create `lib/DisplayServiceHelper/`

- [ ] **`lib/DisplayServiceHelper/DisplayServiceHelper.h`** — Header file:
  ```cpp
  #pragma once
  class IDisplayModule;
  class PsychicHttpServer;
  class MCPToolRegistry;

  class DisplayServiceHelper {
  public:
      explicit DisplayServiceHelper(IDisplayModule* display);
      void registerCommonRoutes(PsychicHttpServer& server);
      void registerCommonMCPTools(MCPToolRegistry& registry);
  private:
      IDisplayModule* _display;
  };
  ```

- [ ] **`lib/DisplayServiceHelper/DisplayServiceHelper.cpp`** — Implementation. Extract these contract-driven endpoints from both modules:

  **Routes to extract (call IDisplayModule methods):**
  | Route | Method | Contract methods used |
  |---|---|---|
  | `GET /api/display/status` | GET | `getDisplayWidth/Height`, `getBrightness`, `isDimmed`, `isSleeping` |
  | `POST /api/display/brightness` | POST | `setBrightness`, `getBrightness` |
  | `POST /api/display/sleep` | POST | `resetActivity`, `isDimmed`, `isSleeping`, screensaver enable/disable, dim/blank timeouts |
  | `GET /api/display/screenshot` | GET | `capturePNG(Print&)` via `BufferStream` |

  **IMPORTANT for `/api/display/status`:** The helper's status response must include ONLY the contract-derived fields: `width`, `height`, `brightness`, `dimmed`, `sleeping`. Module-specific fields (like `flip`, `rotation`, `boardCount`, `activeBoard`, `clockFontSize`, `rotationInterval`) must NOT be in the helper — they will be added by each module's own specific routes.

  **IMPORTANT for `/api/display/sleep`:** The screensaver enable/disable and dim/blank timeout logic differs between U8G2 (`_screensaver.setEnabled()`, `_screensaver.setDimTimeout()`) and LVGL (similar but different object). The helper cannot directly manage screensaver config because `IDisplayModule` doesn't expose `setScreensaverEnabled()`, `setDimTimeout()`, or `setBlankTimeout()`. **Only extract the parts that use contract methods**: `resetActivity()` for wake, `isDimmed()`/`isSleeping()` for status. The screensaver config (enable/disable, timeouts) stays in each module's own routes.

  **MCP tools to extract:**
  | Tool | Contract methods used |
  |---|---|
  | `display_status` | Same as REST status (contract fields only) |
  | `capture_display` | `capturePNG` → base64 PNG |

  **IMPORTANT for MCP `display_control`:** The brightness set and wake/sleep subset CAN be extracted. But carousel ops (U8G2) and rotation ops (LVGL) stay module-specific. Extract a `display_control` tool with ONLY these parameters: `brightness` (int), `wake` (bool), `sleep` (bool). Each module can register additional parameters in its own `display_control` registration, or register a separate module-specific tool.

  Actually, **DO NOT extract `display_control`** — it's too intertwined with module-specific params. Each module keeps its own `display_control` tool entirely. Only extract `display_status` and `capture_display`.

### Refactor `U8G2DisplayModule`

- [ ] **`modules/U8G2Display/U8G2DisplayModule.hpp`** — Add `#include "DisplayServiceHelper.h"` and a `DisplayServiceHelper _serviceHelper{this};` member.

- [ ] **`modules/U8G2Display/U8G2DisplayModule.cpp`** — In `registerRoutes()`:
  1. Call `_serviceHelper.registerCommonRoutes(server);` at the top
  2. Remove the extracted routes (status, brightness, screenshot)
  3. Keep module-specific routes: `/api/display/flip`, `/api/display/boards`, `/api/display/show`, `/api/display/sleep` (the full screensaver config version — but rename sleep-specific parts to avoid conflict with the helper's wake-only route)
  4. **Actually, for sleep:** the helper registers a minimal `/api/display/sleep` that handles `wake`/`sleep` actions. The module adds its OWN version of `/api/display/sleep` that handles the FULL config (enable, timeouts). **This is a conflict.** Resolution: the helper does NOT register `/api/display/sleep`. Only `/api/display/status`, `/api/display/brightness`, and `/api/display/screenshot`. The module keeps full ownership of `/api/display/sleep`.

  In `registerMCPTools()`:
  1. Call `_serviceHelper.registerCommonMCPTools(registry);` at the top
  2. Remove `display_status` and `capture_display` registrations
  3. Keep `display_control` entirely

### Refactor `LVGLDisplayModule`

- [ ] **`modules/LVGLDisplay/LVGLDisplayModule.hpp`** — Add `#include "DisplayServiceHelper.h"` and a `DisplayServiceHelper _serviceHelper{this};` member.

- [ ] **`modules/LVGLDisplay/LVGLDisplayModule.cpp`** — Same refactor pattern as U8G2:
  1. `registerRoutes()`: call helper, remove status/brightness/screenshot, keep flip/rotation/sleep
  2. `registerMCPTools()`: call helper, remove display_status/capture_display, keep display_control

### Summary of route ownership after refactor

| Route | Owner |
|---|---|
| `GET /api/display/status` | Helper (contract fields only; module augments via its own status additions) |
| `POST /api/display/brightness` | Helper |
| `GET /api/display/screenshot` | Helper |
| `POST /api/display/sleep` | Module (full screensaver config) |
| `POST /api/display/flip` | Module (U8G2 + LVGL) |
| `GET /api/display/boards` | Module (U8G2 only) |
| `POST /api/display/show` | Module (U8G2 only) |
| `POST /api/display/rotation` | Module (LVGL only) |

**WAIT — there's a problem.** If the helper registers `GET /api/display/status` with only contract fields, and the module also needs to return module-specific fields (flip, rotation, boards, etc.) on the SAME endpoint, we have a conflict — two handlers for the same route.

**Resolution:** The helper provides a utility method instead:
```cpp
void populateCommonStatus(JsonDocument& doc);  // fills width, height, brightness, dimmed, sleeping
```
Each module calls this inside their OWN `/api/display/status` handler, then adds module-specific fields. Only `POST /api/display/brightness` and `GET /api/display/screenshot` are registered as full routes by the helper.

**Updated route table:**

| Route | Owner | Notes |
|---|---|---|
| `GET /api/display/status` | **Module** (calls `helper.populateCommonStatus()`) | Module adds its specific fields |
| `POST /api/display/brightness` | **Helper** | Pure contract: `setBrightness/getBrightness` |
| `GET /api/display/screenshot` | **Helper** | Pure contract: `capturePNG` |
| `POST /api/display/sleep` | **Module** | Full screensaver config |
| `POST /api/display/flip` | **Module** | |
| Other module-specific | **Module** | |

**Updated MCP tool table:**

| Tool | Owner | Notes |
|---|---|---|
| `display_status` | **Module** (calls `helper.populateCommonMCPStatus()`) | Module adds specific fields |
| `capture_display` | **Helper** | Pure contract: `capturePNG` → base64 |
| `display_control` | **Module** | Too intertwined to share |

So the helper's public API becomes:
```cpp
class DisplayServiceHelper {
public:
    explicit DisplayServiceHelper(IDisplayModule* display);

    // Route helpers
    void registerBrightnessRoute(PsychicHttpServer& server);
    void registerScreenshotRoute(PsychicHttpServer& server);
    void populateCommonStatus(JsonDocument& doc);

    // MCP helpers
    void registerCaptureTool(MCPToolRegistry& registry);
    void populateCommonMCPStatus(JsonObject& result);
};
```

Use this API. Each module calls the individual register methods and the populate helpers.

## Verification Requirement
1. Build all three targets: `esp32dev`, `esp32s3nano`, `crowpanel-128-rotary` — all must succeed
2. Run native tests — all must pass
3. Verify the web portal Display tab still works (the helper should be transparent to the portal JS)
4. Before committing, ask the user if they are happy with the changes

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Summary of Work
- Extracted `DisplayServiceHelper` into `lib/DisplayServiceHelper/` to provide common HTTP routes (`/api/display/brightness`, `/api/display/screenshot`) and MCP tools (`capture_display`).
- Provided utility methods (`populateCommonStatus`, `populateCommonMCPStatus`) for sharing display status state between the `U8G2DisplayModule` and `LVGLDisplayModule` and the helper class.
- Replaced the duplicate code in `U8G2DisplayModule` and `LVGLDisplayModule` with the shared implementation.
- Fixed config persistence regression in `DisplayServiceHelper` by using a `std::function` save callback pattern.
- Successfully verified build targets `esp32dev`, `esp32s3nano`, and `crowpanel-128-rotary`.
- Successfully verified `native` test suite passes.

### Discovered Issues / Concerns
- None discovered.
