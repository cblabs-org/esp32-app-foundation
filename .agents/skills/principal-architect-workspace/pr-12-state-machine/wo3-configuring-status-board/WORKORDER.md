---
name: pr-12-wo3
description: Handoff instructions for Work Order 3 — ConfiguringStatusBoard
---

# WO3 — ConfiguringStatusBoard (New Display Board)

You are an expert senior software engineer and execution-focused worker agent.

## CRITICAL: WORKSPACE
ALL operations MUST use `/Users/mcneillm/Documents/Projects/esp32-app-foundation` as the working directory. Do NOT use `departures-board`.

## Objective

Create a new system board that shows dynamic gate progress during the CONFIGURING phase. This board renders:
- A "Configuring..." header
- Dynamic rows for each registered config gate with status icons
- A progress bar showing `(satisfied count) / (total gates)`

## Branch

Switch to branch `pr-12-wo3-configuring-board` before making any changes:
```bash
git checkout pr-12-wo3-configuring-board
```

## Strict Boundary Box

- ✅ You MAY create: `modules/u8g2Display/boards/configuringStatusBoard.hpp`, `modules/u8g2Display/boards/configuringStatusBoard.cpp`
- ✅ You MAY modify: `modules/u8g2Display/u8g2DisplayModule.hpp`, `modules/u8g2Display/u8g2DisplayModule.cpp`
- ❌ You MUST NOT modify: `AppState.hpp`, `Application.cpp`, `DataManager`, `ConfigManager`, `WifiManager`

## Context — Existing Board Architecture

Before creating the new board, study the existing board patterns:
1. Read `modules/u8g2Display/boards/errorBoard.hpp` — see how `IDisplayBoard` is implemented
2. Read `modules/u8g2Display/boards/bootBoard.hpp` — another system board example
3. Read `modules/u8g2Display/u8g2DisplayModule.hpp` — see how boards are registered and switched
4. Read `modules/u8g2Display/u8g2DisplayModule.cpp` — see `handleConfiguringEvent()` and how boards are shown/dismissed
5. Read `include/interfaces/IConfigGate.hpp` — the gate interface your board will consume

## Design Specification

```
Configured device restart (fast path):
┌──────────────────────────────────────────┐
│  Configuring...                          │
│                                          │
│  ✓ WiFi connected                        │
│  ✓ NTP synchronised                      │
│  ⟳ Checking for updates...               │
│  ○ API keys validated                    │
│                                          │
│  ▓▓▓▓▓▓▓▓▓▓░░░░░░░  55%                 │
└──────────────────────────────────────────┘
```

**Status icons:**
- `✓` (checkmark) = `isSatisfied() == true`
- `⟳` (spinner/rotating) = currently being evaluated (the first unsatisfied gate)
- `○` (empty circle) = not yet reached / waiting

**Progress bar:** `(satisfied count) / (total gates)` expressed as percentage

**Behaviour:**
- Board shows during CONFIGURING state
- When a gate needs user action (e.g., WiFi wizard), the display module should overlay the helper board
- Board auto-dismisses when all gates satisfied (via `app.running` event)
- On regression from RUNNING, the board reappears showing which gate(s) became unsatisfied

## Files to Create/Modify

### NEW: `modules/u8g2Display/boards/configuringStatusBoard.hpp`
- Class declaration implementing `IDisplayBoard`
- Constructor receives a pointer/reference to the gate list (vector of `IConfigGate*`)
- Standard `draw(U8G2& display)` method
- Uses the `messageBoardBase` pattern if appropriate, or standalone

### NEW: `modules/u8g2Display/boards/configuringStatusBoard.cpp`
- `draw()` implementation:
  1. Draw header "Configuring..." at top
  2. Iterate gates, draw each with appropriate icon
  3. Draw progress bar at bottom

### MODIFY: `modules/u8g2Display/u8g2DisplayModule.hpp`
- Add `ConfiguringStatusBoard _configuringStatusBoard` member
- Add any necessary includes

### MODIFY: `modules/u8g2Display/u8g2DisplayModule.cpp`
- In `handleConfiguringEvent()` (or equivalent): show `_configuringStatusBoard` as default board during CONFIGURING
- When a gate needs user action: overlay the helper board (WiFi wizard etc.)
- On `app.running` event: dismiss the configuring board and start carousel

## Line-Item Checklist

- [ ] Read existing board headers/implementations to understand the pattern
- [ ] Read u8g2DisplayModule.hpp/cpp to understand board management
- [ ] Read IConfigGate.hpp to understand the gate interface
- [ ] Create `configuringStatusBoard.hpp` — class declaration
- [ ] Create `configuringStatusBoard.cpp` — rendering implementation
- [ ] Modify `u8g2DisplayModule.hpp` — add member
- [ ] Modify `u8g2DisplayModule.cpp` — wire into configuring/running events
- [ ] Verify: `pio test -e native --filter "test_unit/test_u8g2Display"` — all tests pass
- [ ] Verify: both hardware targets compile clean (`pio run -e esp32s3nano`)

## Verification Requirement

After completing all items, run tests. Then **STOP and send a message back to me** reporting:
1. What files were created/modified
2. Whether tests pass
3. Any design decisions made

Do NOT commit until I explicitly approve.
