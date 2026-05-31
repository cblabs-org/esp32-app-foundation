---
name: pr-12-wo3-hotfix
description: Fix ConfiguringStatusBoard bugs and redesign layout
---

# WO3-HOTFIX — ConfiguringStatusBoard Bug Fixes + Layout Redesign

You are an expert senior software engineer and execution-focused worker agent.

## CRITICAL: WORKSPACE
ALL operations MUST use `/Users/mcneillm/Documents/Projects/esp32-app-foundation` as the working directory.

## Objective

Fix 4 bugs in the ConfiguringStatusBoard and redesign the layout to match the boot board's visual style. The board should show a **single-line status** per gate (cycling through them) with a progress bar in the same position as the boot screen.

## Branch

```bash
git checkout pr-12-state-machine
```

Work directly on `pr-12-state-machine` — no separate branch needed for hotfixes.

## Issues to Fix

### FIX 1: WiFi Wizard Not Showing (Logic Bug) 🔴

**File:** `modules/u8g2Display/u8g2DisplayModule.cpp`
**Location:** Late catch-up code at ~L218-230

The late catch-up path always shows `_configuringStatusBoard` even when the WiFi gate is unsatisfied. It should route to the WiFi wizard the same way `_onAppConfiguring()` does.

**Fix:** Replace the late catch-up block with logic that checks which gate is the first unsatisfied actionable gate:
```cpp
if (_app->getState() == AppState::CONFIGURING) {
    _configuringStatusBoard.setGates(&_app->getConfigGates());
    
    // Find the first unsatisfied gate and route to the appropriate board
    for (const auto* gate : _app->getConfigGates()) {
        if (!gate->isSatisfied()) {
            if (strcmp(gate->getGateName(), "WiFi Station") == 0) {
                const char* ssid = _app->getWifiManager().getSSID();
                const char* apName = (ssid && ssid[0] != '\0') ? ssid : _app->getConfig().hostname;
                _wifiWizard.configure(apName);
                showBoard(&_wifiWizard);
                ESP_LOGI(TAG, "App already in CONFIGURING — showing WiFi wizard (late catch-up)");
            } else {
                showBoard(&_configuringStatusBoard);
                ESP_LOGI(TAG, "App already in CONFIGURING — showing configuring status board (late catch-up)");
            }
            break;
        }
    }
}
```

### FIX 2: Layout Redesign — Match Boot Board Style 🔴

**File:** `modules/u8g2Display/boards/configuringStatusBoard.cpp` and `.hpp`

The current layout tries to show multiple gate rows + icons. The user wants a **single-line design** matching the boot board's LOADING phase layout:

**Boot board LOADING layout (reference — read `bootBoard.cpp` constructor):**
- Title: Y=0, full width, centred, `defaultLarge()` (12pt bold)
- Status text: Y=24, full width, centred, `defaultSmall()` (8pt)
- Progress bar: Y=38, X=16, width=displayWidth-32, height=16

**New configuring layout — MUST match boot board positions exactly:**
- Header: "Configuring..." at Y=0, full width, **centre-aligned**, `defaultLarge()`
- Status text: gate name + status at Y=24, full width, centred, `defaultSmall()`
  - Format: `"⟩ WiFi Station"` (or similar, using `>` character since Unicode is disabled)
  - Show the first **actionable** unsatisfied gate name
- Progress bar: Y=38, X=16, width=displayWidth-32, height=16 (SAME position as boot board)
  - Shows `(satisfied / total)` as percentage

**Key design principle:** The configuring board cycles through gates one-by-one. When a gate needs user action (WiFi wizard, etc.), the display module overlays the helper board. When the helper is dismissed, the configuring board shows the **next** unsatisfied gate. This continues until all gates are satisfied and the app transitions to RUNNING.

**Implementation:**
1. Remove the multi-row gate rendering code entirely
2. Remove the `GateEntry` struct and sorting logic
3. Remove all primitive icon drawing (drawLine, drawTriangle, drawCircle)
4. Use a simple single-line status like the boot board's `_statusLabel`
5. Reposition progress bar to match boot board exactly: Y=38, X=16, width=displayWidth-32, height=16
6. Header widget should be centre-aligned (alignment=1), not left-aligned

### FIX 3: Header Centre Alignment 🟡

Already covered in FIX 2 — change `_headerLabel.setAlignment(0)` to `_headerLabel.setAlignment(1)`.

### FIX 4: Add IConfigGate.hpp Extensibility Guide 🟡

**File:** `include/interfaces/IConfigGate.hpp`

Add a clear extensibility section to the doc comment explaining:
- Only gates registered via `registerConfigGate()` are mandatory
- Anything NOT registered is implicitly optional (app handles at runtime)
- The framework auto-registers `WifiConfigGate`
- Downstream apps add their own gates in main.cpp before `app.begin()`
- Use `getRegressionToleranceMs()` to control RUNNING→CONFIGURING regression
- Use `getDependencies()` to declare gate ordering
- Example code for a custom gate

## Line-Item Checklist

- [ ] Read `modules/u8g2Display/boards/bootBoard.cpp` constructor — note progress bar position (Y=38, X=16, width=dw-32, height=16)
- [ ] Read `modules/u8g2Display/u8g2DisplayModule.cpp` — find late catch-up block (~L218-230)
- [ ] Read `modules/u8g2Display/boards/configuringStatusBoard.cpp` — understand current layout
- [ ] Read `include/interfaces/IConfigGate.hpp` — note current doc comments
- [ ] FIX 1: Modify `u8g2DisplayModule.cpp` — fix late catch-up WiFi wizard routing
- [ ] FIX 2: Modify `configuringStatusBoard.cpp` — redesign to single-line + boot-aligned progress bar
- [ ] FIX 2: Modify `configuringStatusBoard.hpp` — update constructor params, remove unused members if needed
- [ ] FIX 3: Included in FIX 2 (centre alignment)
- [ ] FIX 4: Modify `IConfigGate.hpp` — add extensibility guide section
- [ ] Verify: `pio test -e native --filter "test_unit/test_u8g2Display"` — all tests pass
- [ ] Verify: `pio test -e native --filter "test_unit/test_application"` — all tests pass
- [ ] Verify: `pio run -e esp32s3nano` — hardware build succeeds

## Verification Requirement

After completing all items, run tests and hardware build. Then **STOP and send a message back to me** reporting:
1. What files were modified
2. Test results
3. Any design decisions made

Do NOT commit until I explicitly approve.
