---
name: adopt-display-interface-wo1
description: Forward-port the SLS-redesign stack onto main (adopt #86-89) — copy clean files, reconcile 4 collisions, reach green
complexity: standard
---

# WO1 — Forward-Port SLS Stack onto main (adopt the display interface)

### Role Definition
> You are an expert senior embedded/C++ engineer and execution-focused worker. You are NOT the
> Principal Architect. You do NOT redesign anything. Your job is a **forward-port**: bring the
> unmerged `sls-screen-redesign` work onto the current `main` (which gained #86–89) by copying the
> files `main` never touched and hand-reconciling the small overlap, until native tests are green and
> all three hardware environments compile. **No `git merge`, `git rebase`, or `git squash`** — this is
> a file-level forward-port on a branch already based on `main`.

## Objective
On branch **`adopt-display-interface/wo1-forward-port`** (already checked out, based on `origin/main`),
reproduce the SLS-redesign feature set (SquareLine screens, LVGL CONFIGURING #82, boot-screen rework,
single `FaultScreen`) on top of `main`'s unified `IDisplayModule` world (#86 capture/PngWriter, #88
`DisplayServiceHelper`, #89 portal sub-pills). End state: **native suite green + `crowpanel-128-rotary`
+ `esp32dev` + `esp32s3nano` all compile [SUCCESS]**.

## Provenance
Part of **PR: Adopt Display-Interface Framework** ([../PROJECT.md](../PROJECT.md)). The `PROJECT.md`
Session Log records the human's explicit approval ("proceed please", 2026-08-11) of this forward-port
decomposition. Reading `.agents/rules/` + this WORKORDER authorises you to execute autonomously within
the Strict Boundary Box below.

## Key references (read these first)
- **Source of the SLS work:** branch `sls-screen-redesign/wo7-hardware-test-debug` (tip `4c57f72`). Do NOT
  merge it — read files from it with `git show <ref>:<path>` / `git checkout <ref> -- <path>`.
- **Merge-base** (where the SLS stack diverged from main): `git merge-base sls-screen-redesign/wo7-hardware-test-debug origin/main` → should be `8ff3791` (#81).

## Step 1 — Compute the authoritative clean-vs-collision split
Do NOT trust a hardcoded list; regenerate it:
```
SLS=sls-screen-redesign/wo7-hardware-test-debug
BASE=$(git merge-base $SLS origin/main)
for f in $(git diff --name-only $BASE $SLS | grep -vE '^\.agents/|^tmp/'); do
  n=$(git log --oneline $BASE..origin/main -- "$f" | wc -l | tr -d ' ')
  [ "$n" = 0 ] && echo "CLEAN  $f" || echo "RECONCILE  $f"
done
```
`CLEAN` = `main` never touched it → copy the SLS version verbatim. `RECONCILE` = both sides changed it → hand-merge (spec below).

## Step 2 — Forward-port the CLEAN files
- For each `CLEAN` file that still exists on the SLS tip: `git checkout $SLS -- <path>`.
- For the three screens the SLS work **deleted** (they still exist on `main`): `git rm` them —
  `modules/LVGLDisplay/screens/{BootScreen,ErrorScreen,OTAProgressScreen}.{cpp,hpp}`.
  (These deletions only compile *after* Step 3 removes their references in `LVGLDisplayModule`.)
- Expect CLEAN to include: `adapters/lvgl/CrowPanel128Adapter.cpp`, `include/interfaces/ILVGLSystemScreens.hpp`,
  `include/interfaces/IBootReporter.hpp`, `modules/LVGLDisplay/lv_conf.h`, `library.json`, `exclude_ui_on_native.py`,
  `screens/FaultScreen.*`, `screens/SLSSystemScreens.*`, all `modules/LVGLDisplay/ui/*`, `platformio.ini`,
  `squareline/crowpanel-128/*`, `test/stubs/platform/lvgl.h`, `test/test_unit/test_lvglDisplay/{test_screens,test_sls_screens,ui_stubs}.cpp`, `.mcp.json`.

## Step 3 — Reconcile the 4 collision files (hand-merge)

### 3a. `modules/MCPServer/MCPBuiltinTools.cpp` — **take `main`, discard the SLS change**
#86 deleted the builtin `capture_display` entirely and changed `registerAll()`'s signature. The SLS
branch's `#ifndef WITH_DISPLAY_LVGL` guard is obsolete. Leave `main`'s file as-is (do nothing).

### 3b. `src/main.cpp` (and `sample-app/`, `template-app/` if they differ) — **take `main`'s wiring**
Keep `main`'s `IDisplayModule*` registration/#89 wiring. Ensure the LVGL module is still constructed with
the `SLSSystemScreens` provider (as the SLS branch did) and that boot-reporter registration order is
preserved. Diff `git show $SLS:src/main.cpp` vs `main` and port only the SLS-specific construction bits
(display adapter + SLSSystemScreens) onto main's structure.

### 3c. `modules/LVGLDisplay/LVGLDisplayModule.hpp` + `.cpp` — **start from `main`, re-apply the SLS boot rework**
`main` has what we KEEP: `IDisplayModule` inheritance, `capturePNG`, `showSystemState`, `DisplayServiceHelper`,
geometry/brightness/screensaver accessors, #89 sub-pills, `_onAppConfiguring`. The SLS tip has the boot
rework to RE-APPLY. See both with:
```
git diff $BASE $SLS -- modules/LVGLDisplay/LVGLDisplayModule.hpp modules/LVGLDisplay/LVGLDisplayModule.cpp
git show $SLS:modules/LVGLDisplay/LVGLDisplayModule.cpp   # target for the boot methods
```
Apply these SLS deltas onto `main`'s file (method-by-method; keep everything else main has):
- **`.hpp`:** remove `#include "screens/{BootScreen,ErrorScreen,OTAProgressScreen}.hpp"`; add `#include "screens/FaultScreen.hpp"` and `#include "assets/boot_logo.h"`. Remove members `_builtInBootScreen/_builtInErrorScreen/_builtInOtaScreen` and accessors `getBootScreen()/getErrorScreen()/getOtaScreen()`. Add `FaultScreen _faultScreen;`, `bool _uiInitialized = false;`, `float _lastBootProgress = 0.0f;`, and a `void showFault(const char* title, const char* detail);` decl. Update the two "falls back to built-in Error/OTA" doc comments to "FaultScreen".
- **`.cpp`:** drop the `_builtInBootScreen(...)` ctor initialiser. In `bootstrapHardware()` call `ui_init()` (guarded by `!_uiInitialized`, weak symbol) **before** `showBoot()`. Guard the `ui_init()` in `begin()` with `_uiInitialized` too. Replace the bodies of `reportStatus()`, `showBoot()`, `showError()`, `showOtaProgress()`, `setBootLogo()` with the SLS-tip versions (SLS boot screen via `_screens->bootScreen()`/`onBootProgress()` + synchronous `lv_refr_now` interpolation; `showError`/`showOtaProgress` fall back to `showFault(...)`; add the `showFault()` definition).
- **Do NOT touch** `capturePNG`, `showSystemState`, `_onAppConfiguring`, `DisplayServiceHelper` usage, geometry/brightness/screensaver, or the #89 web-route code — keep main's.
- **Note (for WO2, do not fix here):** main's `showSystemState(Configuring)` calls `reportStatus()`, which now drives the SLS *boot* screen. Leave it; WO2 repoints it.

### 3d. Doc + submodule merges
- `modules/LVGLDisplay/README.md`, `docs/developer/mcp-server.md`: keep `main`'s content and fold in the SLS
  additions (FaultScreen / boot model) so nothing is lost. If trivial, just take main's + append the SLS notes.
- `.gitmodules`, `components/esp_littlefs`: take **main's** submodule state (do not regress #88's `framework_design_system`).

## Strict Boundary Box
- Only the files identified by Step 1 (CLEAN copies) + the collision files in Step 3. Do NOT modify
  `U8G2Display`, `lib/PngWriter`, `lib/DisplayServiceHelper`, framework core, or anything #86–89 added.
- Do NOT redesign. Do NOT "improve" main's capture/DisplayServiceHelper/showSystemState code.
- Do NOT `git merge`/`rebase`/`squash`. Commit as normal linear commits on this branch.
- Cosmetic nits get logged in the Completion Report, not fixed.

## Tooling Constraints
- Use the `platformio` MCP (skill `pio-manager`) for ALL builds/tests — `run_tests` (env `native`),
  `build_project` (envs `crowpanel-128-rotary`, `esp32dev`, `esp32s3nano`) with `background: true`.
  Do NOT flash hardware in this WO (compile only).
- Use `git show`/`git checkout <ref> -- <path>`/`git rm` for the forward-port; no raw `pio` CLI.

## Line-Item Instructions
- [ ] Step 1: regenerate the CLEAN/RECONCILE split; paste it into the Completion Report.
- [ ] Step 2: `git checkout $SLS -- <clean>` + `git rm` the 3 deleted screens.
- [ ] Step 3: reconcile the 4 collision files + doc/submodule merges per spec.
- [ ] Build `native` tests → green (report the count; main was 419 + SLS lvgl tests).
- [ ] Build `crowpanel-128-rotary`, `esp32dev`, `esp32s3nano` → all [SUCCESS].
- [ ] Commit in logical chunks (e.g. "forward-port clean SLS files", "reconcile LVGLDisplayModule onto IDisplayModule", "drop obsolete capture_display guard"). Linear commits only.

## Verification Requirement
Exit = native suite green **and** the three hardware envs compile [SUCCESS]. Report exact test count and
per-env result. If a reconciliation is ambiguous or a build fails in a way that implies a design decision
(e.g. `showSystemState` vs configuring, or a DisplayServiceHelper contract mismatch with our SLS screens),
**stop and surface it** in the Completion Report rather than guessing.

### Issue Surfacing
Any framework-level or out-of-boundary defect you find → **surface in the Completion Report** for the PA
to triage/file. Do NOT open GitHub issues yourself.

---
## Completion Report
<!-- Written at task completion -->
### Status
(pending)   <!-- complete | partial | blocked -->

### CLEAN/RECONCILE split (Step 1 output)
-

### Files forward-ported / deleted / reconciled
-

### Reconciliation notes (esp. LVGLDisplayModule onto IDisplayModule)
-

### Verification
- native: ____/____   | crowpanel-128-rotary: ____ | esp32dev: ____ | esp32s3nano: ____

### Surfaced findings (PA to triage)
-

### Blockers
(none)
