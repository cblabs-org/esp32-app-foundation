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

### Status
complete

Code commit: `eae1f9cc` on `ble-manager/wo10-wifi-sta-resilience` (not pushed). Provenance checked: this WORKORDER names PR #235 / `pr-225-ble-manager`, and `PROJECT.md` → Approval Record has the human's WO-10 approval ("Fix it fully now. and close #236 out").

**Verification**
- `pio run -e esp32s3nano_ble`: SUCCESS (RAM 18.7%, Flash 39.0%). `pio run -e esp32s3nano`: SUCCESS (RAM 17.7%, Flash 31.8%). No warnings from `WifiManager.*` or `Application.cpp` in either log.
- `pio test -e native -f "*test_wifiManager"`: 32/32 passed (6 new). Note: `-f test_wifiManager` with no glob matches nothing ("0 test cases") because the test path is `test_unit/test_wifiManager`.
- Full `pio test -e native`: 939/940 passed; the only failure is the pre-existing `test_lvgl_stubs` ERRORED.

**Scenario walk-throughs** (times measured from the moment the link drops)
1. *Router reboot, 60 s.* `Ready` sees the link drop → `WIFI_DISCONNECTED` and the downtime tracker starts. If the driver re-associates within the 10 s grace, `onStaConnected()` publishes `WIFI_CONNECTED` (HA MQTT reconnects). Otherwise attempt 1 runs at t=10 s and times out at 25 s → 5 s backoff → attempt 2 runs 30–45 s → 10 s backoff → attempt 3 starts at 55 s, and the router is back at 60 s, so it connects inside that 15 s window (worst case, attempt 4 at 100 s). Any background re-association during a backoff is also picked up. `shouldStartFallbackAp()` needs > 180 s of downtime, so **no AP ever appears**, `_isAPMode` stays false and the auth bypass stays false.
2. *Router off, 10 min.* Attempts run at 10, 30, 55, 100 and 175 s (timeouts at 25/45/70/115/190 s). In `StaBackoff` at ≈190 s, downtime > 180 s, so `startAccessPoint()` runs `WiFi.mode(WIFI_AP_STA)` (the setup AP uses `WIFI_AP`), brings up the open soft-AP plus DNS, and publishes `WIFI_AP_STARTED`. Application runs `setAPMode(isSetupMode())`, which is **false** because credentials exist, so **portal auth stays enforced**. `isReady()` is true (AP up), so the portal is served even when STA has not connected since boot. STA keeps retrying every 60 s in AP+STA mode. When the router returns, the next attempt (or a background re-association) calls `onStaConnected()`: `stopAccessPoint()` stops DNS, `softAPdisconnect(true)` switches AP_STA to STA with the STA link kept, `_isAPMode=false`, the retry/suppression state resets, and **only then** is `WIFI_CONNECTED` published (Application re-evaluates the bypass to false). If the AP had been idle for 1 h, only the AP is dropped (`_fallbackApSuppressed`) and STA keeps retrying; Wi-Fi is never switched off.
3. *First boot, no credentials.* `Init` (2 s) → `ApStarting` → `startAccessPoint()` with `WIFI_AP`, then `WIFI_AP_STARTED` → `isSetupMode()` is true → bypass enabled. Then `Ready`, and after 1 h idle → `Shutdown` (Wi-Fi off). Unchanged. If credentials arrive via `updateWiFi()`, `isSetupMode()` becomes false at once, the deferred reconnect tears the AP down, and STA-first applies. Covered by the native tests.

### Changes Made
- `modules/WifiManager/WifiManager.hpp`: new `StaBackoff` state (documented in the enum and the file header policy block); named timing constants; `_fallbackApSuppressed`; `isSetupMode()` with a doc comment on the #236 security rationale; pure policy helpers `staRetryDelayMs()` and `shouldStartFallbackAp()`; `isReady()` is now also true while an AP is up; `getAPMode()` doc clarified; private helper declarations; `transitionTo()` takes `now`.
- `modules/WifiManager/WifiManager.cpp`: state machine rewritten around the helpers `startStaAttempt`, `onStaConnected`, `startAccessPoint`, `stopAccessPoint`, `applyPowerSave`, `markStaDown` and `staDownMs`.
  - D-A: timeout → `_connectionRetries++` → `StaBackoff` (5/10/30/60 s), never AP-only.
  - D-B: fallback AP in `WIFI_AP_STA` from `StaBackoff` after > 3 min; torn down on reconnect.
  - D-D: the 1 h inactivity check drops only the fallback AP; `Shutdown` is reachable only from setup mode.
  - D-E: `WIFI_PS_MIN_MODEM` when `WITH_BLE` or `CONFIG_BT_ENABLED` (not on native); `WIFI_PS_NONE` otherwise.
  - D-F: `_reconnectRequested` and DNS are handled at the top of `tick()` in every state; `Init → ApStarting` without credentials is unchanged.
  - Also: `isWifiPersistentError()` now masks only setup mode, not the fallback AP. The MCP `wifi_scan_networks` tool reverts to `WIFI_AP` only in setup mode, because on the fallback AP that revert would have killed STA.
- `modules/Application/Application.cpp` (AuthManager wiring sites only): the `WIFI_AP_STARTED` handler now uses `isSetupMode()`; a `WIFI_CONNECTED` subscription re-evaluates `setAPMode(isSetupMode())`; the boot-time `setAPMode` uses `isSetupMode()`.
- `test/test_unit/test_wifiManager/test_wifiManager.cpp`: 6 tests covering the backoff schedule, fallback thresholds and preconditions, the cumulative timeline (fallback after about 3 min, ≥ 4 attempts first), setup AP plus `WIFI_AP_STARTED`, credentials ending setup mode (and `WIFI_CONNECTED` published once), and credentials never entering AP or setup mode over a simulated 10 min.
- `docs/developer/security.md`: the "AP Mode Bypass" section now describes the setup-only bypass and the fallback AP with auth enforced.

### Decisions Made
- **One new state (`StaBackoff`).** The fallback AP is raised only from it, between attempts, so the radio mode never changes mid-association. With the schedule this puts it at ≈ 3 min 10 s after link loss.
- **`WiFi.disconnect(false)` on attempt timeout** (was `disconnect(true)`). It aborts the association without disabling the STA interface, so a running fallback AP is untouched and the radio isn't cycled off/on every minute. The next attempt re-applies the mode.
- **The downtime tracker starts at the first attempt**, not only at link loss. A device that boots while its router is off therefore also gets the fallback AP (and `isWifiPersistentError()`) after 3 min.
- **Pairing `WIFI_CONNECTED` after a re-association within the grace period.** Previously only `WIFI_DISCONNECTED` fired, which left HomeAssistant MQTT disconnected (`HomeAssistantModule::onWifiDisconnected` → `disconnectMqtt()`). This uses an existing event, not a new one.
- **`isReady()` includes `_isAPMode`.** `WebServerManager::tick()` starts the HTTP server on the first `isReady()`. Without this, a device that booted with its router off would never serve the portal on the fallback AP.
- **Setup AP plus credentials applied at runtime** (for example a config import): `Ready` sets `_reconnectRequested`, so STA-first takes over rather than sitting in AP-only mode with credentials.
- **Power-save gate: `!UNIT_TEST && (WITH_BLE || CONFIG_BT_ENABLED)`.** `sdkconfig.defaults` sets `CONFIG_BT_ENABLED=n` and only `sdkconfig.ble.defaults` sets it to `y`, so non-BLE envs keep `WIFI_PS_NONE`. Native is excluded because its WiFi stub has no `WIFI_PS_MIN_MODEM`.
- **No event on AP stop.** None exists, and none was invented.

### Dead Ends & Warnings
- **Native stub limits.** `test/stubs/platform/WiFi.h` always returns `WL_CONNECTED` and `WiFi.mode()` is a no-op with no getter, and the stub is outside my boundary. So a real STA failure can't be driven through `tick()`, and the AP_STA radio mode can't be observed natively. The failure-path decisions were factored into the pure helpers that `tick()` calls, and those are what is tested. The AP+STA mode choice and the teardown are verified by code review only.
- **Hardware not exercised** (flashing is not allowed). The issue's hardware check is still outstanding: drop the AP for 60 s and for more than 3 min on `esp32s3nano_ble`, with BLE churn, and confirm that the "Should enable WiFi modem sleep" log line is gone.
- **Asynchronous event dispatch on target.** EventBus dispatch is asynchronous there, so the bypass re-evaluation on `WIFI_CONNECTED` / `WIFI_AP_STARTED` lags by a loop iteration. The only case where the bypass is still true once credentials exist is setup mode → `updateWiFi()` → STA up, until the queued `WIFI_CONNECTED` is dispatched. The setup AP is already torn down at that point.

### Out-of-Scope Findings
- **`docs/design_specification.md` is stale.** L1563 and L1743 still say `wifi.ap_started` enables the AuthManager bypass unconditionally. This file is outside `docs/developer/`.
- **`docs/developer/bluetooth.md` §6 (Coexistence)** should mention modem sleep (`WIFI_PS_MIN_MODEM`) on BLE builds, as #236 suggests. That isn't old AP-fallback or auth-bypass text, so I left it.
- **`testConnection()`** runs blocking `WiFi.*` calls (`disconnect(true)`, `begin`, `delay` polling) from the HTTP handler thread. This is pre-existing and not main-loop confined. On the fallback AP it can briefly flip the radio mode.
- **`WifiPreconditionContributor`** reports `FallbackEngaged` for any AP mode, and its describe text for that cause might want wording that fits the fallback AP (STA still retrying). No change was needed for correctness.

### Blockers
None.
