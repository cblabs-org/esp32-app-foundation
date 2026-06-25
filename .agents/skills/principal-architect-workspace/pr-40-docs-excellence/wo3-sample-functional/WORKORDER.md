---
name: docs-excellence-wo3
description: Handoff instructions for Work Order 3 — Sample-app functional fixes and polish
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Fix functional issues and polish the sample-app: IConfigGate registration, WeatherWidget units bug, streaming timeout, README corrections, and platformio.ini documentation.

# Strict Boundary Box

- **ONLY** modify files under `sample-app/`
- Do NOT touch any framework files (`modules/`, `lib/`, `include/`, `src/`)
- Do NOT touch `template-app/`
- Do NOT create git branches or make commits

# Tooling Constraints

- Use `platformio` MCP tools for builds: `build_project`
- Build the sample-app to verify changes compile

# Line-Item Instructions

## 1. Verify IConfigGate auto-detection

- [ ] Read `modules/Application/Application.cpp` — specifically the `registerModule()` method — to determine whether `IConfigGate` is auto-detected via compile-time introspection
- [ ] If `IConfigGate` IS auto-detected: add a comment in `sample-app/src/main.cpp` at the `registerModule()` call confirming this: `// registerModule() auto-detects IConfigGate — no separate registration needed`
- [ ] If `IConfigGate` is NOT auto-detected: add explicit registration `app.registerConfigGate(&weatherModule);` in main.cpp before `app.begin()`, with an explanatory comment

## 2. Fix WeatherWidget hardcoded units

- [ ] In `sample-app/src/WeatherWidget.hpp`: update `draw()` signature to accept a `const char* units` parameter (or obtain it from WeatherModule)
- [ ] In `sample-app/src/WeatherWidget.cpp`: replace hardcoded `°C` with the passed units string
- [ ] In `sample-app/src/WeatherClockBoard.cpp`: update the `draw()` call site to pass the correct units from WeatherModule

## 3. Add streaming timeout to fetchUpdate()

- [ ] In `sample-app/src/WeatherModule.cpp`, in the `fetchUpdate()` parser loop: add an absolute timeout guard (~30 seconds). Use `millis()` to track elapsed time. If exceeded, break out of the loop, log an error, and return `UpdateStatus::FetchError`
- [ ] Add a comment explaining why the timeout exists: "Prevents indefinite blocking if the server stalls mid-stream"

## 4. Thread safety documentation

- [ ] In `sample-app/src/WeatherModule.hpp`: add an inline comment at `getUnits()` documenting the accepted race window: "Returns by value. Config writes (applyConfig) and data reads (fetchUpdate) may race on _units, but SSO guarantees atomicity for short strings like 'metric'/'imperial'."

## 5. API key log masking

- [ ] In `sample-app/src/WeatherModule.cpp` in `fetchUpdate()`: where the URL is logged via `ESP_LOGI`, mask the API key portion. Either log only lat/lon without the full URL, or replace the key with `***` in the log string.

## 6. README fixes

- [ ] In `sample-app/README.md`: update the interface count from 6 to 7 (add IDisplayBoard to the interface table)
- [ ] Add IDisplayBoard row to the interface table: `| IDisplayBoard | WeatherClockBoard | Renders weather data on the OLED carousel |`

## 7. platformio.ini documentation

- [ ] In `sample-app/platformio.ini`: add inline comments for non-obvious build flags:
  - `SECURITY_VERSION=1` — "Anti-rollback security version counter"
  - `CONFIG_ASYNC_TCP_RUNNING_CORE=1` — "Pin async TCP to core 1 to avoid WiFi contention"
  - `CONFIG_ASYNC_TCP_STACK_SIZE=4096` — "Stack size for async TCP task"
  - Any other undocumented flags

## 8. Carousel registration comment

- [ ] In `sample-app/src/main.cpp`: at the carousel `add()` call, add a comment: "IDisplayBoard carousel registration is manual — registerModule() does not auto-detect display boards because the display module is optional and may not be present."

## 9. Rename WeatherClockBoard → WeatherBoard

The class name is misleading — it doesn't render a clock. Rename throughout.

- [ ] Rename `sample-app/src/WeatherClockBoard.hpp` → `sample-app/src/WeatherBoard.hpp`
- [ ] Rename `sample-app/src/WeatherClockBoard.cpp` → `sample-app/src/WeatherBoard.cpp`
- [ ] Rename the class `WeatherClockBoard` → `WeatherBoard` in both files
- [ ] Update `#include "WeatherClockBoard.hpp"` → `#include "WeatherBoard.hpp"` in `main.cpp`
- [ ] Update the variable name `weatherClockBoard` → `weatherBoard` in `main.cpp`
- [ ] Update the `getBoardName()` return value from `"WeatherClock"` → `"Weather"` (or similar)
- [ ] Update any references in `sample-app/README.md`
- [ ] Update the header block module path in both files to reflect new filenames

# Verification

- [ ] Build the sample-app for esp32dev: must succeed
- [ ] Build the sample-app native tests if they exist: must pass
- [ ] Verify the WeatherWidget now renders the correct unit string
- [ ] Verify `grep -rn "WeatherClockBoard" sample-app/` returns zero results
- [ ] **STOP and ask the user if they are happy before distilling**

