# Graveyard: ota_get_status last_check_ms Bug

## The Bug

`ota_get_status` returned `last_check_ms: 0` even after the boot OTA check completed successfully.

## Root Cause

Two separate code paths perform OTA checks, and only one tracked timing:

1. **Daily scheduled check** (`tick()` → `checkForUpdate()`) — set `_lastCheckDay` (Unix day counter)
2. **Boot check** (`tick()` → `performUpdate()` → `performUpdateTask()`) — did its own inline GitHub query, never called `checkForUpdate()`, never set any timing field

The WO2 worker's MCP tool tried to convert `_lastCheckDay` (a Unix day counter) back to milliseconds (`day * 86400 * 1000`), which would produce a Unix timestamp, not elapsed time. And since boot checks bypassed `checkForUpdate()`, `_lastCheckDay` stayed 0.

## The Fix

1. Added `uint32_t _lastCheckMillis = 0` member to `OTAUpdateManager`
2. Set `_lastCheckMillis = millis()` in both `checkForUpdate()` (line 285) and `performUpdateTask()` (line 470)
3. Changed MCP tool to report `millis() - _lastCheckMillis` (elapsed ms since last check) or `-1` if no check has occurred

## Lesson

When a module has multiple code paths that perform the same logical operation (check for update), any instrumentation must cover ALL paths. The `performUpdateTask()` method duplicates the GitHub query logic from `checkForUpdate()` because it runs in a FreeRTOS task context and needs to continue into the download/flash pipeline if an update is found.
