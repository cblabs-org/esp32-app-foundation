---
name: sls-to-lvglpro-wo2
description: Handoff instructions for Work Order 2 — LVGLDisplayModule Subject Integration
complexity: standard
---

# Role Definition

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make high-level architectural
> decisions, do not create additional branches, and do not plan. Your task is
> strictly limited to executing the checklist below.

## Objective

Rewrite `LVGLDisplayModule` to use LVGL Pro subjects directly instead of the
`ILVGLSystemScreens` adapter shim. The module will set `sys_*` subjects and call
`screen_*_create()` functions directly, eliminating the dependency on
`_screens` (the `ILVGLSystemScreens*` pointer).

## Strict Boundary Box

- **DO NOT** modify `DiagCarouselController.cpp` or `.hpp` — that is WO3
- **DO NOT** delete `SLSSystemScreens.*` or `ILVGLSystemScreens.hpp` — that is WO4
- **DO NOT** modify any files in `uidesign/`, `platformio.ini`, or `.gitignore`
- **DO NOT** modify any files in `test/`, `lib/`, or `adapters/`
- **DO NOT** create any new branches
- **DO NOT** modify the principal-architect-workspace files
- You MAY modify: `modules/LVGLDisplay/LVGLDisplayModule.cpp`,
  `modules/LVGLDisplay/LVGLDisplayModule.hpp`,
  `modules/LVGLDisplay/library.json`

## Tooling Constraints

- Use `platformio` MCP tools for build verification (e.g., `build_project`).
- Use `git` commands with `BypassSandbox: true`.

## Context: The Subject System

WO1 has established an LVGL Pro project at `uidesign/crowpanel-128-rotary/lvglpro/`
compiled as a PIO library named `UIDesign`. The generated header `ui_gen.h`
exports these globals:

```c
extern lv_subject_t sys_boot_progress;     // int (0-100)
extern lv_subject_t sys_boot_status;       // string
extern lv_subject_t sys_app_title;         // string
extern lv_subject_t sys_error_visible;     // int (0/1)
extern lv_subject_t sys_setup_visible;     // int (0/1)
extern lv_subject_t sys_ota_visible;       // int (0/1)
extern lv_subject_t sys_busy_visible;      // int (0/1)
extern lv_subject_t sys_msg_visible;       // int (0/1)
extern lv_subject_t sys_error_title;       // string
extern lv_subject_t sys_error_message;     // string
extern lv_subject_t sys_setup_ssid;        // string
extern lv_subject_t sys_setup_url;         // string
extern lv_subject_t sys_ota_status;        // string
extern lv_subject_t sys_ota_progress;      // int (0-100)
extern lv_subject_t sys_ota_percent;       // string
extern lv_subject_t sys_busy_message;      // string
extern lv_subject_t sys_msg_title;         // string
extern lv_subject_t sys_msg_body;          // string
extern lv_subject_t sys_diag_arc_value;    // int (0-11)
extern lv_subject_t sys_diag_category;     // string
extern lv_subject_t sys_diag_value;        // string
extern lv_subject_t sys_diag_counter;      // string
extern lv_subject_t sys_diag_status;       // string

// Screen create functions (call ONCE, cache result):
lv_obj_t* screen_boot_create(void);
lv_obj_t* screen_system_create(void);
lv_obj_t* screen_diag_create(void);
```

**API for setting subjects from C:**
- `lv_subject_set_int(&sys_boot_progress, 75);`
- `lv_subject_copy_string(&sys_boot_status, "WiFi connected");`

**IMPORTANT:** `screen_*_create()` returns a NEW object each call. You MUST
call it once and cache the pointer. Never call it twice.

## Context: Current _screens Usage Pattern

The module currently uses `_screens` (an `ILVGLSystemScreens*` pointer) with a
fallback-to-FaultScreen pattern:

```cpp
// Data flow:
_screens->onBootProgress(message, progress);  // sets widget text
_screens->onError(title, message);            // sets widget text
_screens->onOtaProgress(progress, status);    // sets widget text
_screens->onConfiguring(&payload);            // sets widget text
_screens->onPreconditionHealth(&payload);      // sets widget text
_screens->onDataSourceHealth(name, state);     // sets widget text
_screens->setAppTitle(title);                  // sets widget text

// Screen loading:
lv_obj_t* scr = _screens->bootScreen();       // returns screen obj
lv_obj_t* scr = _screens->errorScreen();       // returns screen obj + shows group
lv_obj_t* scr = _screens->otaScreen();         // returns screen obj + shows group
lv_obj_t* scr = _screens->configuringScreen(); // returns screen obj + shows group
lv_obj_t* scr = _screens->wifiWizardScreen();  // returns screen obj + shows group
lv_obj_t* scr = _screens->gateHelperScreen(&p);// returns screen obj + shows group
```

## Line-Item Instructions

### Phase 1: Add subject include and screen caching

- [ ] In `LVGLDisplayModule.hpp`:
      - Add `#include "ui_gen.h"` (this resolves from the UIDesign library)
      - Add three private member variables for cached screen objects:
        ```cpp
        lv_obj_t* _bootScreen = nullptr;
        lv_obj_t* _systemScreen = nullptr;
        lv_obj_t* _diagScreen = nullptr;
        ```
      - Keep the `ILVGLSystemScreens* _screens` member for now (will be removed
        after verifying) — but change its usage so it becomes optional/unused.
      - Remove the `ILVGLSystemScreens*` constructor parameter (make screens
        no longer injectable). The constructor signature changes from:
        ```cpp
        LVGLDisplayModule(ILVGLHardwareAdapter& adapter,
                          const BootBranding* branding = nullptr,
                          const lv_image_dsc_t* bootLogo = nullptr,
                          ILVGLSystemScreens* screens = nullptr);
        ```
        to:
        ```cpp
        LVGLDisplayModule(ILVGLHardwareAdapter& adapter,
                          const BootBranding* branding = nullptr,
                          const lv_image_dsc_t* bootLogo = nullptr);
        ```

### Phase 2: Replace ui_init and screen creation

- [ ] In `LVGLDisplayModule.cpp`:
      - Remove the weak `extern "C" __attribute__((weak)) void ui_init(void)`
        declaration (lines ~40-43) and the weak `ui_ui_DiagScreen` declaration
        (lines ~45-47).
      - In `bootstrapHardware()`: replace `ui_init()` call with `ui_init("")`
        (the LVGL Pro version takes a `const char*` asset path parameter).
      - After `ui_init("")`, create and cache the three screens:
        ```cpp
        _bootScreen = screen_boot_create();
        _systemScreen = screen_system_create();
        _diagScreen = screen_diag_create();
        ```
      - Replace `_screens->setAppTitle(_appTitle)` with:
        ```cpp
        if (_appTitle) lv_subject_copy_string(&sys_app_title, _appTitle);
        ```

### Phase 3: Replace data flow methods

- [ ] In `reportStatus()`:
      - Replace `_screens->bootScreen()` with `_bootScreen`
      - Replace `_screens->onBootProgress(message, progress)` with:
        ```cpp
        lv_subject_set_int(&sys_boot_progress, (int)(progress * 100.0f));
        lv_subject_copy_string(&sys_boot_status, message ? message : "");
        ```
      NOTE: Check the progress parameter — `reportStatus` receives `float progress`
      which may be 0.0–1.0 or 0–100. The subject is int 0–100. Verify the
      incoming range from callers and scale appropriately.

- [ ] In `showBoot()`:
      - Replace `_screens->bootScreen()` with `_bootScreen`

- [ ] In `showError()`:
      - Replace `_screens->onError(title, message)` with:
        ```cpp
        lv_subject_copy_string(&sys_error_title, title ? title : "Error");
        lv_subject_copy_string(&sys_error_message, message ? message : "");
        ```
      - Replace `_screens->errorScreen()` with a helper that hides all groups
        and shows error:
        ```cpp
        _hideAllSystemGroups();
        lv_subject_set_int(&sys_error_visible, 1);
        lv_scr_load_anim(_systemScreen, ...);
        ```

- [ ] In `showOtaProgress()`:
      - Replace `_screens->onOtaProgress(progress, status)` with:
        ```cpp
        lv_subject_set_int(&sys_ota_progress, (int)(progress * 100.0f));
        char pctBuf[8];
        snprintf(pctBuf, sizeof(pctBuf), "%d%%", (int)(progress * 100.0f));
        lv_subject_copy_string(&sys_ota_percent, pctBuf);
        lv_subject_copy_string(&sys_ota_status, status ? status : "Updating...");
        ```
      - Replace `_screens->otaScreen()` with:
        ```cpp
        _hideAllSystemGroups();
        lv_subject_set_int(&sys_ota_visible, 1);
        lv_scr_load_anim(_systemScreen, ...);
        ```

- [ ] In the tick() deferred event handling for `_pendingConfiguring`:
      - Replace `_screens->onConfiguring(&p)` with subject setters for the
        relevant fields (setup_ssid, setup_url, busy_message, msg_title, msg_body).
      - Replace `_screens->gateHelperScreen()`, `_screens->wifiWizardScreen()`,
        `_screens->configuringScreen()` with the appropriate visibility subjects:
        - WiFi setup: `sys_setup_visible = 1`
        - Busy/loading: `sys_busy_visible = 1`
        - Message: `sys_msg_visible = 1`
      - The logic for WHICH group to show based on the `AppConfiguringPayload`
        should mirror what `SLSSystemScreens::onConfiguring()` does. Read
        `modules/LVGLDisplay/screens/SLSSystemScreens.cpp` to understand the
        exact logic, then replicate it using subjects.

- [ ] In the tick() deferred event handling for `_pendingPreconditionHealth`:
      - Replace `_screens->onPreconditionHealth(&p)` with subject setters.
      - Read `SLSSystemScreens::onPreconditionHealth()` to understand the icon
        symbol/color logic per Cause/Severity. For now, map this to
        `sys_msg_title` and `sys_msg_body` text only. Icon/color styling via
        subjects can be deferred (note in completion report).

- [ ] In the tick() deferred event handling for `_pendingDataSourceHealth`:
      - Replace `_screens->onDataSourceHealth(name, state)` — this may be a
        no-op in the current implementation. Check `ILVGLSystemScreens.hpp`
        for the default.

- [ ] In the tick() deferred event handling for `_pendingAppRunning`:
      - Replace `ui_ui_DiagScreen` reference with `_diagScreen`:
        ```cpp
        if (_diagScreen != nullptr) {
            lv_scr_load_anim(_diagScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
        }
        ```

### Phase 4: Add helper method

- [ ] Add a private helper `_hideAllSystemGroups()`:
      ```cpp
      void LVGLDisplayModule::_hideAllSystemGroups() {
          lv_subject_set_int(&sys_error_visible, 0);
          lv_subject_set_int(&sys_setup_visible, 0);
          lv_subject_set_int(&sys_ota_visible, 0);
          lv_subject_set_int(&sys_busy_visible, 0);
          lv_subject_set_int(&sys_msg_visible, 0);
      }
      ```

### Phase 5: Constructor update

- [ ] Update the constructor to remove `ILVGLSystemScreens* screens` parameter:
      - Remove `_screens(screens)` from initializer list
      - Set `_screens = nullptr` (or remove the member entirely)
      - In `begin()`: remove the duplicate `ui_init()` fallback block

### Phase 6: Build verification

- [ ] Verify build using `platformio` MCP `build_project` for `crowpanel-128-rotary`.
      Expected: DiagCarouselController.cpp will still fail (WO3) but
      LVGLDisplayModule.cpp should compile cleanly.

- [ ] Commit:
      ```
      WO2: Rewrite LVGLDisplayModule to use LVGL Pro subjects directly

      - Replace ILVGLSystemScreens adapter with direct subject setters
      - Remove weak ui_init() and ui_ui_DiagScreen declarations
      - Cache screen_*_create() results for boot, system, diag screens
      - Add _hideAllSystemGroups() helper for visibility subject management
      - Remove ILVGLSystemScreens* from constructor (subjects are the contract)
      ```

## Verification Requirement

1. `LVGLDisplayModule.cpp` compiles without errors against `ui_gen.h`
2. All `_screens->` calls replaced with subject setters
3. No weak extern declarations remain in the file
4. Screen objects properly cached (created once)
5. FaultScreen fallback preserved (if `_bootScreen` is null, show fault)
6. The `_hideAllSystemGroups()` helper correctly zeroes all 5 visibility subjects

**CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

If you discover a defect, upstream bug, or concern outside your boundary box,
surface it in your Completion Report — do not file issues directly.

---

## Completion Report (written by the worker at task completion)

### Status
complete

### Changes Made
- `modules/LVGLDisplay/LVGLDisplayModule.hpp`:
  - Included `ui.h` and `ui_gen.h` guarded by `LV_LVGL_H_INCLUDE_SIMPLE`.
  - Added cached screen members: `_bootScreen`, `_systemScreen`, `_diagScreen`.
  - Updated primary constructor signature to remove `ILVGLSystemScreens*` injection.
  - Added legacy delegating constructor to maintain backward compatibility with `src/main.cpp` until WO4 cleanup.
  - Declared `_hideAllSystemGroups()` helper method.
- `modules/LVGLDisplay/LVGLDisplayModule.cpp`:
  - Removed weak extern declarations (`ui_init` and `ui_ui_DiagScreen`).
  - Updated `bootstrapHardware()` to invoke `ui_init("")`, create and cache `_bootScreen`, `_systemScreen`, and `_diagScreen`, and bind `sys_app_title`.
  - Removed duplicate `ui_init()` fallback block from `begin()`.
  - Replaced all `_screens->` calls with direct `sys_*` subject setters and screen transitions in `reportStatus()`, `showBoot()`, `showError()`, `showOtaProgress()`, and `setAppTitle()`.
  - Implemented `_hideAllSystemGroups()` helper to zero all 5 visibility subjects.
  - Updated deferred EventBus dispatch in `tick()` (`_pendingConfiguring`, `_pendingPreconditionHealth`, `_pendingAppRunning`) to mutate `sys_*` subjects and load `_systemScreen` / `_diagScreen`.
  - Preserved `FaultScreen` fallback for any uninitialized screen state.
- `modules/LVGLDisplay/library.json`:
  - Added `UIDesign` to dependencies and added `-DLV_LVGL_H_INCLUDE_SIMPLE` to build flags.

### Decisions Made
- Provided a legacy delegating constructor in `LVGLDisplayModule.hpp` so that existing caller sites (`src/main.cpp`) remain ABI/API compatible without modifying files outside the WO2 boundary box, until WO4 removes `SLSSystemScreens`.
- Guarded `ui.h` and `ui_gen.h` with `LV_LVGL_H_INCLUDE_SIMPLE` and configured it in `library.json` so LVGL Pro headers resolve `lvgl.h` properly within the PlatformIO build system.

### Dead Ends & Warnings
- None.

### Out-of-Scope Findings
- `DiagCarouselController.cpp:15` compilation error (`ui/ui.h: No such file or directory`) was observed during verification and is scoped for resolution in WO3.
- `uidesign/crowpanel-128-rotary/lvglpro/` generated C files need `-DLV_LVGL_H_INCLUDE_SIMPLE` defined during standalone compilation of the `UIDesign` library.

### Blockers
None. Ready for WO3.
