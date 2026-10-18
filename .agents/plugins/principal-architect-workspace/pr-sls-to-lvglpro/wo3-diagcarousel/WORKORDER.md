---
name: sls-to-lvglpro-wo3
description: Handoff instructions for Work Order 3 — DiagCarouselController Rebinding
complexity: standard
---

# Role Definition

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make high-level architectural
> decisions, do not create additional branches, and do not plan. Your task is
> strictly limited to executing the checklist below.

## Objective

Rebind `DiagCarouselController` from SLS weak widget pointers to LVGL Pro
subjects for text/value data, while keeping direct widget access for styling
(arc indicator colors, font sizes) via the `lv_obj_t*` returned by
`screen_diag_create()`.

## Strict Boundary Box

- **DO NOT** modify `LVGLDisplayModule.cpp` or `.hpp` — that was WO2
- **DO NOT** delete `SLSSystemScreens.*` or `ILVGLSystemScreens.hpp` — that is WO4
- **DO NOT** modify files in `uidesign/`, `platformio.ini`, `.gitignore`, `test/`, `lib/`
- **DO NOT** create any new branches
- **DO NOT** modify the principal-architect-workspace files
- You MAY modify: `modules/LVGLDisplay/DiagCarouselController.cpp`,
  `modules/LVGLDisplay/DiagCarouselController.hpp`

## Tooling Constraints

- Use `platformio` MCP tools for build verification.
- Use `git` commands with `BypassSandbox: true`.

## Context: Available Subjects and Widgets

WO1 established these subjects in `ui_gen.h`:

```c
extern lv_subject_t sys_diag_arc_value;    // int (0-11) — arc position
extern lv_subject_t sys_diag_category;     // string — metric name
extern lv_subject_t sys_diag_value;        // string — reading text
extern lv_subject_t sys_diag_counter;      // string — "3 / 12"
extern lv_subject_t sys_diag_status;       // string — health label
```

**API:**
- `lv_subject_set_int(&sys_diag_arc_value, index);`
- `lv_subject_copy_string(&sys_diag_category, text);`

The LVGL Pro screen XML binds these subjects to widgets using `bind_value`
and `bind_text`, so setting a subject automatically updates the widget text.

**However**, the DiagCarousel also needs direct widget access for:
1. Arc indicator **color** — `lv_obj_set_style_bg_color()` on the arc
2. Status text **color** — `lv_obj_set_style_text_color()` on the status label
3. Value label **font size** — `lv_obj_set_style_text_font()` (large for numeric, small for text)

For this, the widget `name` attributes in the XML can be used with LVGL's
`lv_obj_get_child_by_id()` API. Check the generated header `ui_gen.h` for
the widget name declarations — they're likely generated as `extern lv_obj_t*`
globals similar to the old SLS pattern. **Verify this first** by reading
`uidesign/crowpanel-128-rotary/lvglpro/screens/screen_diag_gen.h` and
`screen_diag_gen.c` to see how widgets with `name` attributes are exported.

If widgets named `diag_arc`, `diag_category`, etc. exist as `lv_obj_t*` globals,
you can reference them directly for styling. If not, you'll need to find them
by traversing the screen object from `screen_diag_create()`.

## Current Code Structure

The file currently:
1. Declares 5 weak extern SLS widget pointers (lines 31-35):
   ```cpp
   __attribute__((weak)) extern lv_obj_t* ui_ui_DiagArc;
   __attribute__((weak)) extern lv_obj_t* ui_ui_DiagCategory;
   __attribute__((weak)) extern lv_obj_t* ui_ui_DiagValue;
   __attribute__((weak)) extern lv_obj_t* ui_ui_DiagCounter;
   __attribute__((weak)) extern lv_obj_t* ui_ui_DiagStatus;
   ```
2. In `begin()`: configures the arc range, registers callback, enters EDIT mode
3. In `updateWidgets()`: sets text via `lv_label_set_text()` and colors via
   `lv_obj_set_style_*()` on the widget pointers
4. In `refreshValueOnly()`: lightweight value refresh
5. `#include "ui/ui.h"` at line 15

## Line-Item Instructions

### Phase 1: Research widget accessibility

- [ ] Read `uidesign/crowpanel-128-rotary/lvglpro/screens/screen_diag_gen.h` and
      `screen_diag_gen.c` to determine how LVGL Pro exports named widgets.
      Check if `diag_arc`, `diag_category`, `diag_value`, `diag_counter`,
      `diag_status` are available as extern pointers.
      Document findings before proceeding.

### Phase 2: Update include and declarations

- [ ] Replace `#include "ui/ui.h"` with `#include "ui.h"` (or `"ui_gen.h"`)
      to match the UIDesign library's include path.

- [ ] Remove the 5 weak extern `lv_obj_t*` declarations (lines 31-35).

- [ ] If LVGL Pro exports the widget pointers as globals (likely with names
      matching the XML `name` attributes like `diag_arc`, `diag_category`,
      etc.), declare or include the appropriate header.

### Phase 3: Replace text setters with subjects

- [ ] In `updateWidgets()`:
      - Replace `lv_label_set_text(ui_ui_DiagCounter, counterBuf)` with
        `lv_subject_copy_string(&sys_diag_counter, counterBuf)`
      - Replace `lv_label_set_text(ui_ui_DiagCategory, item.category)` with
        `lv_subject_copy_string(&sys_diag_category, item.category)`
      - Replace `lv_label_set_text(ui_ui_DiagValue, valueBuf)` with
        `lv_subject_copy_string(&sys_diag_value, valueBuf)`
      - Replace `lv_label_set_text(ui_ui_DiagStatus, status)` with
        `lv_subject_copy_string(&sys_diag_status, status)`
      - Replace `lv_arc_set_value(ui_ui_DiagArc, _currentIndex)` with
        `lv_subject_set_int(&sys_diag_arc_value, _currentIndex)`

- [ ] In `refreshValueOnly()`:
      - Apply the same replacements for value and status text setters.

### Phase 4: Keep direct widget access for styling

- [ ] For the following styling operations, use the LVGL Pro widget pointers
      (however they're exported — globals or child lookup):
      - `lv_obj_set_style_bg_color(arc, evaluateColour(status), LV_PART_INDICATOR)`
      - `lv_obj_set_style_text_color(statusLabel, evaluateColour(status), LV_PART_MAIN)`
      - `lv_obj_set_style_text_font(valueLabel, font, LV_PART_MAIN)`

- [ ] If LVGL Pro uses the same widget names as subjects (e.g., both the
      label and subject are called `diag_category`), there may be name
      collisions. In that case, reference the widget via the LVGL Pro
      generated name (check the _gen.h header for the actual variable names).

### Phase 5: Update begin() and onArcValueChanged()

- [ ] In `begin()`:
      - Replace `ui_ui_DiagArc` references with the LVGL Pro arc widget pointer.
      - The arc configuration (range, callback, focus group) should use the
        widget pointer directly — subjects handle the VALUE but the callback
        registration needs the widget object.

- [ ] In `onArcValueChanged()`:
      - The callback uses `lv_event_get_target(e)` to get the arc, so it
        doesn't need the global pointer. This should work unchanged.
      - Replace `self->updateWidgets()` — already calls subjects internally.

### Phase 6: Update null guards

- [ ] Replace all `&ui_ui_DiagArc == nullptr` weak-symbol null guards with
      the appropriate guard for the new widget source. If using LVGL Pro
      globals, the guard may not be needed (they're always present when
      the UI is initialized). Use a simple null check on the pointer value.

### Phase 7: Build verification

- [ ] Verify build using `platformio` MCP `build_project` for `crowpanel-128-rotary`.
      This should be the FIRST time the full project compiles cleanly
      (WO1 infrastructure + WO2 module + WO3 diag carousel all done).

- [ ] Commit:
      ```
      WO3: Rebind DiagCarouselController to LVGL Pro subjects

      - Replace 5 weak SLS widget pointers with sys_diag_* subject setters
      - Use subjects for text/value data flow (category, value, counter, status, arc)
      - Keep direct widget access for styling (arc color, text color, font size)
      - Update include from ui/ui.h to ui_gen.h
      ```

## Verification Requirement

1. No weak extern `lv_obj_t*` SLS declarations remain
2. Text data flows through subjects (`lv_subject_copy_string`/`lv_subject_set_int`)
3. Styling (colors, fonts) still uses direct widget access
4. Arc encoder callback still works (widget registration + value-changed handler)
5. `crowpanel-128-rotary` compiles cleanly
6. All null guards updated appropriately

**CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

Surface any out-of-scope findings in the Completion Report.

---

## Completion Report (written by the worker at task completion)

### Status
complete

### Changes Made
- `modules/LVGLDisplay/DiagCarouselController.hpp`:
  - Added member pointers `_arc`, `_valueLabel`, `_statusLabel` to cache widgets resolved at `begin()` for dynamic styling without weak symbols.
- `modules/LVGLDisplay/DiagCarouselController.cpp`:
  - Replaced `#include "ui/ui.h"` with `#include "ui_gen.h"`.
  - Removed all 5 legacy weak SLS widget declarations (`ui_ui_DiagArc`, `ui_ui_DiagCategory`, `ui_ui_DiagValue`, `ui_ui_DiagCounter`, `ui_ui_DiagStatus`).
  - Added `findChildByName()` helper with indexed fallback to resolve widgets dynamically from `lv_screen_active()`.
  - Rebound text and value updates in `updateWidgets()` and `refreshValueOnly()` to LVGL Pro subjects (`sys_diag_arc_value`, `sys_diag_counter`, `sys_diag_category`, `sys_diag_value`, `sys_diag_status`).
  - Retained direct widget styling (`lv_obj_set_style_text_font`, `lv_obj_set_style_text_color`, `lv_obj_set_style_arc_color`).
  - Cleaned up weak-symbol null guards.

### Decisions Made
- Added a fallback by child index to `findChildByName` in case `LV_USE_OBJ_NAME` is not active in a downstream build configuration: the 5 child widgets of `screen_diag` are created in stable order (0: arc, 1: category, 2: counter, 3: value, 4: status).

### Dead Ends & Warnings
- None.

### Out-of-Scope Findings
- The `UIDesign` library (`uidesign/crowpanel-128-rotary/lvglpro/library.json`) lacks `"flags": ["-DLV_LVGL_H_INCLUDE_SIMPLE"]` (or `-DLV_LVGL_H_INCLUDE_SIMPLE` under `[env:crowpanel-128-rotary]` in `platformio.ini`), which causes its C headers to attempt `#include "lvgl/lvgl.h"` instead of `"lvgl.h"`. Surfaced for PA to address before/in WO4.

### Blockers
- None.

