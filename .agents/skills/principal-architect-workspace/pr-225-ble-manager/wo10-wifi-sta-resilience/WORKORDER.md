---
name: pr-225-ble-manager-wo10
description: Handoff instructions for Work Order 10 — fix issue #236 (WifiManager STA resilience, no unauthenticated fallback AP)
complexity: architect
---

# WO-10 — WifiManager STA resilience: fix issue #236

**Parent project:** `pr-225-ble-manager` (PR #235). The approval is recorded in `../PROJECT.md` → *Approval Record* (WO-10 entry, 2026-09-25).
**Issue:** [#236](https://github.com/matthewmcneill/esp32-app-foundation/issues/236). Read the issue body first (`gh issue view 236 -R matthewmcneill/esp32-app-foundation`).
**Branch:** `ble-manager/wo10-wifi-sta-resilience`, already checked out in this worktree. It runs in parallel with WO-8/WO-9, which only touch `modules/BLEManager/`.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions beyond those fixed below, do not create branches, do not push, and do not plan. Execute the checklist.

## Objective
A device that has Wi-Fi credentials must **never** end up permanently in AP mode, must never expose an **unauthenticated** portal, and must never switch Wi-Fi off, because of a transient STA outage. Keep the first-boot setup flow (no credentials) unchanged.

## Current behaviour (`modules/WifiManager/WifiManager.cpp` at `665b847`)
- **Idle-link recovery (~L331–334).** When the STA link is lost, it waits 10 s, then goes to `Init`, which makes one connect attempt.
- **Single attempt (~L263–266).** `StaConnecting` times out after 15 s → `WiFi.disconnect(true)` → `ApStarting`. `_connectionRetries` is reset but never used.
- **AP-only mode (~L270–296).** `ApStarting` uses `WiFi.mode(WIFI_AP)` (STA off) and an open `softAP()`. `Ready` with `_isAPMode` never retries STA, and after 1 h of inactivity it goes to `Shutdown`, which turns Wi-Fi off until reboot.
- **Auth bypass.** In `modules/Application/Application.cpp`, the `WIFI_AP_STARTED` handler (~L238–243) and the later `_authManager.setAPMode(_wifiManager.getAPMode())` both enable the AuthManager AP bypass (`AuthManager.cpp` ~L139: `if (_apMode) return true;`), even when credentials exist.
- **Power save.** L235 forces `WiFi.setSleep(WIFI_PS_NONE)`. ESP-IDF Wi-Fi/BT coexistence requires modem sleep while BT is enabled.

## Fixed design decisions (the PA's; implement as stated)
- **D-A: STA-first.** If `hasCredentials()`, a failed or timed-out STA attempt never switches to AP-only mode. Retry STA with backoff delays between attempts of **5 s, 10 s, 30 s, then every 60 s**, tracked with `_connectionRetries`. Keep the per-attempt timeout at 15 s.
- **D-B: fallback AP runs alongside STA.** If credentials exist but STA has been down continuously for **> 3 minutes** (consistent with `isWifiPersistentError()`), start the captive AP in **`WIFI_AP_STA`** mode, so STA keeps retrying on the same schedule. When STA reconnects, tear down the AP (stop DNS, `softAPdisconnect`), clear `_isAPMode`, and publish `WIFI_CONNECTED` as usual. A publish on AP stop is only needed if an event already exists for it; don't invent one.
- **D-C: no auth bypass once credentials exist.** The AuthManager AP bypass applies **only** to first-boot setup (AP mode **and** `!hasCredentials()`). Add `bool WifiManager::isSetupMode() const` (`_isAPMode && !hasCredentials()`) and use it in both places in `Application.cpp` instead of `getAPMode()`. The fallback AP from D-B keeps auth enforced. Its network may stay open; the portal behind it requires login.
- **D-D: Wi-Fi never turns itself off when credentials exist.** The 1-hour AP inactivity shutdown applies only in setup mode (`!hasCredentials()`). With credentials, keep STA retrying indefinitely; after 1 h of AP inactivity it's fine to drop just the fallback AP and keep STA retrying.
- **D-E: power save.** Use `WIFI_PS_MIN_MODEM` when BT is enabled (`#if CONFIG_BT_ENABLED` or `#ifdef WITH_BLE`; include `sdkconfig.h` if needed), and keep `WIFI_PS_NONE` otherwise.
- **D-F: user actions still work.** Portal credential updates (`_reconnectRequested`) keep working in all modes. The existing no-credentials path (`Init` → `ApStarting`) is unchanged.

## Strict Boundary Box
- **Only these files may change:**
  - `modules/WifiManager/WifiManager.cpp`, `modules/WifiManager/WifiManager.hpp`;
  - `modules/Application/Application.cpp` (**only** the two AuthManager AP-mode wiring sites);
  - `test/test_unit/test_wifiManager/*`;
  - `docs/developer/` (only to correct text that describes the old AP-fallback or auth-bypass behaviour, if any exists; keep edits minimal);
  - plus the Completion Report in this WORKORDER.
- Do NOT touch `modules/BLEManager/` (WO-8/WO-9 are editing it in parallel), AuthManager internals, other modules, portal JS, `platformio.ini` or sdkconfig.
- Do NOT push, tag, flash hardware, create branches, or comment on or close issues. The PA handles #236.

## Tooling Constraints
- The `platformio` MCP server (rule 04) is **not available in this harness**. Use `~/.platformio/penv/bin/pio` for **build and test only**. **No flashing.**
- Commands, run from the worktree root:
  - `~/.platformio/penv/bin/pio run -e esp32s3nano_ble`
  - `~/.platformio/penv/bin/pio run -e esp32s3nano`
  - `~/.platformio/penv/bin/pio test -e native -f test_wifiManager`
  - full `~/.platformio/penv/bin/pio test -e native`
- Temporary files go in `tmp/` at the worktree root.

## Line-Item Instructions
- [ ] `WifiManager.hpp`/`.cpp`: implement D-A to D-F in the existing `WiFiState` state machine. Add states only if they're clearly needed, and document any new state in the header comment block. Keep all `WiFi.*` calls on the main loop (`tick()`), as today.
- [ ] `WifiManager.hpp`: add `isSetupMode()`, with a doc comment explaining the security rationale (#236).
- [ ] `Application.cpp`: replace both AuthManager AP-mode wiring sites so the bypass follows `isSetupMode()`. Because the fallback AP can start and stop at runtime, make sure the bypass is re-evaluated on the relevant events, i.e. it is never left `true` after credentials exist. The simplest correct approach is to set `_authManager.setAPMode(_wifiManager.isSetupMode())` in the `WIFI_AP_STARTED` handler and also on `WIFI_CONNECTED`.
- [ ] `test/test_unit/test_wifiManager/`: add native tests to the extent the existing stubs allow:
  - with credentials, repeated STA timeouts **don't** enter AP-only mode and follow the backoff schedule;
  - after > 3 min down, the fallback AP runs in AP+STA mode;
  - `isSetupMode()` is false whenever credentials exist.

  If the Arduino WiFi stub can't express something, say so in the report rather than faking it.

## Verification Requirement
1. Both firmware envs build (`esp32s3nano_ble`, `esp32s3nano`) with no new warnings in the files you touched.
2. `pio test -e native -f test_wifiManager` passes. The full native run shows only the pre-existing `test_lvgl_stubs` failure.
3. Self-review against D-A to D-F. Walk through three scenarios in the report:
   - router reboot for 60 s: STA recovers, and no AP ever appears;
   - router off for 10 min: the fallback AP appears at about 3 min in AP+STA, auth is enforced, and when the router returns STA reconnects and the AP is torn down;
   - first boot with no credentials: setup AP and bypass, unchanged.
4. **Commit** on `ble-manager/wo10-wifi-sta-resilience` (`fix(wifi): … (#236)`), ending with:
   ```
   Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
   ```
   **Do not push.**

## Issue Surfacing
Out-of-boundary findings go under *Out-of-Scope Findings* in your Completion Report. Do not fix them and do not file issues.

---
## Completion Report
<!-- Written by the worker at task completion; append below using the template. -->
