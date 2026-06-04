---
name: pr-18-wo1
description: Handoff instructions for Work Order 1 — Expand display config schema with independent screensaver settings
---

# PR-18 / WO1 — Config Expansion

You are an expert senior software engineer and execution-focused worker agent. You are NOT
the Principal Architect. Do not make high-level architectural decisions, do not branch, and
do not plan. Your task is strictly limited to executing the checklist below.

---

## Objective

Expand the `display` config namespace with independent screensaver controls:
`screensaverEnabled` (bool), `dimTimeout` (ms), and `blankTimeout` (ms). Currently
`dimTimeout` is hardcoded to `sleepTimeout / 2`. After this WO, all three are
independently configurable, persisted, and exposed via the existing REST API.

---

## Git Context

You are working on branch **`pr-18-wo1-config-expansion`**. Check it out before making any changes:

```bash
git checkout pr-18-wo1-config-expansion
```

Do NOT commit to any other branch. Do NOT merge. The Principal Architect handles all merges.

---

## Strict Boundary Box

| ✅ You MAY touch | ❌ You MUST NOT touch |
|---|---|
| `modules/U8G2Display/U8G2DisplayModule.hpp` (add enable flag) | `Application.cpp` or `main.cpp` |
| `modules/U8G2Display/U8G2DisplayModule.cpp` (config methods + REST routes) | Board `.hpp` / `.cpp` files |
| `modules/U8G2Display/ScreensaverController.hpp` (add enable/disable API) | `CarouselEngine.*` |
| `modules/U8G2Display/ScreensaverController.cpp` (implement enable/disable) | `BoardCatalog.hpp` |
| `test/test_unit/test_u8g2Display/` (new/updated tests) | Web portal files (`web/portal/`) |

---

## Pre-Work Reading (Mandatory)

Before writing a single line of code, read these files in order:

1. `modules/U8G2Display/ScreensaverController.hpp` — full API surface
2. `modules/U8G2Display/ScreensaverController.cpp` — state machine implementation
3. `modules/U8G2Display/U8G2DisplayModule.cpp` lines 480–560 — `applyConfig()`, `serializeConfig()`, `resetConfig()`
4. `modules/U8G2Display/U8G2DisplayModule.cpp` lines 660–775 — REST API routes (`/api/display/status`, `/api/display/sleep`)
5. `.agents/skills/house-style-docs/references/cpp-embedded.md` — house style

---

## Implementation Checklist

### Step 1 — Add `setEnabled()` / `isEnabled()` to `ScreensaverController`

- [ ] Add to `ScreensaverController.hpp`:
  ```cpp
  void setEnabled(bool enabled);
  bool isEnabled() const;
  ```
- [ ] Add private member: `bool _enabled = true;` (default ON — preserves current behaviour)
- [ ] Implement in `ScreensaverController.cpp`:
  - `setEnabled(bool)`: Set `_enabled` flag. If disabling and currently dimmed/blanked, call `_wake()` to restore
  - `isEnabled()`: Return `_enabled`
- [ ] Modify `tick()`: Early-return if `!_enabled` (skip all timeout checks)

### Step 2 — Rename `sleepTimeout` → split into `dimTimeout` + `blankTimeout`

The existing config has a single `sleepTimeout` that maps to `blankTimeout`, with
`dimTimeout = sleepTimeout / 2`. Make them independent.

- [ ] In `applyConfig()`:
  - Read `screensaverEnabled` (bool, default `true`)
  - Read `dimTimeout` (unsigned long, default `150000` — 2.5 min)
  - Read `blankTimeout` (unsigned long, default `300000` — 5 min)
  - **Migration**: If `sleepTimeout` key exists (old config), derive `dimTimeout = sleepTimeout / 2` and `blankTimeout = sleepTimeout`, then ignore the old key
  - Wire: `_screensaver.setEnabled(enabled)`, `_screensaver.setDimTimeout(dimTimeout)`, `_screensaver.setBlankTimeout(blankTimeout)`

- [ ] In `serializeConfig()`:
  - Write `screensaverEnabled`, `dimTimeout`, `blankTimeout`
  - Remove the old `sleepTimeout` key

- [ ] In `resetConfig()`:
  - Set `_screensaver.setEnabled(true)`, `dimTimeout = 150000`, `blankTimeout = 300000`

- [ ] Bump `configVersion()` return value by 1 (triggers migration on next boot)

### Step 3 — Update `getDimTimeout()` accessor

- [ ] Add `getDimTimeout()` to `ScreensaverController.hpp` if not already present:
  ```cpp
  unsigned long getDimTimeout() const { return _dimTimeoutMs; }
  ```

### Step 4 — Update REST API routes

- [ ] `GET /api/display/status` — add fields:
  ```json
  {
    "screensaverEnabled": true,
    "dimmed": false,
    "dimTimeout": 150000,
    "blankTimeout": 300000,
    ...existing fields...
  }
  ```
  Replace the old `sleepTimeout` key with `dimTimeout` + `blankTimeout`.

- [ ] `POST /api/display/sleep` — extend to accept:
  ```json
  {
    "action": "sleep" | "wake",
    "enabled": true/false,
    "dimTimeout": 150000,
    "blankTimeout": 300000
  }
  ```
  Each field is optional. If `enabled` is provided, call `_screensaver.setEnabled()` and persist.
  If `dimTimeout`/`blankTimeout` provided, set and persist.

### Step 5 — Unit Tests

- [ ] Add tests in `test/test_unit/test_u8g2Display/` for:
  - `ScreensaverController.setEnabled(false)` prevents dim/blank
  - `ScreensaverController.setEnabled(true)` restores normal timeout behaviour
  - Disabling while dimmed/blanked triggers wake
  - Independent dimTimeout and blankTimeout work correctly
  - Config round-trip: serialize → apply → serialize produces identical output
  - Migration: old `sleepTimeout` key converts to `dimTimeout`/`blankTimeout`

---

## Verification Requirement

```bash
# Native unit tests
pio test -e native

# Hardware build
pio run -e esp32s3nano
```

---

## Completion Protocol

1. Verify both builds pass and all tests are green
2. **STOP. Send a message back to the Principal Architect** with:
   - Build pass confirmation (native + hardware)
   - Test results (total count, new tests added)
   - Any deviations from the checklist (with justification)
   - A request for approval before wrapping up

> Do NOT distill, do NOT commit, do NOT merge until the Principal Architect explicitly approves.

---

## Status

**COMPLETED** — 2026-06-04T15:19:00Z

| Verification | Result |
|---|---|
| `pio test -e native` (test_u8g2Display) | ✅ PASSED — all screensaver + config tests green |
| `pio run -e esp32s3nano` | ✅ SUCCESS — RAM 15.5%, Flash 58.5% |

**Tests added**: 14 new test cases (13 screensaver + 1 config migration)
**Pre-existing failures**: `test_application` + `test_configManager` ERRORED (MAX_CONFIGURABLES removal — unrelated to WO1)

**Deviation**: `tick()` wake-request check reordered to precede `!_enabled` guard (see `context_artifacts/adr_tick_wake_ordering.md`). Approved by Principal Architect.

**Session**: `e4941b47-2ef8-4ad6-b023-004ca8ab37f7`

