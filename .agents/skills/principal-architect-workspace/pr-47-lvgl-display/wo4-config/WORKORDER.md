---
name: lvgl-display-wo4
description: Handoff instructions for Work Order 4 — configuration persistence (IConfigurable)
complexity: standard
---

# lvgl-display / WO4 — Configuration Persistence (IConfigurable)

## Provenance & Authorization
- **Parent project/PR:** PR-47 — [`../PROJECT.md`](../PROJECT.md)
- **Base branch:** `lvgl-display/wo3-module-core`
- **Your branch:** `lvgl-display/wo4-config` (created for you — **do not branch**)
- **Stack position:** WO4 of 11.
- **Authorization:** WORKORDER + `.agents/rules/`, valid once PR-47 `PROJECT.md` records the human's decomposition approval. Verify; if absent, **stop and report**.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Make `LVGLDisplayModule` an `IConfigurable`: persist and apply display settings (brightness, rotation, screensaver timeouts) through the ConfigManager gateway, with module-owned defaults.

## Context You Need
- Read the `framework-expert` `IConfigurable` interface docs and the Config System docs; mirror `U8G2DisplayModule`'s config handling (`applyConfig`, module-owned defaults via a `resetConfig()`-style default, per the PR-20 pattern where modules own their runtime defaults, not `FrameworkConfig`).
- ConfigManager is the gateway (persists to `/etc/config.json` on the `storage` LittleFS partition). Do not read/write LittleFS directly — go through the ConfigManager API as U8G2 does.
- `setBrightness` must delegate to `_adapter.setBrightness(level)`.

## Strict Boundary Box — do NOT:
- Implement screens, EventBus, screensaver behaviour (WO5/WO6), web routes (WO7), or MCP tools (WO8).
- Add new config keys that belong to those later WOs (e.g. do not implement screensaver *behaviour* here — only persist the timeout values it will consume; coordinate the schema so WO6 reads them).
- Bypass ConfigManager or introduce a second persistence path.

## Tooling Constraints
`platformio` MCP tools (`build_project`, `run_tests`) via `pio-manager` — never raw `pio`. `house-style-docs`. Heed `.agents/rules/`.

## Line-Item Instructions
- [ ] Add `public IConfigurable` to `LVGLDisplayModule`; implement `applyConfig(const JsonObject& section)` for brightness (0–255), rotation (0/90/180/270), and screensaver timeout fields (dim/blank), with validation and sane module-owned defaults.
- [ ] Provide the module-owned default config (mirror U8G2's `resetConfig()`/defaults approach) so a fresh device gets correct values without `FrameworkConfig`.
- [ ] Wire brightness apply → `_adapter.setBrightness()`. Persist rotation intent for the display (apply where LVGL supports it; if rotation requires adapter support, expose a seam and document it — do not overreach into WO9).
- [ ] Add `test/test_unit/test_lvglDisplay/test_config.cpp` (mirror `test_u8g2Display/test_config.cpp`): apply valid/invalid JSON, defaults on empty, brightness clamps, adapter `setBrightness` called with expected value (via mock adapter).

## Verification Requirement
- [ ] `run_tests` (native) — config tests pass, full suite green.
- [ ] `build_project` `env:esp32s3nano` green.

## Completion Protocol
1. Validate, then **STOP and message the Principal Architect** with results and the config schema you settled on (so WO6 can consume the screensaver timeouts). Surface out-of-boundary findings; do not file issues.
2. **Do not distill until the human approves.**
3. On approval: `@distillery` with target `.agents/skills/principal-architect-workspace/pr-47-lvgl-display/wo4-config/`.
4. Append a status update to this `WORKORDER.md` and terminate.
