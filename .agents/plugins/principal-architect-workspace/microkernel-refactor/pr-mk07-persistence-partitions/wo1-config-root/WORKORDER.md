---
name: pr-mk07-wo1
description: Handoff instructions for Work order 1 (Config Root Abstraction)
complexity: standard
---
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Abstract the hardcoded filesystem paths (`/etc`, `/mod`) in `KeyManager`, `AuthManager`, `ConfigManager`, and `WebServerManager` to use a central configuration root accessor, preparing the system for a layout-agnostic mount where config might live on a dedicated partition or fallback to the app partition.

## Strict Boundary Box
- **DO NOT** implement the partition mount logic itself (that is WO2).
- **DO NOT** modify any core logic of how the config is parsed, keys are generated, or certs are saved. Only change the path string generation/usage.
- **DO NOT** remove the directories from the paths, just route them through a central getter.
- Only modify `KeyManager`, `AuthManager`, `ConfigManager`, `WebServerManager`, and where the central accessor is defined.

## Tooling Constraints
- Use the `platformio` MCP server tools (e.g., `build_project`) for compiling and verifying. Do NOT use raw `pio` CLI commands.

## Line-Item Instructions

- [x] Create a static `ConfigRoot` utility class (e.g., in `modules/ConfigManager/ConfigRoot.hpp` and `.cpp`) that exposes methods like `static const char* getEtcPath()` (returns `"/etc"`) and `static const char* getModPath()` (returns `"/mod"`). For now, these just return the hardcoded strings. They will be made dynamic in WO2. Provide helpers for constructing full paths to avoid repeating concatenation logic across modules.
- [x] Update `modules/AuthManager/AuthManager.cpp` and `AuthManager.hpp` to replace all hardcoded `"/etc"` and `"/etc/server.key"`, `"/etc/server.crt"` strings with calls to `ConfigRoot`.
- [x] Update `modules/KeyManager/KeyManager.cpp` and `KeyManager.hpp` to replace hardcoded `"/etc"` and `"/etc/keys.json"` with the new accessor.
- [x] Update `modules/ConfigManager/ConfigManager.cpp` and `ConfigManager.hpp` to replace all hardcoded `"/mod"` and `"/mod/<ns>.json"` string generation with the new accessor.
- [x] Update `modules/WebServer/WebServerManager.cpp` where it loops over `/etc` and `/mod` to erase them (lines ~470-500) to use the accessor.
- [x] Update `#include` directives in these modules as needed to include `ConfigRoot.hpp`.
- [x] Build the project for `native`, `esp32dev`, and `esp32s3nano` using the `build_project` MCP tool to verify it still compiles cleanly.
- [x] **CRITICAL**: Ask the user if they are happy before completing. Only after approval, invoke the `@distillery` skill to package the state passing `.agents/skills/principal-architect-workspace/microkernel-refactor/pr-mk07-persistence-partitions/wo1-config-root/` as the target directory.
- [x] Append a status update to this `WORKORDER.md` file indicating completion.

## Status Update

- **Date**: 2026-07-29
- **Status**: ✅ **COMPLETED**
- **Branch**: `pr-mk07-persistence-partitions/wo1-config-root`
- **Commit**: `0770a96`
- **Summary**: Implemented `ConfigRoot` static path accessor utility. Abstracted all hardcoded system (`/etc`) and module (`/mod`) paths across `AuthManager`, `KeyManager`, `ConfigManager`, and `WebServerManager`. Verified clean compilation on `esp32dev` and `esp32s3nano` via PlatformIO MCP tools and passed 401 unit test cases. Human approved changes and distillery package created.

