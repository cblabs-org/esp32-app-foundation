---
name: lvgl-display-wo7
description: Handoff instructions for Work Order 7 — web portal REST + Display tab + capture primitive
complexity: standard
---

# lvgl-display / WO7 — Web Portal: REST Endpoints, Display Tab & Capture Primitive

## Provenance & Authorization
- **Parent project/PR:** PR-47 — [`../PROJECT.md`](../PROJECT.md)
- **Base branch:** `lvgl-display/wo6-eventbus-screensaver`
- **Your branch:** `lvgl-display/wo7-web-portal` (created for you — **do not branch**)
- **Stack position:** WO7 of 11.
- **Authorization:** WORKORDER + `.agents/rules/`, valid once PR-47 `PROJECT.md` records the human's decomposition approval. Verify; if absent, **stop and report**.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Make `LVGLDisplayModule` an `IWebContributor`: expose `/api/display/*` REST endpoints, contribute a module-owned "Display" portal tab, and implement the on-demand display **capture primitive** (OQ-6) feeding a screenshot endpoint.

## Context You Need
- Read the `framework-expert` Web Portal docs and `IWebContributor` contract; mirror the module-owned tab pattern from PR-23/PR-18 and `web/portal/display.js`. Existing portal JS fragments live in `web/portal/`; the portal is served from LittleFS and built by `scripts/portalBuilder.py`.
- **OQ-6 capture approach (decided):** LVGL partial buffers keep no full framebuffer, so implement an **on-demand snapshot**: allocate a transient full-screen RGB565 buffer in **PSRAM**, force a full-refresh render into it, encode (e.g. raw/RGB565 or PNG as the portal expects), return it, then free. Not in the hot path — PSRAM cost acceptable. This primitive is reused by WO8's `capture_display` MCP tool, so expose it as a clean module method.
- Playwright web tests live under `test/web/` / `test/test_e2e` with a mock server (PR-15). Mirror existing display tab specs.

## Strict Boundary Box — do NOT:
- Implement MCP tools (WO8) — but **do** expose the capture primitive as a reusable method WO8 will call.
- Touch auth/security beyond following the existing route-registration + auth pattern used by peer modules.
- Store screenshots or assets in LittleFS persistently; the snapshot is transient.
- Modify unrelated portal tabs.

## Tooling Constraints
`platformio` MCP tools (`build_project`, `run_tests`) via `pio-manager` — never raw `pio`. Web tests run via the existing `scripts/run_web_tests.py` / Playwright harness. `house-style-docs` for C++ and follow existing JS conventions in `web/portal/`.

## Line-Item Instructions
- [ ] Add `public IWebContributor`; implement `registerRoutes(PsychicHttpServer&)` for `/api/display/*` (status, settings GET/POST mapping to WO4 config, brightness, screenshot). Follow the peer-module route + auth pattern.
- [ ] Contribute the module-owned "Display" tab (JS fragment in `web/portal/`, e.g. `lvgldisplay.js` or extend the display tab per the established pattern), with settings UI bound to the REST API.
- [ ] Implement the capture primitive (PSRAM transient full-frame snapshot) as a module method + wire the screenshot endpoint to it.
- [ ] Add/extend Playwright specs for the Display tab (render, settings round-trip, screenshot endpoint) mirroring existing specs.
- [ ] Native unit tests for the route handlers/config mapping where feasible (`test/test_unit/test_lvglDisplay/`).

## Verification Requirement
- [ ] `run_tests` (native) — green.
- [ ] Web/Playwright suite green via the existing harness.
- [ ] `build_project` `env:esp32s3nano` green.

## Completion Protocol
1. Validate, then **STOP and message the Principal Architect** with results and the capture-primitive method signature (WO8 depends on it). Surface out-of-boundary findings; do not file issues.
2. **Do not distill until the human approves.**
3. On approval: `@distillery` with target `.agents/skills/principal-architect-workspace/pr-47-lvgl-display/wo7-web-portal/`.
4. Append a status update to this `WORKORDER.md` and terminate.
