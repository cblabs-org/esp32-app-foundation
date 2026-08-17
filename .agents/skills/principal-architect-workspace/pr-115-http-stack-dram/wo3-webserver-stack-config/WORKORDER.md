---
name: pr-115-http-stack-dram-wo3
description: Handoff instructions for Work Order 3 — WebServerManager stack/socket config
complexity: standard
---

# WORKORDER — WO3: WebServerManager stack/socket configuration

**Parent project:** `pr-115-http-stack-dram` ([PROJECT.md](../PROJECT.md)) · **Parent issue:** #115
**Base branch:** `pr-115-http-stack-dram/wo2-screenshot-offload` · **This branch:** `pr-115-http-stack-dram/wo3-webserver-stack-config`
**Provenance:** decomposition human-approved per PROJECT.md status log. Autonomous execution authorized within the boundary box.
**Depends on:** WO2 — screenshot compression is already offloaded, so lowering the shared httpd stack is now safe.

### Role Definition
> You are an expert senior embedded C++ engineer and execution-focused worker agent. You are NOT the Principal Architect. Execute strictly the checklist.

### Objective
Remove the unconditional 16 KB httpd stack and make the HTTP task stack / socket count board-configurable, defaulting to a portal-safe 8 KB — returning internal DRAM to lwIP. Provide a clean setter so consuming apps stop reaching into `getServer().config`.

### Design specification (decisions already made)
- Delete the two unconditional `config.stack_size = 16384` lines: `WebServerManager.cpp:89` (HTTP) and `:126` (HTTPS).
- Board-tunable defaults, applied to both HTTP and HTTPS servers:
  ```cpp
  #ifndef BOARD_HTTP_STACK_SIZE
  #define BOARD_HTTP_STACK_SIZE 8192          // safe for all non-screenshot routes (compression is offloaded, WO2)
  #endif
  #ifndef BOARD_HTTP_MAX_OPEN_SOCKETS
  #define BOARD_HTTP_MAX_OPEN_SOCKETS 7       // = HTTPD_DEFAULT_CONFIG default (ENABLE_ASYNC is off)
  #endif
  ```
- **Flash-safety invariant (do not violate):** the httpd task stack MUST remain **internal DRAM**. Do **NOT** set `config.task_caps` to anything PSRAM, and do **NOT** move the httpd task stack to PSRAM. The httpd task runs `Update.write` (direct OTA upload) and LittleFS config-save, both of which write flash with the SPI cache disabled — a PSRAM stack there would fault. (This is the rejected "Option D".) Leave `task_caps` at its default.
- Public setter (Option C): `void WebServerManager::setHttpConfig(size_t stackSize, int maxOpenSockets);` — must be called **before** `begin()` (document this). Applies to whichever server(s) `begin()` starts.
- Keep the `#ifndef UNIT_TEST` guards (native stub has no `.config`).

### Strict Boundary Box — do NOT touch
- Do **not** modify the screenshot route, `DeepStackExecutor`, `PngWriter`, or display modules (WO1/WO2).
- Do **not** set any board's `BOARD_HTTP_STACK_SIZE` override in this WO unless a board is already known to need it — the fallback is the default. (If `crowpanel-128-rotary` needs an explicit value, note it; do not guess.)
- Do **not** flash, tag, or open a PR.

### Tooling Constraints
- **`platformio` MCP** (`build_project`, `run_tests`).
- Verify `native` + `crowpanel-128-rotary` (compile), and the existing `test/test_unit/test_webServer` suite.

### Line-Item Instructions
- [ ] `modules/WebServer/WebServerManager.cpp` — remove both `stack_size = 16384` lines; apply `BOARD_HTTP_STACK_SIZE` + `BOARD_HTTP_MAX_OPEN_SOCKETS` to HTTP and HTTPS `config`.
- [ ] `modules/WebServer/WebServerManager.hpp` — declare `setHttpConfig(size_t, int)`; add house-style docs incl. the "call before begin()" + "httpd stack stays internal DRAM (OTA flash-safety)" notes; store overrides applied in `begin()`.
- [ ] `hardware/_common` or the board-header fallback location — add the `#ifndef` fallbacks in the appropriate shared header if that is the project convention (check where existing `BOARD_*` fallbacks live before adding; do not scatter).
- [ ] Extend `test/test_unit/test_webServer` to assert `setHttpConfig` sets the intended values (guarded for the stub).

### Verification Requirement
`run_tests` (native) green; `build_project crowpanel-128-rotary` compiles. Portal asset delivery + DRAM headroom are PA/human hardware-verified after the stack lands (worker must not flash). **Before committing, ask the user if they are happy with the changes.**

### Issue Surfacing
Record out-of-scope findings in the Completion Report; do not fix or file.

---
## Completion Report
<!-- Written by the worker at task completion -->

### Status
(pending)

### Changes Made
-

### Decisions Made
-

### Dead Ends & Warnings
-

### Out-of-Scope Findings
-

### Blockers
(none)
