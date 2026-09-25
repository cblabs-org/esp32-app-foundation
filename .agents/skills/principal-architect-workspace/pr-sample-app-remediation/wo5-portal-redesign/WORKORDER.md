---
name: sample-app-remediation-wo5
description: Handoff instructions for Work Order 5 — Portal JS design system overhaul
complexity: standard
---

# WO-5: Portal JS Design System Overhaul

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Rewrite `weather.js` to use the framework's portal design system — CSS variables, component patterns, dark mode support, and proper state management (loading, error, configuration-needed).

## Parent Project
`sample-app-remediation` — see [PROJECT.md](../PROJECT.md) for human approval provenance.

## Context

The current `weather.js` (302 lines) uses extensive hard-coded inline styles rather than the framework's design system CSS variables and component patterns. It lacks proper configuration-needed state when preconditions are unmet, has no loading or error states, and has no auto-refresh.

## Strict Boundary Box
**IN SCOPE:** `sample-app/web/weather.js` ONLY.

**OUT OF SCOPE — DO NOT TOUCH:**
- `WeatherModule.*`, `WeatherBoard.*`, `WeatherWidget.*`, `IWeatherDataProvider.hpp`
- `main.cpp`
- Any BLE files
- Any framework modules or portal framework files (styles.css, portal.js, etc.)
- `platformio.ini`

## Tooling Constraints
- No build tool verification needed for this WO — it's a standalone JS file.
- **Read these design references first** (in this order):
  1. `/Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/framework-design-system/SKILL.md` — design system overview, tokens, and principles
  2. `/Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/framework-expert/systems/web-portal/html-js-fragments.md` — framework HTML/JS component patterns
  3. `/Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/framework-expert/systems/web-portal/overview.md` — portal architecture
  4. `/Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/framework-design-system/styles.css` — the actual framework CSS variables
  5. Look at other framework portal tab JS files for reference patterns. Search `modules/*/web/` for examples.

## Line-Item Instructions

### Phase 1: Audit Existing Code + Reference Patterns

- [ ] **Read** `sample-app/web/weather.js` fully to understand current structure
- [ ] **Read** at least 2 other portal tab JS files from the framework for reference patterns (search `modules/*/web/*.js`)
- [ ] **Note** all API endpoints used: `/api/weather/config` (GET/POST), `/api/weather/status` (GET)

### Phase 2: Rewrite weather.js

Rewrite the file from scratch, preserving all API endpoint usage but using proper design system patterns:

- [ ] **Remove all hard-coded inline CSS styles** — use only CSS variables from the framework's `styles.css`:
  - `--primary`, `--primary-hover` for accent colors
  - `--card-bg`, `--card-border` for cards
  - `--text-primary`, `--text-muted`, `--text-secondary` for text
  - `--border-color`, `--border-radius` for borders
  - `--spacing-*` for spacing if available, otherwise use `rem` units
  - `--success`, `--warning`, `--danger` for status colors

- [ ] **Use CSS classes instead of inline styles** — define a `<style>` block at the top of the template with class-based rules using CSS variables. Name classes with a `weather-` prefix to avoid collisions.

- [ ] **Implement three display states:**
  1. **Loading state**: Shown while fetching config/data. Simple centered spinner or "Loading..." text.
  2. **Configuration-needed state**: Shown when API key or location is missing. A prominent card with:
     - A clear message ("Weather station needs configuration")
     - Instructions to set API key and location
     - Still show the config form below so the user can configure immediately
  3. **Data state**: The weather summary card + config form. Show weather data when available.

- [ ] **Weather summary card** (shown when data is available):
  - Current temperature (large), condition, location
  - Detail grid: humidity, wind speed, pressure, visibility, feels like, temp range
  - Use `<article>` with `<header>` per framework patterns
  - Use CSS Grid or Flexbox with CSS variables for the detail grid

- [ ] **Configuration form:**
  - API Key input (password type, with show/hide toggle if the framework has one)
  - Latitude and Longitude inputs (number type)
  - Polling interval input
  - Units select (metric/imperial)
  - Save button using framework button classes
  - API key field: show "Key saved ✓" placeholder when key exists but don't expose the actual key

- [ ] **Dark mode compatibility**: All colors via CSS variables — no hardcoded hex colors. The `[data-theme="dark"]` selector is handled by the framework's CSS variables automatically if you use them consistently.

- [ ] **Auto-refresh**: When the tab is active, poll `/api/weather/status` every 60 seconds and update the summary card. Stop polling when the tab loses focus.

- [ ] **Error handling**: Show error messages from failed API calls in a dismissible alert or status message area.

- [ ] **Mobile-first**: Use `max-width` or responsive patterns. The portal targets 480px viewport width.

### Phase 3: Verify

- [ ] **Manually review** the generated JS for any remaining hard-coded colors or inline styles
- [ ] **Verify** all three API endpoints are still called correctly
- [ ] **Verify** the tab event listener pattern matches: `window.addEventListener('tab-changed', function(e) { if (e.detail !== 'tab-weather') return; ... })`

## Verification Requirement
1. **Code review**: Zero hard-coded inline CSS styles (no `style="..."` on elements except where absolutely necessary for dynamic values like display:none/flex toggling).
2. **Code review**: All styling via CSS variables from framework `styles.css`.
3. **CRITICAL**: Before committing, ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete. Implementation approved by coordinator.

### Changes Made
- Rewrote `sample-app/web/weather.js` from scratch to adhere to the ESP32 App Foundation portal design system.
- Completely removed all inline `style="..."` attributes and hard-coded hex colors, introducing a scoped `<style>` block using `.weather-` prefixed class names and standard framework CSS custom properties (`--primary`, `--primary-hover`, `--primary-alpha`, `--card-bg`, `--border`, `--text`, `--text-muted`, `--error`, `--warning`).
- Implemented three distinct display states:
  1. **Loading state (`#weather-loading`)**: Centered spinner and descriptive status text during initial async data loading.
  2. **Configuration-needed state (`#weather-config-needed`)**: Prominent callout card displayed when API key or location coordinates are missing, explaining requirements with a link to OpenWeatherMap, while retaining the config form below for immediate entry.
  3. **Data state (`#weather-summary-card`)**: Responsive card with primary temperature, apparent temperature, condition text, OWM icon, and a 6-item CSS Grid detail layout (Humidity, Wind Speed, Pressure, Visibility, Temp Range, Cloud Cover).
- Overhauled the configuration form:
  - Masked API key field displaying `"Key saved ✓"` when a key is already stored, preventing credential exposure in DOM.
  - Interactive Feather eye icon button to toggle password visibility during active input.
  - Status dot indicator (`.dot--green` / `.dot--red`) for API key state.
  - Save button with native `aria-busy` busy spinner support.
- Added lifecycle-aware auto-refresh:
  - Polls `GET /api/weather/status` every 60 seconds while the Weather tab is active.
  - Automatically ceases polling when navigating away or when document visibility is hidden (`visibilitychange`).
- Added dismissible error/warning alert banner (`#weather-alert`) for API connection issues and OpenWeatherMap return codes (Auth Error, Rate Limited, Network Error, Parse Error).
- Refactored supplementary diagnostics drawer to use `.info-drawer` and `.diag-table` component classes.

### Decisions Made
- Replaced dynamic inline `.style.*` DOM manipulations with clean classList toggles (`.weather-hidden`, `.dot--green`, `.dot--red`, `.dot--yellow`, `.dot--grey`) and standard HTML attributes (`disabled`, `aria-busy`).
- Used `color-mix()` with framework CSS tokens for subtle alert and callout background tints, ensuring automatic adaptability in both light and dark themes without hardcoding RGB values.
- Retained the diagnostics drawer below the configuration form so detailed backend state and raw metrics remain accessible without cluttering the primary weather view.

### Dead Ends & Warnings
- None.

### Out-of-Scope Findings
- None.

### Blockers
- None.
