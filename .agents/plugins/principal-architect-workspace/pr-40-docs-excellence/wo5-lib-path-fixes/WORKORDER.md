---
name: docs-excellence-wo5
description: Handoff instructions for Work Order 5 — Library header attribution fixes + module path casing fixes
complexity: mechanical
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Fix missing attribution ("CB Labs" + repo URL) in 3 library headers and correct module path casing in 5 framework .cpp file headers. These are mechanical find-and-fix operations.

# Strict Boundary Box

- **ONLY** modify the specific files listed below
- Do NOT touch sample-app, template-app, or any other files
- Do NOT modify any logic or behavior — header comment blocks only
- Do NOT create git branches or make commits

# Tooling Constraints

- Use `platformio` MCP tools for builds: `build_project`
- Build the main firmware to verify nothing was broken

# Line-Item Instructions

## 1. Library header attribution fixes

Each of these 3 files has an incomplete header — missing "CB Labs" org name and/or the repository URL. Fix them to match the standard format:

```
/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: [correct module path]
 * Description: [keep existing description]
 * ...
 */
```

- [ ] `lib/Logger/Logger.hpp` — currently has `(c) 2026` without "CB Labs" and no repo URL. Add both.
- [ ] `lib/DeviceCrypto/DeviceCrypto.hpp` — currently has `(c) 2026` without "CB Labs" and no repo URL. Add both.
- [ ] `lib/MD5Utils/MD5Utils.hpp` — currently has `(c) 2026` without "CB Labs" and no repo URL. Add both.

## 2. Module path casing fixes in .cpp headers

Each of these 5 files has a `Module:` line in its header comment with incorrect casing. The module path must match the actual filesystem path exactly (PascalCase directories and filenames):

- [ ] `modules/MCPServer/MCPServer.cpp` — change `Module: modules/mcpServer/mcpServer.cpp` → `Module: modules/MCPServer/MCPServer.cpp`
- [ ] `modules/MCPServer/MCPToolRegistry.cpp` — change `Module: modules/MCPServer/mcpToolRegistry.cpp` → `Module: modules/MCPServer/MCPToolRegistry.cpp`
- [ ] `modules/MCPServer/MCPBuiltinTools.cpp` — change `Module: modules/mcpServer/mcpBuiltinTools.cpp` → `Module: modules/MCPServer/MCPBuiltinTools.cpp`
- [ ] `modules/KeyManager/KeyManager.cpp` — change `Module: modules/KeyManager/keyManager.cpp` → `Module: modules/KeyManager/KeyManager.cpp`. Also add missing "CB Labs" and repo URL if not present.
- [ ] `modules/DataManager/DataManager.cpp` — change `Module: modules/DataManager/dataManager.cpp` → `Module: modules/DataManager/DataManager.cpp`

# Verification

- [ ] Build the main firmware: `pio run -e esp32dev` — must succeed (comments-only changes should never break builds, but verify)
- [ ] Run `grep -rn "Module:" lib/Logger/Logger.hpp lib/DeviceCrypto/DeviceCrypto.hpp lib/MD5Utils/MD5Utils.hpp modules/MCPServer/MCPServer.cpp modules/MCPServer/MCPToolRegistry.cpp modules/MCPServer/MCPBuiltinTools.cpp modules/KeyManager/KeyManager.cpp modules/DataManager/DataManager.cpp` — verify all paths are now PascalCase and all headers have "CB Labs" + URL
- [ ] **STOP and ask the user if they are happy before distilling**
