---
name: portal-decentralise-wo1
description: Update portalBuilder.py to scan module web dirs + web/display, then move 9 JS/HTML files to their new owners
complexity: standard
---

# WO1 — Build Pipeline & Source Relocation

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent.

## Objective
Extend `portalBuilder.py` to scan `web/display/` and `modules/*/web/` for web assets (in addition to the existing `web/portal/`), then move 9 JS/HTML files from `web/portal/` to their proper owner directories.

## Strict Boundary Box
- **ONLY** modify `scripts/portalBuilder.py`.
- **ONLY** move files listed in the checklist (use `git mv`).
- **DO NOT** modify any `.cpp`, `.hpp`, or `.h` files.
- **DO NOT** modify `platformio.ini`.
- **DO NOT** modify `web/portal/app.js`.
- **DO NOT** branch.

## Tooling Constraints
- Use the `platformio` MCP server for build verification (`build_project`).
- Do NOT use raw `pio` CLI commands.

## Line-Item Instructions

### Update portalBuilder.py
- [x] Refactor `build_web_portal()` to scan multiple source directories. Replace the single `web_dir` with a list of scan directories:
  1. `web/portal/` (existing)
  2. `web/display/` (new — shared display contract assets)
  3. `modules/*/web/` (new — per-module web assets, discovered dynamically)
- [x] All directories feed the same output pipeline → `include/portal_assets/`. Same flat namespace, same include guards, same gzip+PROGMEM format.
- [x] Add a collision check: if two source files have the same basename, print a warning and fail.
- [x] Preserve the existing header format and license comment exactly.

### Move files (use `git mv`)
- [x] `mkdir -p web/display`
- [x] `git mv web/portal/display.js web/display/display.js`
- [x] `git mv web/portal/screenshot.js web/display/screenshot.js`
- [x] `git mv web/portal/screenshot.html web/display/screenshot.html`
- [x] `mkdir -p modules/OTAUpdateManager/web`
- [x] `git mv web/portal/ota.js modules/OTAUpdateManager/web/ota.js`
- [x] `mkdir -p modules/MCPServer/web`
- [x] `git mv web/portal/mcp.js modules/MCPServer/web/mcp.js`
- [x] `mkdir -p modules/TimeManager/web`
- [x] `git mv web/portal/time.js modules/TimeManager/web/time.js`
- [x] `mkdir -p modules/HomeAssistant/web`
- [x] `git mv web/portal/homeassistant.js modules/HomeAssistant/web/homeassistant.js`
- [x] `mkdir -p modules/U8G2Display/web`
- [x] `git mv web/portal/carousel.js modules/U8G2Display/web/carousel.js`
- [x] `mkdir -p modules/LVGLDisplay/web`
- [x] `git mv web/portal/hardware.js modules/LVGLDisplay/web/hardware.js`

### Verify generated headers are byte-identical
- [x] Delete `include/portal_assets/` directory.
- [x] Run a hardware build (e.g. `esp32s3nano`) via the platformio MCP server — this triggers `portalBuilder.py` as a pre-script.
- [x] Verify the build succeeds (meaning the builder found all source files and regenerated all headers).
- [x] **Before committing, ask the user if they are happy with the changes.**

## Issue Surfacing
If you discover a defect or concern outside your boundary box, surface it in your Completion Report.

---

## Completion Report

### Status
Completed

### Changes Made
- Updated `scripts/portalBuilder.py` to scan `web/portal/`, `web/display/`, and dynamically discovered `modules/*/web/`.
- Added basename collision check to halt build (`sys.exit(1)`) if duplicate asset filenames are detected.
- Relocated 9 web asset files from `web/portal/` to `web/display/` and their respective `modules/*/web/` directories using `git mv`.
- Verified regeneration of all 20 `.hpp` files in `include/portal_assets/` (byte-identical verification).
- Committed changes with message `refactor(portal): relocate optional module web assets to owner directories (#96)`.

### Dead Ends & Warnings
- None. Initial build verification timed out on initial cold build, resolved by background execution (`background: true`).

### Out-of-Scope Findings
- None. Web application tests run during full PlatformIO test suites will need WO2 route updates to serve moved assets from module endpoints.

