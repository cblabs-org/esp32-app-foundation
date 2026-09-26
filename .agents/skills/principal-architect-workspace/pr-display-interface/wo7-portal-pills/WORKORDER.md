---
name: display-interface-wo7
description: Handoff instructions for Work Order 7 — Portal pill restructure and diagnostics drawer
complexity: standard
---

# WO7 — Portal Pill Restructure & Diagnostics Drawer

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Restructure the Display portal tab's sub-pills so that contract-driven controls live in the helper-owned "Settings" pill, while module-specific controls live in module-contributed pills: "Carousel" (U8G2) and "Hardware" (LVGL). Add a Display Diagnostics info drawer to the Settings pill.

## Context
After WO6, the `DisplayServiceHelper` handles common REST routes (brightness, screenshot) and provides `populateCommonStatus()`. But the portal JS (`display.js`) still mixes contract-driven and module-specific controls in one Settings pill.

Read the design system skill for UI guidance:
- `.agents/skills/framework-portal-design/readme.md` — full DS reference
- `.agents/skills/framework-portal-design/components/navigation/Drawer.prompt.md` — Drawer component
- `.agents/skills/framework-portal-design/components/data-display/InfoRow.prompt.md` — InfoRow component

Read the current portal code:
- `web/portal/display.js` — current Settings tab (to be cleaned)
- `web/portal/screenshot.js` — Screenshot tab (unchanged)
- `web/portal/index.html` — tab container divs

Read the module headers to understand the `IWebContributor` sub-pill pattern:
- `modules/U8G2Display/U8G2DisplayModule.hpp` — nested `SettingsContributor`, `ScreenshotContributor`
- `modules/LVGLDisplay/LVGLDisplayModule.hpp` — same pattern
- `include/interfaces/IWebContributor.hpp` — tab contribution contract

## Strict Boundary Box
- **DO NOT** modify `lib/DisplayServiceHelper/` — it is complete from WO6
- **DO NOT** modify `lib/PngWriter/` — it is complete
- **DO NOT** modify `include/interfaces/IDisplayModule.hpp` — the contract is frozen
- **DO NOT** modify `screenshot.js` — it is complete
- **DO NOT** file issues or create branches

## Tooling Constraints
- Use the `platformio` MCP server's `build_project` tool for compilation verification
- Build targets: `esp32dev`, `esp32s3nano`, `crowpanel-128-rotary`

## Line-Item Instructions

### 1. Clean `display.js` (Settings pill — contract-only)

- [ ] **`web/portal/display.js`** — Remove the **Flip Screen toggle** from the Settings tab scaffold. The flip toggle belongs in the module-specific Carousel pill (U8G2) or Hardware pill (LVGL). After removal, the Settings tab should contain ONLY:
  - Brightness slider (contract: `setBrightness`/`getBrightness`)
  - Screensaver section (enable toggle, dim delay, blank delay, wake/sleep buttons)
  - **NEW: Display Diagnostics drawer** (see below)

- [ ] **Add Display Diagnostics info drawer** to `display.js`, at the bottom of the Settings tab, after the Screensaver card. Use a native `<details>` element styled per the design system's Drawer component (`variant="info"`):
  ```html
  <details class="diag-drawer">
    <summary>Display Diagnostics</summary>
    <div class="diag-content">
      <!-- InfoRow key-value pairs -->
    </div>
  </details>
  ```

  The drawer fetches data from the existing `/api/display/status` endpoint. Render these rows:
  | Label | Value source | Format |
  |---|---|---|
  | Resolution | `status.width` × `status.height` | `"128 × 64 px"` (monospace for numbers) |
  | Brightness | `status.brightness` | `"210 / 255"` |
  | Screensaver | `status.dimmed` / `status.sleeping` | `"Active"` / `"Dimmed"` / `"Sleeping"` |

  Conditionally render module-specific rows (only if the field exists in the status JSON):
  | Label | Condition | Value |
  |---|---|---|
  | Active Board | `status.activeBoard !== undefined` | `"WeatherBoard (3 of 5)"` |
  | Rotation | `status.rotation !== undefined` | `"0°"` / `"90°"` / `"180°"` / `"270°"` |
  | Flip | `status.flip !== undefined` | `"Yes"` / `"No"` |

  Style the drawer per DS guidelines:
  - Collapsed by default
  - Neutral border (`var(--border)`), `border-radius: 10px`
  - Summary text: `font-weight: 600`, `color: var(--text-muted)`
  - InfoRow labels: `font-size: 0.8rem`, `color: var(--text-muted)`
  - InfoRow values: `font-family: "Courier New", monospace` for numeric values

### 2. Create `carousel.js` (U8G2-only pill)

- [ ] **`web/portal/carousel.js`** — New file. Contains the U8G2 Carousel sub-pill content:
  - **Flip Screen toggle** (moved from display.js) — `POST /api/display/flip`
  - **Board list** — `GET /api/display/boards`, rendered as a card with active board highlighted
  - **Force-show board** — buttons to select a specific board via `POST /api/display/show`
  - **Rotation interval** — display/edit the carousel rotation interval

  **NOTE:** Clock font size is NOT here — it is in the App tab, not the Display tab.

  Follow the same IIFE pattern as `display.js` — wrap everything in `(function() { ... })();`
  Listen for the `tab-changed` event for the "Carousel" tab to initialize.

### 3. Create `hardware.js` (LVGL-only pill)

- [ ] **`web/portal/hardware.js`** — New file. Contains the LVGL Hardware sub-pill content:
  - **Rotation selector** — 4-button group (0° / 90° / 180° / 270°) calling `POST /api/display/rotation`
  - **Flip toggle** (LVGL version — calls `POST /api/display/flip` which maps to `setRotation(flip ? 180 : 0)`)

  Same IIFE pattern, same `tab-changed` event pattern.

### 4. Add tab container divs to `index.html`

- [ ] **`web/portal/index.html`** — Add container divs for the new sub-pills:
  ```html
  <!-- Display sub-pill: Carousel (populated by carousel.js, U8G2 only) -->
  <div id="tab-carousel" class="tab-content">
  </div>
  <!-- Display sub-pill: Hardware (populated by hardware.js, LVGL only) -->
  <div id="tab-hardware" class="tab-content">
  </div>
  ```

### 5. Register new sub-pill contributors in display modules

- [ ] **`modules/U8G2Display/U8G2DisplayModule.hpp`** — Add a new nested `CarouselContributor`:
  ```cpp
  class CarouselContributor : public IWebContributor {
  public:
      const char* getTabName() const override { return "Carousel"; }
      const char* getParentTab() const override { return "Display"; }
      int getTabOrder() const override { return 30; }
      const char* getTabScript() const override { return "/carousel.js"; }
      void registerRoutes(PsychicHttpServer& server) override {}  // Routes on main module
  };
  CarouselContributor _carouselContributor;
  ```
  Register it in setup/begin via `app.registerModule(&_carouselContributor)` — or expose a getter so the consumer can register it alongside the display module.

  **Actually** — the existing pattern has the display module itself register child contributors internally during `begin()` by calling `_application->registerModule()`. Check how `SettingsContributor` and `ScreenshotContributor` are currently registered. Follow the same pattern.

- [ ] **`modules/LVGLDisplay/LVGLDisplayModule.hpp`** — Add a new nested `HardwareContributor`:
  ```cpp
  class HardwareContributor : public IWebContributor {
  public:
      const char* getTabName() const override { return "Hardware"; }
      const char* getParentTab() const override { return "Display"; }
      int getTabOrder() const override { return 30; }
      const char* getTabScript() const override { return "/hardware.js"; }
      void registerRoutes(PsychicHttpServer& server) override {}
  };
  HardwareContributor _hardwareContributor;
  ```

### 6. Move flip-related routes

- [ ] **`modules/U8G2Display/U8G2DisplayModule.cpp`** — The `/api/display/flip` route stays where it is (it's already module-specific). No change needed.

- [ ] **`modules/LVGLDisplay/LVGLDisplayModule.cpp`** — Same — `/api/display/flip` and `/api/display/rotation` stay where they are.

### 7. Documentation Updates

Update all docs that reference the display module's web/MCP ownership or the portal sub-pill structure.

#### Framework-Expert Skill Docs

- [ ] **`.agents/skills/framework-expert/systems/display/overview.md`**:
  - Add mention of `DisplayServiceHelper` as the shared contract-driven web/MCP provider
  - Line 16: clarify that `display_status` and `capture_display` are provided via `DisplayServiceHelper`; `display_control` remains module-specific
  - Add a bullet in the Multi-Interface Pattern section for `DisplayServiceHelper`

- [ ] **`.agents/skills/framework-expert/systems/display/lvgl-display.md`**:
  - Lines 13-14: clarify that common routes (brightness, screenshot) come from `DisplayServiceHelper`, module adds rotation/flip routes
  - `IMCPContributor` line: note `display_status`/`capture_display` via helper, `display_control` module-specific
  - Add mention of **Hardware** sub-pill for rotation settings

- [ ] **`.agents/skills/framework-expert/systems/display/replacing-the-display.md`** — **CRITICAL UPDATE**:
  - Section "Which Interfaces": add that composing `DisplayServiceHelper` gives you brightness/screenshot/capture routes for free
  - Section "Headless Operation" (lines 52-57): **Remove** the stale `#define HEADLESS_MODE` and `NullDisplayManager` advice. Replace with: "Simply don't register a display module. `Application` uses `NullStatusReporter` to absorb boot-phase calls. No `list_tools` entry for display tools. No Display tab in portal."
  - Add a new subsection showing the `DisplayServiceHelper` composition pattern for new display modules

- [ ] **`.agents/skills/framework-expert/systems/mcp-server/overview.md`** (line 30):
  - Update MCP tool ownership table: `display_status` and `capture_display` → "via DisplayServiceHelper", `display_control` → "module-specific"

- [ ] **`.agents/skills/framework-expert/systems/web-portal/routes.md`** (line 86):
  - `capture_display` is listed under "Built-in MCP Tool Endpoints" — clarify it's now provided by `DisplayServiceHelper` (not built-in, not per-module)

#### Developer Docs

- [ ] **`docs/developer/using-optional-modules.md`** (lines 100-137):
  - Update the Interfaces table (line 108): add `capture_display` to IMCPContributor, note `DisplayServiceHelper` provides common tools
  - Update Sub-Object Registration section: add `CarouselContributor` (U8G2) and `HardwareContributor` (LVGL) to the code examples
  - Update the registration code example to show the new sub-pills:
    ```cpp
    app.registerModule(display.settingsTabStub());      // Settings sub-pill (generic)
    app.registerModule(display.screenshotTabStub());    // Screenshot sub-pill (generic)
    app.registerModule(display.carouselTabStub());      // Carousel sub-pill (U8G2 only)
    // OR for LVGL:
    app.registerModule(display.hardwareTabStub());      // Hardware sub-pill (LVGL only)
    ```

- [ ] **`docs/developer/mcp-server.md`** (line 183):
  - Update note to say `capture_display` is provided via `DisplayServiceHelper` (composed by display modules)

## Verification Requirement
1. Build all three targets: `esp32dev`, `esp32s3nano`, `crowpanel-128-rotary` — all must succeed
2. The Display tab should show:
   - Settings pill: brightness, screensaver, diagnostics drawer (no flip)
   - Screenshot pill: unchanged
   - Carousel pill: visible only on U8G2 builds (flip, boards, rotation interval)
   - Hardware pill: visible only on LVGL builds (rotation selector)
3. Grep the framework-expert skill for stale references to `DisplayInfo`, `setDisplayInfo`, and the old "Built-in MCP Tool" listing of `capture_display`
4. Before committing, ask the user if they are happy with the changes

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

- Restructured the Display portal tab's sub-pills into Settings, Screenshot, Carousel, and Hardware.
- Moved contract-driven controls into the generic Settings tab and integrated a Display Diagnostics `<details>` drawer.
- Shifted hardware-specific UI controls (flip, board lists, interval, rotation) into `carousel.js` (U8G2) and `hardware.js` (LVGL) scripts.
- Updated the C++ display modules to dynamically provide the `CarouselContributor` and `HardwareContributor` stubs.
- Ensured all display-related documentation across `framework-expert` and `developer` accurately reflects the new architecture and correctly describes headless operation.
- Validated via `pio run` builds against `esp32dev`, `esp32s3nano`, and `crowpanel-128-rotary` targets, and successfully ran playwright automated web portal tests.
