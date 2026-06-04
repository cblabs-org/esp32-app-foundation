---
name: pr-18-bridge
description: Bridge prompt for Principal Architect to execute PR-18 Screensaver & Display Settings
---

# PR-18: Screensaver & Display Settings — Bridge Prompt

## Context

The PR-18 project has been fully scoped and all work order documents are written.
The project adds comprehensive screensaver control and display settings across MCP tools,
web portal, REST API, and persistent config.

## Your Task

You are the Principal Architect for **PR-18 — Screensaver & Display Settings**.

1. Read your project file: `.agents/skills/principal-architect-workspace/pr-18-screensaver-settings/PROJECT.md`
2. The current working branch is `pr-17-wo4-module-root` — you will need to create the PR-18 feature branch from `main` (or wherever appropriate)
3. Create WO branches and spawn workers sequentially:
   - **WO1** (config expansion) → **WO2** (MCP tools) → **WO3** (web portal)
4. Each WO has a complete `WORKORDER.md` ready to hand off — they are located at:
   - `.agents/skills/principal-architect-workspace/pr-18-screensaver-settings/wo1-config-expansion/WORKORDER.md`
   - `.agents/skills/principal-architect-workspace/pr-18-screensaver-settings/wo2-mcp-tools/WORKORDER.md`
   - `.agents/skills/principal-architect-workspace/pr-18-screensaver-settings/wo3-display-tab/WORKORDER.md`

## Important Notes

- The display module ownership was just refactored (PR-17 WO4) — `U8G2DisplayModule` is now
  owned by `Application` as a member. The current branch has these changes but they may not
  be on `main` yet. Ensure the PR-18 branch includes these changes.
- The `ScreensaverController` already has `setDimTimeout()`, `setBlankTimeout()`, `setPolicy()`,
  `resetActivity()`, `isActive()`, `isDimmed()`, `isBlanked()` — WO1 only adds `setEnabled()`/`isEnabled()`
- The web portal follows the mandatory `/web-testing-strategy` workflow — WO3 workers MUST read it
- All three WOs are serialized (not parallel) since they touch overlapping files in U8G2DisplayModule

## Execution

Start with WO1. Read the WORKORDER.md, create the branch, spawn the worker, and manage the pipeline.
