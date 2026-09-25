---
name: sample-app-remediation-wo4
description: Handoff instructions for Work Order 4 — Boot sequence cleanup + identity consolidation
complexity: standard
---

# WO-4: Boot Sequence + main.cpp Cleanup + Identity Consolidation

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Fix the display module initialization order, clean up stale includes and OTA TODOs, and consolidate the three disconnected device identity wiring calls per issue #232.

## Parent Project
`sample-app-remediation` — see [PROJECT.md](../PROJECT.md) for human approval provenance.

## Context

**W1 — Display init order:** `displayModule.begin(&app.getEventBus())` is called at L201 BEFORE `app.begin(config)` at L213. The `using-optional-modules.md` guide specifies calling `begin(eventBus)` AFTER `app.begin()`.

**W3 — Stale include:** `<cstring>` is included at L19 but no `memcpy`/`strlen`/`strcpy` are used directly.

**W4 — OTA TODO:** Lines 191-192 have commented-out OTA config with a TODO.

**W7 / #232 — Device identity fragmentation:** Three separate identity calls:
```cpp
app.setAppName("Weather Station");                          // L101
app.getWifiManager().setDefaultHostname("WeatherStation");  // L197
haModule.setAppName(app.getAppName());                      // L132
```
These should be consolidated — derive the hostname from the app name.

## Strict Boundary Box
**IN SCOPE:** `sample-app/src/main.cpp` ONLY.

**OUT OF SCOPE — DO NOT TOUCH:**
- `WeatherModule.*`, `WeatherBoard.*`, `WeatherWidget.*`, `IWeatherDataProvider.hpp`
- `web/weather.js`
- Any BLE files or BLE-guarded provider code (the `#ifdef WITH_BLE` blocks can have comments added but the actual BLE wiring code must NOT be changed)
- Any framework modules
- `platformio.ini`

## Tooling Constraints
- Use the `platformio-mcp_platformio` MCP server's `build_project` tool for compilation verification.
- Read the `pio-manager` skill at `/Users/mcneillm/.gemini/config/plugins/platformio-mcp/skills/pio-manager/SKILL.md` before your first build.
- Read `docs/developer/using-optional-modules.md` for the canonical display module init pattern.

## Line-Item Instructions

### Phase 1: Fix Display Init Order (W1)

- [x] **`main.cpp`** — Move `displayModule.begin(&app.getEventBus())` call from its current position (BEFORE `app.begin(config)`) to AFTER the `app.begin(config)` call. Read `docs/developer/using-optional-modules.md` for the canonical two-phase init pattern. If `bootstrapHardware()` is mentioned, verify whether it's needed and add it if so.
- [x] **`main.cpp`** — Move `configuringStatus.setEngine(...)` call to also be after `app.begin(config)` if it depends on the PreconditionEngine being initialized.

### Phase 2: Remove Stale Include (W3)

- [x] **`main.cpp`** — Remove `#include <cstring>` if no `memcpy`, `strlen`, `strcpy`, `strncpy`, `strlcpy`, `memset`, `memmove`, or other `<cstring>` functions are used directly in main.cpp. Verify by searching before removing.

### Phase 3: OTA Config (W4)

- [x] **`main.cpp`** — Replace the commented-out OTA lines (around L191-192) with a well-documented example block:
  ```cpp
  // --- OTA Configuration ---
  // Uncomment and configure for over-the-air firmware updates.
  // See docs/developer/versioning-policy.md for the tag-before-flash policy.
  // otaManager.setUpdateUrl("https://your-ota-server.com/firmware");
  // otaManager.setCheckInterval(3600000);  // Check every hour
  ```

### Phase 4: Identity Consolidation (#232)

- [x] **`main.cpp`** — Add a helper lambda or inline code near the top of `setup()` (after `app.setAppName(...)`) that derives the WiFi hostname from the app name. Convert to lowercase and replace spaces/special chars with hyphens:
  ```cpp
  // #232: Derive WiFi hostname from app name until framework auto-propagates.
  // "Weather Station" → "weather-station"
  auto sanitiseHostname = [](const char* name) -> String {
      String hostname;
      hostname.reserve(32);
      for (const char* p = name; *p && hostname.length() < 31; ++p) {
          char c = *p;
          if (c >= 'A' && c <= 'Z') c += 32;  // tolower
          if (c == ' ' || c == '_') c = '-';
          if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') {
              hostname += c;
          }
      }
      return hostname;
  };
  ```
- [x] **`main.cpp`** — Replace the hardcoded `setDefaultHostname("WeatherStation")` call with `setDefaultHostname(sanitiseHostname(app.getAppName()).c_str())`
- [x] **`main.cpp`** — Add an inline comment at the `haModule.setAppName(app.getAppName())` line: `// #232: Manual wiring until framework auto-propagates`
- [x] **`main.cpp`** — In the `#ifdef WITH_BLE` block, add a comment documenting the BLE gap: `// #232: BLE advertised name not yet wirable from app name — see issue #232`
- [x] **`main.cpp`** — Verify the `BootBranding` title is set to `"Weather Station"` (it should already be correct)

### Phase 5: Header Cleanup

- [x] **`main.cpp`** — Update the file header comment block to reflect the changes made (identity consolidation, display init order fix)

## Verification Requirement

1. **Build verification**: Use the `build_project` MCP tool to compile the `esp32dev` environment. Must succeed with zero errors.
2. **Code review**: Verify `displayModule.begin()` is now called after `app.begin(config)`.
3. **Code review**: Verify no `#include <cstring>` remains (if it was unused).
4. **CRITICAL**: Before committing, ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete

### Changes Made
- `sample-app/src/main.cpp`:
  - **Display Init Order (W1)**: Moved `displayModule.begin(&app.getEventBus())`, `displayModule.setConfigManager(...)`, `diagnosticsBoard.configure(...)`, `displayModule.boards.configuringStatus.setEngine(...)`, `displayModule.carousel.add(...)`, and `weatherBoard.wireEventBus(...)` from before `app.begin(config)` to after `app.begin(config)`.
  - **Stale Include Removal (W3)**: Removed `#include <cstring>` after verifying no C-string memory/string manipulation functions were utilized.
  - **OTA Configuration (W4)**: Replaced commented-out legacy OTA configuration and TODO comments with the documented example block.
  - **Identity Consolidation (#232)**: Added `sanitiseHostname` lambda in `setup()` immediately following `app.setAppName("Weather Station")` to derive `"weather-station"`. Replaced hardcoded `setDefaultHostname("WeatherStation")` with `setDefaultHostname(sanitiseHostname(app.getAppName()).c_str())`. Annotated `haModule.setAppName(...)` and `#ifdef WITH_BLE` block with `#232` tracking comments. Verified `BootBranding` title is `"Weather Station"`.
  - **Header Cleanup**: Updated file header comment block to document identity consolidation and display two-phase lifecycle.

### Decisions Made
- `bootstrapHardware()` is called automatically by `Application::begin()` through `_bootReporter->beginEarly()` when `displayModule` is registered prior to `app.begin()`, conforming to the framework's two-phase display initialization architecture without requiring an explicit call in `setup()`.
- Grouped `sanitiseHostname` and `app.getWifiManager().setDefaultHostname(...)` directly after `app.setAppName("Weather Station")` at the beginning of `setup()` to consolidate identity initialization in a single location.

### Dead Ends & Warnings
- None encountered.

### Out-of-Scope Findings
- Issue #232 tracking: Automatic propagation of device application name to peripheral identifiers (such as BLE advertising local name and WiFi default hostname) remains a desirable future framework-level feature.

### Blockers
- None. User approval explicitly granted on 2026-09-25.
