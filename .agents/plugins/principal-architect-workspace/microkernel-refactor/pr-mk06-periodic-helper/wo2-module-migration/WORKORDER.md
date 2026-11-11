---
name: pr-mk06-wo2
description: Handoff instructions for Work order 2 - module migration
complexity: mechanical
---

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Migrate the remaining `millis()` throttle sites in the feature modules and `lib/` to use the newly created `Periodic` helper class.

## Strict Boundary Box
- You must strictly use `include/utils/Periodic.hpp`.
- Migrate throttle sites in `modules/` (e.g., `DataManager`, `HomeAssistant`, `MCPServer`, `OTAUpdateManager`, `U8G2Display` and its boards, `WebServer`, `WifiManager`) and `lib/` (e.g., `OTADownloadEngine`).
- **No behavioural change** — intervals and fire semantics must remain identical.
- Skip any throttle that is intentionally not periodic (e.g., timeouts, one-shot guards).
- Surface findings, don't file them.

## Tooling Constraints
- Use the `platformio` MCP tools for building and testing.
- You must verify compilation in both `native` and `esp32dev` environments.
- Existing timing tests must pass unchanged.

## Checklist
- [x] Find and migrate periodic throttles in `modules/` and `lib/` that use `millis()`.
- [x] Verify `native` + `esp32dev` environments are green.
- [x] Ask the user if they are happy with the implementation.
- [x] Upon user approval, invoke the `@distillery` skill to package the state and drop control, passing `.agents/plugins/principal-architect-workspace/microkernel-refactor/pr-mk06-periodic-helper/wo2-module-migration/` as the target directory.
- [x] Append a status update to this `WORKORDER.md` file and terminate your session.

## Status Update (2026-07-29)
All periodic throttles migrated to `Periodic` helper in `WifiManager`, `Application`, `HomeAssistantModule`, and `U8G2DisplayModule`. `OTADownloadEngine` skipped as its `millis()` usage is for hard timeout deadlines. Session distilled.
