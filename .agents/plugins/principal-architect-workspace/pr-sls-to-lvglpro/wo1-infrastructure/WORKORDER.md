---
name: sls-to-lvglpro-wo1
description: Handoff instructions for Work Order 1 — Infrastructure & Design-as-Library
complexity: standard
---

# Role Definition

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make high-level architectural
> decisions, do not create additional branches, and do not plan. Your task is
> strictly limited to executing the checklist below.

## Objective

Set up the infrastructure for the SLS → LVGL Pro migration: rename the design
folder to match the PIO environment name, rename the LVGL Pro project to "ui",
add the `sys_` prefix to all subjects, create the PIO library.json, update
platformio.ini, and remove old SLS-generated files from git tracking.

## Strict Boundary Box

- **DO NOT** modify `LVGLDisplayModule.cpp` or `.hpp` — that is WO2
- **DO NOT** modify `DiagCarouselController.cpp` or `.hpp` — that is WO3
- **DO NOT** delete `SLSSystemScreens.*` or `ILVGLSystemScreens.hpp` — that is WO4
- **DO NOT** modify any files in `src/`, `test/`, `lib/`, or `adapters/`
- **DO NOT** create any new branches — work on the current branch only
- **DO NOT** modify the principal-architect-workspace files (MEMORY.md, PROJECT.md)
- You MAY modify: `uidesign/`, `modules/LVGLDisplay/ui/` (removal only),
  `modules/LVGLDisplay/library.json`, `modules/LVGLDisplay/exclude_ui_on_native.py`,
  `platformio.ini`, `.gitignore`

## Tooling Constraints

- Use `platformio` MCP tools for any build verification (e.g., `build_project`)
  rather than raw `pio` CLI commands.
- Use `git` commands with `BypassSandbox: true` (sandbox git has fsmonitor issues).
- For file operations, prefer the file-editing tools over shell commands.

## Context: Current State

The LVGL Pro project is at `uidesign/crowpanel-128/lvglpro/` with:
- `project.xml` has `name="crowpanel_128"`
- `globals.xml` has 22 subjects WITHOUT `sys_` prefix (e.g., `boot_progress`)
- All `screens/*.xml` files reference unprefixed subjects in `bind_*` attributes
- Generated C files (`.c`/`.h`) use unprefixed names
- The current PIO env is `crowpanel-128-rotary`

The old SLS files are at `modules/LVGLDisplay/ui/` and tracked by git.

## Line-Item Instructions

### Phase 1: Folder Rename

- [ ] Rename `uidesign/crowpanel-128/` → `uidesign/crowpanel-128-rotary/`
      using `git mv uidesign/crowpanel-128 uidesign/crowpanel-128-rotary`
      Note: The LVGL Pro files inside are UNTRACKED. You'll need to handle
      the rename carefully — the `squareline/` subfolder IS tracked.
      If `git mv` fails on the parent, rename manually and re-add.

### Phase 2: Project Rename

- [ ] Edit `uidesign/crowpanel-128-rotary/lvglpro/project.xml`:
      Change `name="crowpanel_128"` → `name="ui"` (keep all other attributes)

### Phase 3: Subject Rename (sys_ prefix)

- [ ] Edit `uidesign/crowpanel-128-rotary/lvglpro/globals.xml`:
      Rename ALL subjects to add `sys_` prefix. The complete mapping:
      - `boot_progress` → `sys_boot_progress`
      - `boot_status` → `sys_boot_status`
      - `error_visible` → `sys_error_visible`
      - `setup_visible` → `sys_setup_visible`
      - `ota_visible` → `sys_ota_visible`
      - `busy_visible` → `sys_busy_visible`
      - `msg_visible` → `sys_msg_visible`
      - `error_title` → `sys_error_title`
      - `error_message` → `sys_error_message`
      - `setup_ssid` → `sys_setup_ssid`
      - `setup_url` → `sys_setup_url`
      - `ota_status` → `sys_ota_status`
      - `ota_progress` → `sys_ota_progress`
      - `ota_percent` → `sys_ota_percent`
      - `busy_message` → `sys_busy_message`
      - `msg_title` → `sys_msg_title`
      - `msg_body` → `sys_msg_body`
      - `diag_arc_value` → `sys_diag_arc_value`
      - `diag_category` → `sys_diag_category`
      - `diag_value` → `sys_diag_value`
      - `diag_counter` → `sys_diag_counter`
      - `diag_status` → `sys_diag_status`

- [ ] Add NEW subject to globals.xml:
      `<string name="sys_app_title" value="ESP32 App" />`

- [ ] Edit ALL screen XML files in `uidesign/crowpanel-128-rotary/lvglpro/screens/`:
      Update every `bind_text="..."`, `bind_value="..."`, and
      `<bind_flag_if_eq subject="..." .../>` to use the `sys_` prefixed names.
      Files to update: `screen_boot.xml`, `screen_system.xml`, `screen_diag.xml`

### Phase 4: PIO Library Configuration

- [ ] Create `uidesign/crowpanel-128-rotary/lvglpro/library.json`:
      ```json
      {
          "name": "UIDesign",
          "version": "1.0.0",
          "description": "LVGL Pro generated UI for this display environment",
          "dependencies": {
              "lvgl/lvgl": "^9.5.0"
          },
          "build": {
              "srcDir": ".",
              "srcFilter": [
                  "+<*.c>",
                  "+<screens/*.c>",
                  "+<fonts/*.c>",
                  "+<images/*.c>",
                  "-<preview-build/>",
                  "-<preview-bin/>"
              ]
          }
      }
      ```

- [ ] Edit `platformio.ini` `[env:crowpanel-128-rotary]` section:
      Change `lib_extra_dirs = modules, adapters`
      to `lib_extra_dirs = modules, adapters, uidesign/crowpanel-128-rotary`

### Phase 5: Remove Old SLS Files

- [ ] Remove all SLS-generated files from git tracking:
      `git rm -r modules/LVGLDisplay/ui/` (deletes tracked SLS files)

- [ ] Update `.gitignore`:
      Add entries for LVGL Pro editor build artifacts:
      ```
      # LVGL Pro Editor build artifacts
      uidesign/*/lvglpro/preview-build/
      uidesign/*/lvglpro/preview-bin/
      uidesign/*/lvglpro/.cache.json
      ```

### Phase 6: Build Verification

- [ ] Add the untracked LVGL Pro files to git:
      `git add uidesign/crowpanel-128-rotary/lvglpro/`

- [ ] Verify the project compiles using the `platformio` MCP `build_project`
      tool for the `crowpanel-128-rotary` environment.
      **NOTE:** The build will likely have compiler WARNINGS or ERRORS related to
      `LVGLDisplayModule.cpp` and `DiagCarouselController.cpp` because they
      still reference unprefixed subjects and SLS globals. This is EXPECTED and
      will be fixed in WO2/WO3. The goal here is to verify the PIO library
      discovery and compilation of the UI design files themselves works.
      If the errors prevent compilation entirely, add `lib_ignore = UIDesign`
      temporarily to the `[env:native]` section to verify other envs aren't broken.

- [ ] Commit all changes:
      `git add -A && git commit -m "WO1: Infrastructure for SLS→LVGL Pro migration"`

## Verification Requirement

1. The LVGL Pro design files at `uidesign/crowpanel-128-rotary/lvglpro/` are
   tracked by git with the correct structure
2. `project.xml` has `name="ui"`
3. All subjects in `globals.xml` have `sys_` prefix (23 total including new `sys_app_title`)
4. All screen XMLs reference `sys_*` subjects
5. `library.json` exists and is valid JSON
6. `platformio.ini` includes `uidesign/crowpanel-128-rotary` in `lib_extra_dirs`
7. Old SLS files in `modules/LVGLDisplay/ui/` are removed from git
8. `.gitignore` covers preview-build/preview-bin directories
9. Build attempt made (even if partial failures expected from WO2/WO3 scope)

**CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

If you discover a defect, upstream bug, or concern outside your boundary box,
surface it in your Completion Report below — do not file issues directly.

---

## Completion Report (written by the worker at task completion)

### Status
complete

### Changes Made
- `uidesign/crowpanel-128/` → `uidesign/crowpanel-128-rotary/`: Renamed to align with PlatformIO environment name.
- `uidesign/crowpanel-128-rotary/lvglpro/project.xml`: Updated project name to `name="ui"`.
- `uidesign/crowpanel-128-rotary/lvglpro/globals.xml`: Renamed all 22 subjects with `sys_` prefix and added 23rd subject `sys_app_title`.
- `uidesign/crowpanel-128-rotary/lvglpro/screens/*.xml`: Updated all `bind_*` subject references to use `sys_*`.
- `uidesign/crowpanel-128-rotary/lvglpro/ui.h`, `ui.c`, `ui_gen.h`, `ui_gen.c`, `screens/*_gen.c`, `file_list_gen.cmake`: Updated generated C files to match `ui` project name, `sys_*` subjects, and `ui_init` / `ui_init_gen`.
- `uidesign/crowpanel-128-rotary/lvglpro/library.json`: Created `UIDesign` PIO library manifest with source filters.
- `platformio.ini`: Added `uidesign/crowpanel-128-rotary` to `lib_extra_dirs` under `[env:crowpanel-128-rotary]`.
- `modules/LVGLDisplay/ui/`: Removed old SLS-generated files from git tracking.
- `modules/LVGLDisplay/exclude_ui_on_native.py`: Cleaned up to remove references to `ui/`.
- `.gitignore`: Added ignore rules for LVGL Pro editor build artifacts (`preview-build/`, `preview-bin/`, `.cache.json`).

### Decisions Made
- Updated the generated C files in `lvglpro/` (`ui.c`, `ui.h`, `ui_gen.c`, `ui_gen.h`, etc.) to match the renamed project `name="ui"` and `sys_*` subjects so that PlatformIO library compilation resolves cleanly without undefined symbols or obsolete names.

### Dead Ends & Warnings
- None.

### Out-of-Scope Findings
- None. `DiagCarouselController.cpp` missing header error during PIO build verification is expected and scoped for WO2/WO3.

### Blockers
None. Ready for WO2.
