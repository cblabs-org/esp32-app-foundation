---
name: 146-config-gate-wo5
description: Handoff instructions for Work Order 5 — Portal API & UI
complexity: standard
---

# WO5 — Portal API & Precondition Health UI

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Expose precondition health via a REST API and render it prominently in the portal UI. This is CG-18's "not every consumer has a display" — the web portal is the universal surfacing mechanism.

## Context

Read:
- `docs/config-gate-design.md` — CG-18 (surfacing: portal mandatory), CG-18b (portal surfacing details)
- The design skill at `.agents/skills/framework-design-system/SKILL.md` — for portal UI guidelines, colors, typography
- `modules/WebServer/WebServerManager.cpp` — how routes are registered and the `/api/tabs` pattern
- `modules/Application/Application.hpp` — `getPreconditionEngine()` accessor
- `web/portal/app.js` — SPA structure, dynamic script loading, `GET /api/tabs`
- `web/portal/system.js` — system page: status display, existing card rendering

**Key design decisions from CG-18:**
- Portal surfacing is **mandatory and prominent** — it goes on the System status page.
- Nobody watches a portal; they open it *because* something is wrong.
- Display surfacing rides the published `precondition.health` EventBus topic (WO6).
- The API returns a snapshot; the portal polls it (same pattern as `/api/status`).

## Strict Boundary Box

- **DO NOT** modify `PreconditionEngine`, `IPrecondition`, or the core types.
- **DO NOT** modify display modules — that is WO6.
- **DO NOT** modify sample-app or template-app.
- **DO NOT** delete any old files — that is WO7.
- Stay within `WebServerManager`, `Application` (route registration only), and `web/portal/`.

## Tooling Constraints

- Use `platformio` MCP `build_project` (env: `esp32dev` or `native`) for compilation.
- The portal asset pipeline uses `python3 web/portal/portalBuilder.py` — read the build process.
- If the framework-design skill is available, read it for UI component patterns before writing JS/CSS.

## Line-Item Instructions

- [ ] **`modules/Application/Application.cpp`** — Register a new route in the web contributor section:
  Add a `GET /api/preconditions` route that returns a JSON array of all preconditions. For each:
  ```json
  {
    "name": "WiFi Station",
    "role": "Startup",
    "met": false,
    "cause": "Connecting",
    "severity": "Transient",
    "everMet": true,
    "breaker": "Closed",
    "suppressed": false
  }
  ```
  Use `PreconditionEngine::getPreconditionCount()` and `getStateAt(i)`. Serialize enums as strings. Use ArduinoJson (the framework's standard JSON library).

  Alternatively, if `Application` already implements `IWebContributor::registerRoutes()`, add the route there. If the route should live in a separate contributor, create a `PreconditionWebContributor` inner class. Follow the existing pattern.

- [ ] **`web/portal/system.js`** — Add a precondition health panel to the System page:
  1. **Fetch** `GET /api/preconditions` on page load and on a 5-second polling interval.
  2. **Render** a "Precondition Health" card/section prominently (above or alongside the existing system info). For each precondition:
     - Name
     - Status badge: ✓ met (green), ✗ unmet (red/amber by severity)
     - Cause text (the `cause` field, human-readable)
     - Role badge: Startup | Operational
     - If severity is `NeedsUser` or `Persistent`: highlight row with warning styling
     - If breaker is `Open`: show "Paused — retry" affordance
  3. **Optional: Retry button.** If a precondition's breaker is `Open`, show a "Retry" button that `POST /api/preconditions/{name}/retry` — add this route too. The route calls `_preconditionEngine.requestRetry(name)`.
  4. Use the framework portal CSS classes. Match the existing card/panel styling.

- [ ] **Build the portal assets:** Run `python3 web/portal/portalBuilder.py` to regenerate the gzipped portal assets. Verify the build succeeds.

- [ ] Verify: full build succeeds on target environment.

## Verification Requirement

1. `GET /api/preconditions` returns valid JSON with all registered preconditions.
2. Portal system page renders the precondition health panel.
3. If retry affordance is implemented: POST retry endpoint works.
4. Asset pipeline builds cleanly.
5. Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

Surface any findings in the Completion Report — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
(pending)

### Changes Made
(pending)

### Decisions Made
(pending)

### Dead Ends & Warnings
(pending)

### Out-of-Scope Findings
(pending)

### Blockers
(pending)
