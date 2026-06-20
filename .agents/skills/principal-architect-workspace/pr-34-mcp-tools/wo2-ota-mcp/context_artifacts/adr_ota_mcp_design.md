# ADR: OTA MCP Tool Design Decisions

## Context

WO2 required exposing three OTA domain tools via IMCPContributor on OTAUpdateManager.

## Decisions

### 1. `ota_check_now` calls `checkForUpdate()` directly (blocking HTTPS)

`checkForUpdate()` makes a blocking HTTPS call to the GitHub API. This is safe from the MCP handler context because PsychicHttp dispatches requests on dedicated connection tasks with generous timeouts. However, this method must NEVER be called from `tick()`. The existing code already documents this constraint.

**Alternative considered:** Launching checkForUpdate in a FreeRTOS task and returning immediately. Rejected because the caller (MCP agent) needs the synchronous `update_available` result to make decisions. The existing `performUpdate()` path already uses a FreeRTOS task for the heavy download work.

### 2. `ota_trigger_update` accepts but does not consume the `version` parameter

The work order specified accepting an optional `version` string. The current OTA pipeline always targets the latest release for the configured channel — there is no version-pinned download path. The parameter is accepted and documented as "reserved for future use" to maintain API forward-compatibility without breaking existing callers when version pinning is eventually implemented.

### 3. `last_check_ms` derived from `_lastCheckDay`

The OTAUpdateManager tracks the last check time as a Unix day counter (`_lastCheckDay = time()/86400`), not as a millisecond timestamp. The MCP tool multiplies this back to approximate milliseconds. This means the value has day-level granularity, not sub-second. This is sufficient for the MCP use case (checking when the last update check occurred).

### 4. Error state auto-reset on `ota_check_now` and `ota_trigger_update`

Both mutating tools auto-reset `_state` from `Error` back to `Idle` before proceeding. This mirrors the existing web API behavior (line 1278–1281 in the POST /api/ota handler). Without this, a transient error would permanently lock the OTA pipeline until device reboot.

### 5. No graveyard needed

Implementation succeeded on first attempt with zero compilation errors. No dead-ends or failed approaches to document.
