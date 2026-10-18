---
name: pr41-wo7
description: Handoff instructions for Work Order 7 — Migrate ConfigManager + ConfigManifest
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Remove all `#ifdef NATIVE_BUILD` / `#ifndef NATIVE_BUILD` guards from ConfigManager (~7 guards) and ConfigManifest library (~4 guards). Both modules use LittleFS for file I/O with `std::fstream` alternatives in native builds. The LittleFS stub header (created in WO1) is backed by `std::fstream` on `test/tmp/`, so the alternative implementations become redundant.

## Strict Boundary Box

- **DO** modify `modules/ConfigManager/ConfigManager.hpp`, `modules/ConfigManager/ConfigManager.cpp`
- **DO** modify `lib/ConfigManifest/ConfigManifest.cpp`
- **DO** extend stub headers in `test/stubs/platform/` if needed
- **DO NOT** modify any other module's source files

## Tooling Constraints

- Use `platformio` MCP tools for building and testing

## Line-Item Instructions

### ConfigManager

- [ ] **`ConfigManager.hpp`** — Remove guard around Arduino/LittleFS/WiFi/time includes and the `#else` block with `<string>`, `<stdint.h>`, `<ctime>`. Include the hardware headers unconditionally (stubs resolve in native).
- [ ] **`ConfigManager.cpp`** — Remove all guards:
  - Include guards for `<PsychicHttp.h>`, `<fstream>`, `<sstream>`, `<cstdio>`
  - `getConfigPath()`: remove path routing (`/mod/<ns>.json` vs `test/tmp/mod_<ns>.json`). **IMPORTANT**: The LittleFS stub maps paths to `test/tmp/` internally, so use the hardware path (`/mod/<ns>.json`) unconditionally and let the stub translate.
  - `loadModuleConfig()`: remove guard around LittleFS vs `std::ifstream` — use LittleFS unconditionally
  - `saveModuleConfig()`: remove guard around LittleFS vs `std::ofstream` — use LittleFS unconditionally
  - `buildBackupMeta()`: remove guard around `WiFi.macAddress()`, `ESP.getChipModel()`, `time()` — stubs provide defaults
  - `registerRoutes()`: remove PsychicHttp guard
  - `registerMCPTools()`: remove guard around LittleFS and timer operations

### ConfigManifest

- [ ] **`ConfigManifest.cpp`** — Remove all guards:
  - Include guard for `<LittleFS.h>` vs `<fstream>`, `<sstream>`
  - Path routing: use hardware path unconditionally
  - `load()`: remove LittleFS vs `std::ifstream` guard
  - `save()`: remove LittleFS vs `std::ofstream` guard

### LittleFS Stub Path Mapping

- [ ] **Verify LittleFS stub compatibility**: The stub must translate hardware paths (e.g., `/mod/wifi.json`) to local filesystem paths (e.g., `test/tmp/mod/wifi.json`). If the WO1 stub doesn't handle this, extend it. The key invariant: production code uses hardware paths; the stub silently prepends `test/tmp/`.
- [ ] **Verify**: `pio test -e native` and `pio run -e esp32s3nano`

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.

## Status Update
- **Status**: COMPLETE
- **Details**: `#ifdef NATIVE_BUILD` guards were fully removed from `ConfigManager` and `ConfigManifest`. Required FreeRTOS headers and definitions were added to the stub environment. Path mismatch in tests (`test_configManager.cpp` and `test_application.cpp`) was resolved by correctly provisioning directory paths (`test/tmp/mod`) and updating references.
- **Verification**: `pio test -e native` passes 100%. `pio run -e esp32s3nano` builds successfully. Changes committed. Context bridged and distilled.
