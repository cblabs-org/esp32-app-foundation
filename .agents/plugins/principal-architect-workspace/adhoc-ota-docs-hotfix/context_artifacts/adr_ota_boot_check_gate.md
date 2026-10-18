# ADR: OTA Boot Check — Wi-Fi Gate + Silent Check

## Context

`OTAUpdateManager::tick()` fires `performUpdate()` on the first call after
entering Running state. Running state entry implies Wi-Fi is associated, but
the TLS stack and DNS resolver need additional seconds to settle. The boot
check was launching a FreeRTOS task that immediately hit `api.github.com`
via TLS, which consistently failed with a network error and emitted a
"Firmware Update Failed" modal to the display board.

## Decision 1 — Wi-Fi Connectivity Gate

`tick()` now returns early if `WiFi.status() != WL_CONNECTED` (guarded by
`#ifndef NATIVE_BUILD`). `_hasBootChecked` is only set to `true` once the
gate is passed, so the boot check retries silently every tick until the
network is genuinely ready. This is the **primary fix**.

**Why not a delay?** A fixed `vTaskDelay` in tick() would couple the OTA
module to an arbitrary magic number. The `WiFi.status()` check is a zero-cost
polling gate that self-resolves exactly when the network is ready.

## Decision 2 — `_silentCheck` Flag (Defence in Depth)

A `bool _silentCheck` member was added to `OTAUpdateManager`. Auto-check
paths (boot, daily scheduler) set `_silentCheck = true` before calling
`performUpdate()`. The `checkGithub` API handler sets it `false`.

In `performUpdateTask()`, if the GitHub query fails and `_silentCheck` is
`true`, the state returns to `Idle` with no `publishProgress` call — zero
UI impact. If `false`, the error modal is shown as normal.

**Why keep `_silentCheck` given the WiFi gate?** The gate ensures Wi-Fi is
associated, but the first TLS handshake to `api.github.com` can still fail
on networks with aggressive DNS TTLs or when the AP's uplink is momentarily
busy. `_silentCheck` prevents a rare but real race from showing an error
modal that would confuse the user.

## Rejected Alternatives

- **Delay in `begin()` or `tick()`** — Magic number, not self-correcting.
- **Retry loop inside `performUpdateTask()`** — Blocks the FreeRTOS task
  for minutes on a bad network, consumes stack, and prevents the task from
  returning cleanly.
- **Event-driven trigger from WiFiManager** — Correct architectural design
  but requires adding an observer interface to WiFiManager. Deferred as
  over-engineering for a background check.
