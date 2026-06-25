---
name: docs-excellence-wo6
description: Handoff instructions for Work Order 6 — String stub consolidation + OTA naming fix
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Consolidate 4 duplicate `String` stub class definitions (used for `NATIVE_BUILD` compilation) into a single shared header. Also fix the `learnedManifestUrl_` naming convention violation in OTAUpdateManager.

# Strict Boundary Box

- **ONLY** modify the specific files listed below
- Do NOT touch sample-app or template-app
- Do NOT modify any runtime behavior — the String stub is only used under `#ifdef NATIVE_BUILD`
- Do NOT create git branches or make commits

# Tooling Constraints

- Use `platformio` MCP tools for builds: `build_project`
- Build the main firmware (esp32dev) AND native tests to verify both compilation paths work

# Context

The `NATIVE_BUILD` String stub is defined in 4 places with the `STRING_STUB_DEFINED` include guard:
1. `lib/Logger/Logger.hpp` (L28-29)
2. `modules/OTAUpdateManager/OTAUpdateManager.hpp` (L75-76)
3. `modules/WifiManager/WifiManager.hpp` (L64-65)
4. `modules/MCPServer/MCPServer.hpp` (L84-85)

Each has a slightly different implementation. The consolidation creates one authoritative definition.

# Line-Item Instructions

## 1. Create shared native-build stubs header

- [ ] Create `include/platform/NativeBuildStubs.hpp` with:
  - House-style header block (full attribution, license, module path, description)
  - The consolidated String stub class under `#ifdef NATIVE_BUILD` with `STRING_STUB_DEFINED` guard
  - Examine all 4 existing stubs — take the most complete implementation (likely the one with `c_str()`, `length()`, `operator=`, `operator==`, etc.)
  - Include a comment explaining why this file exists: "Provides stub implementations of Arduino/ESP-IDF types for native desktop compilation and unit testing"
  - Also check if any other stubs (e.g., `WiFiClient`, `AsyncWebServerRequest`) could be consolidated here — but only move stubs that are duplicated, not one-off stubs

## 2. Update MCPServer.hpp

- [ ] Remove the inline `#ifdef NATIVE_BUILD` / `#ifndef STRING_STUB_DEFINED` String stub block
- [ ] Add `#include "../../include/platform/NativeBuildStubs.hpp"` in the `#ifdef NATIVE_BUILD` section (ensure it's included before any code that uses `String`)

## 3. Update WifiManager.hpp

- [ ] Remove the inline String stub block
- [ ] Add the shared include

## 4. Update OTAUpdateManager.hpp

- [ ] Remove the inline String stub block
- [ ] Add the shared include
- [ ] Rename member variable `learnedManifestUrl_` → `_learnedManifestUrl` (trailing underscore → leading underscore per naming convention)

## 5. Update OTAUpdateManager.cpp

- [ ] Find and replace all references to `learnedManifestUrl_` → `_learnedManifestUrl`
- [ ] Verify the JSON key in `serializeConfig()` and `applyConfig()` remains `"learnedManifestUrl"` (only the C++ variable name changes, not the JSON key)

## 6. Update Logger.hpp

- [ ] Remove the inline String stub block
- [ ] Add the shared include

# Verification

- [ ] Build main firmware for esp32dev: `pio run -e esp32dev` — must succeed
- [ ] Build and run native tests: `pio test -e native` — must pass (this exercises the NATIVE_BUILD code path)
- [ ] Run `grep -rn "STRING_STUB_DEFINED" --include="*.hpp" .` — should now show only `include/platform/NativeBuildStubs.hpp`
- [ ] Run `grep -rn "learnedManifestUrl_" modules/OTAUpdateManager/` — should return zero results
- [ ] Run `grep -rn "_learnedManifestUrl" modules/OTAUpdateManager/` — should show the renamed references
- [ ] **STOP and ask the user if they are happy before distilling**
