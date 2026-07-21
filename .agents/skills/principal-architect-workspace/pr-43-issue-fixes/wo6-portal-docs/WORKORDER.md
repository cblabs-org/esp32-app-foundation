---
name: pr-43-wo6
description: Rewrite web portal docs — overview.md and html-js-fragments.md
complexity: standard
---

# WO6 — Rewrite Web Portal Docs

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Rewrite `overview.md` and `html-js-fragments.md` in the framework-expert web-portal system docs to describe the real tab contribution and JavaScript loading mechanism (post-WO3 state). Closes GitHub Issue #23.

## Strict Boundary Box

- **DO NOT** modify any C++ source or header files.
- **DO NOT** modify `save-policy.md` or `shared-utilities.md` — they are already correct.
- **DO NOT** modify `routes.md` — it was updated in WO5.
- **DO NOT** modify any files outside the two listed below.

## Prerequisites

WO3 must be complete. The docs must describe the post-WO3 API which includes:
- `getTabScript()` on `IWebContributor`
- `script` field in `/api/tabs` response
- Dynamic script loading in `app.js`

## Reference: The Real API (post-WO3)

Tab contribution: implement `IWebContributor` with overrides:
- `registerRoutes(PsychicHttpServer&)` — register REST API routes
- `getTabName()` — tab display name (nullptr = no tab)
- `getParentTab()` — parent tab for sub-navigation (nullptr = top-level)
- `getTabOrder()` — sort order (default 100)
- `getTabIcon()` — SVG inner content (nullptr = generic icon)
- `getTabScript()` — route path for tab JS (nullptr = no script) **[NEW in WO3]**

`/api/tabs` schema: `name`, `id`, `parent`, `order`, `icon`, `script`

Tab JS entry point: `window.addEventListener('tab-changed', handler)`
Container ID derivation: `'tab-' + name.toLowerCase().replace(/\s+/g, '-')`

## Checklist

### `.agents/skills/framework-expert/systems/web-portal/overview.md`

- [ ] **Full rewrite.** Replace the entire file content. The new version must cover:
  1. SPA architecture overview (keep the existing accurate description of single-page load)
  2. Tab declaration via `IWebContributor` overrides — list all 6 methods with descriptions
  3. The true `/api/tabs` schema (with the new `script` field from WO3)
  4. Boot sequence: SPA fetches `/api/tabs`, loads built-in + declared scripts, builds nav
  5. Remove ALL references to: `registerTab()`, `AppTabContributor` constructor with 5 params, `htmlUrl`, `jsUrl`, "Dynamic Fragment Loading"
  6. Note: `AppTabContributor` exists as a private struct in `Application.hpp` — do NOT claim it doesn't exist, but clarify it's an internal helper, not a public API

### `.agents/skills/framework-expert/systems/web-portal/html-js-fragments.md`

- [ ] **Full rewrite.** Replace the entire file content. The new version must cover:
  1. **Remove** all `tab.html` structure rules — there are no HTML fragments
  2. Tab JS is served as a C++ raw-string literal from a route (cite `WeatherModule` as example pattern)
  3. The `tab-changed` event contract:
     - Entry: `window.addEventListener('tab-changed', function(e) { if (e.detail === 'tab-weather') { ... } })`
     - Container ID derivation: `'tab-' + name.toLowerCase().replace(/\s+/g, '-')`
  4. **Keep** the IIFE wrapper guidance — still valid and important
  5. **Keep** the ID-prefixing rules — still valid
  6. **Remove** all references to `App.onTabLoaded` — does not exist
  7. **Add** auth note: tab scripts are served unauthenticated; sensitive data should come from authenticated `/api/` endpoints
  8. **Add** reference to the save-policy doc for settings persistence

## Verification

- [ ] Grep the updated files for removed terms: `registerTab`, `htmlUrl`, `jsUrl`, `onTabLoaded`, `tab.html`, `App.onTabLoaded`. None should appear.
- [ ] Verify the `/api/tabs` schema shown in the docs matches the actual fields emitted by `WebServerManager.cpp` (post-WO3).
- [ ] Verify the container-ID derivation rule matches what `app.js` actually does.
