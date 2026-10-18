---
name: pr-35-wo0
description: Handoff instructions for Work Order 0 — Portal Prep & Template Rename
complexity: standard
---

# Work Order 0: Portal Prep & Template Rename

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Rename the `template/` directory to `sample-app/`, relocate the Credits bottom-nav tab to an ℹ️ info icon modal in the portal header, enable `esp-mqtt` in the sdkconfig, and gate the internal `AppTabContributor` stub so it does not conflict with application-provided tabs.

## Strict Boundary Box

- You may ONLY modify files within: `template/`, `sample-app/`, `web/portal/`, `modules/WebServer/`, `modules/Application/Application.hpp`, `modules/Application/Application.cpp`, `sdkconfig.defaults`, `README.md`, `.github/workflows/`, `platformio.ini`.
- Do NOT touch any other module code (e.g., `modules/HomeAssistant/`, `modules/U8G2Display/`).
- Do NOT create any new framework modules.

## Tooling Constraints

- Use the PlatformIO MCP server tools (`build_project`, `check_project`) to verify compilation after changes. Do not use raw `pio` CLI commands.
- Read the framework-expert skill at `.agents/skills/framework-expert/SKILL.md` for house style and coding conventions.

## Line-Item Instructions

- [ ] **1. Rename `template/` → `sample-app/`**
  - `git mv template/ sample-app/`
  - Update all references in `README.md`, `sample-app/README.md`, `sample-app/platformio.ini`, and any `.github/workflows/*.yml` files.

- [ ] **2. Relocate Credits tab to header ℹ️ icon modal**
  - In `web/portal/app.js` (or the relevant portal shell): Remove the "Credits" entry from the bottom-nav tab registration.
  - Add an ℹ️ info icon button to the top-right of the portal header bar.
  - Clicking the icon opens a modal overlay. The modal body renders the existing content from `credits.js` (the open-source library list card).
  - Ensure the modal has a close button and closes on backdrop click.
  - The `credits.js` file itself should remain largely unchanged — just its rendering target changes from `tab-credits` to the modal container.

- [ ] **3. Enable `esp-mqtt` in sdkconfig**
  - Add `CONFIG_MQTT_PROTOCOL_311=y` to `sdkconfig.defaults` (the base layer).
  - Review and set reasonable defaults for MQTT buffer sizes and task stack (`CONFIG_MQTT_BUFFER_SIZE`, `CONFIG_MQTT_TASK_STACK_SIZE`).
  - Add an inline comment documenting the approximate RAM cost.

- [ ] **4. Gate the `AppTabContributor` in `Application.hpp`**
  - The internal `AppTabContributor` struct (lines ~518-523) contributes a hardcoded "App" top-level tab. Applications that register their own `IWebContributor` with a top-level tab should not see a duplicate "App" tab.
  - Add a `bool _enableDefaultAppTab = true;` flag and a `void disableDefaultAppTab()` method. When the flag is false, do not register `_appTabContributor` as a web contributor during `begin()`.
  - Alternatively, check during `begin()` whether any user-registered `IWebContributor` already provides a top-level tab — if so, suppress the default "App" tab automatically.

## Verification Requirement

1. Run `build_project` via the PlatformIO MCP server for the `esp32s3nano` environment to verify the framework compiles cleanly.
2. Run `pio test -e native` (or the MCP equivalent) to verify no test regressions.
3. **CRITICAL:** Before distilling or committing, STOP and send a message back to the Principal Architect confirming results. Only after approval should you invoke the `@distillery` skill.

## Status: COMPLETED
All tasks completed. Changes committed to branch. Native build test regression (`millis()` in `OTAUpdateManager`) has been noted and logged in the distillery `graveyard` artifact but was out of scope for this Work Order.
