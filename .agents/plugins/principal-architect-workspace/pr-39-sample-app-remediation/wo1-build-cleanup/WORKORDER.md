---
name: pr39-wo1
description: Handoff instructions for Work Order 1 — Build config cleanup & parity
complexity: mechanical
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Clean up stale build artefacts and bring the sample-app's `platformio.ini` and `CMakeLists.txt` into parity with the main project. F03 (dead MyApp web assets) is already resolved — the files were moved to `template-app/` by the user.

# Strict Boundary Box

- **ONLY** touch files inside `sample-app/`.
- Do **NOT** modify any framework kernel code (`modules/`, `lib/`, `include/`, `src/`).
- Do **NOT** modify any files in `template-app/`.
- Do **NOT** create new branches or merge anything.

# Tooling Constraints

- Use the `platformio` MCP server tools for builds (e.g., `build_project`) — not raw `pio` CLI commands.
- The sample-app's PlatformIO project root is `/Users/mcneillm/Documents/Projects/esp32-app-foundation/sample-app`.

# Checklist

- [ ] **F24: Rename CMake project** — In `sample-app/CMakeLists.txt`, change `project(template)` to `project(weather-station)`.

- [ ] **F22: Add portalBuilder.py** — In `sample-app/platformio.ini` `[env:base]` section, add `pre:../scripts/portalBuilder.py` to `extra_scripts` (after `inject_version.py`).

- [ ] **F23: Native env defines** — In `sample-app/platformio.ini` `[env:native]` section, add these build flags:
  ```
  -DBOARD_ID=\"native\"
  -DAPP_TITLE=\"Weather\ Station\"
  -DSECURITY_VERSION=1
  ```
  Also add `test_filter = test_unit/*` to `[env:native]`.

- [ ] **Verify: Build compiles** — Run `pio run -e native` in the sample-app directory using MCP tools to verify the build succeeds. If it fails due to portalBuilder dependencies, note the error but don't remove the script — just add a comment.

# Verification

After making all changes, verify with a native build. Report the results.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.
