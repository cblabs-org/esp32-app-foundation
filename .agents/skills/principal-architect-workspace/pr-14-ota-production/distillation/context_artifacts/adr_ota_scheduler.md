# ADR: OTA Scheduler — tick() design

## Context

`_autoCheck` was a persisted config field that was surfaced in the portal UI but had no backing implementation — no call site fired `performUpdate()` at boot, and no periodic trigger existed for the daily "quiet hour" check.

## Decision: Use ITickable + tick(), not a FreeRTOS timer

**Rejected alternative: FreeRTOS `xTimerCreate`**
- A one-shot or repeating FreeRTOS software timer would require a timer callback. Timer callbacks run in the RTOS timer task context (priority 2), which has only 4KB stack. The OTA flow involves TLS handshakes and large heap allocations — the timer callback itself would need to call `performUpdate()`, which spawns its own task anyway. This adds a layer without benefit.
- Software timers also require cleanup on state changes (e.g. WiFi disconnect). Managing timer lifetime adds complexity.

**Chosen: ITickable with tick() ≤10ms constraint respected**
- `tick()` does nothing heavy: it checks two booleans and calls `time()` + `localtime_r()` (all ≤1ms).
- `performUpdate()` is called only when conditions are met; it immediately spawns a FreeRTOS task and returns.
- No timer lifecycle to manage.
- Consistent with how WifiManager, TimeManager and Display handle periodic work.

## Boot Check vs Daily Check

Two distinct triggers implemented in `tick()`:

**Boot check (`_hasBootChecked` flag):**
- Fires exactly once on the first tick after the Application enters Running state.
- Running state is only entered when all gates pass — which requires WiFi to be connected.
- Therefore WiFi is guaranteed at the point the boot check fires.
- Does NOT respect `_checkHour` — boot is a special case, not a "quiet hour" event.

**Daily scheduled check (unix-day comparison):**
- `_lastCheckDay = (uint32_t)(time(nullptr) / 86400)` gives today's day number.
- Fires when `tm_hour == _checkHour && today > _lastCheckDay`.
- Guards: `time() < 1_000_000` → NTP not yet synced → skip silently.
- `_lastCheckDay` is NOT persisted (reset to 0 each boot). This means the daily check can fire on the same calendar day as the boot check if the boot happens before `_checkHour`. This is acceptable — two checks per day in a rare edge case is harmless.

## ITickable Registration

`OTAUpdateManager` added to `_tickables` in `Application::begin()` **after** Display. Boot order: WiFi → Time → Display → OTA. OTA is lowest priority tickable, which is correct — display and connectivity must be stable first.
