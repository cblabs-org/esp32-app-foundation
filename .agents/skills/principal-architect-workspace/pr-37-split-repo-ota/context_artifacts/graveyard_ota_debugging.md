# OTA Debugging Graveyard — Failed Approaches & Red Herrings

## 1. Serial Port Not Capturing (DTR/RTS Stuck)

**Symptom:** PIO MCP monitor returned 0-byte log files. Direct `cat` on serial
port also returned 0 bytes.

**Red herring:** Assumed wrong baud rate or wrong port. Tried multiple approaches:
`stty 115200 raw`, `strings` on binary log, `xxd` hex dump.

**Actual cause:** CH340 USB-serial chip had DTR/RTS state stuck from a previous
monitor session being killed uncleanly. The ESP32 was running and producing logs,
but the CH340 wasn't passing data through until DTR was toggled.

**Fix:** User physically toggled DTR state (opened a fresh serial terminal).

## 2. `checkForUpdate()` Stub Confusion

**Symptom:** User triggered OTA check from portal, device reported "up to date".

**Red herring:** Investigated SemVer parsing, version comparison math, manifest
content, CDN caching.

**Actual cause:** The portal "Check for updates" button calls `checkForUpdate()`
which is a **stub** that always returns false (line 302: "use performUpdate() for
manifest-first check"). The real pipeline is `performUpdate()` / `ota_trigger_update`.

## 3. `_silentCheck` Error Swallowing

**Symptom:** Boot auto-check runs, state transitions from Checking → Idle with no
error, no download, no update.

**Context:** Boot checks set `_silentCheck = true` to suppress error modals on the
display. When `fetchAndVerifyManifest_()` fails during a silent check, the error
is swallowed: state resets to Idle and `_lastOtaError` is set to nullptr.

**Impact:** Made debugging much harder — the MCP status endpoint showed no error,
so the failure appeared as "no update available" rather than a fetch error.

## 4. GitHub CDN Cache Lag

**Symptom:** `raw.githubusercontent.com` served stale manifest (dev.7) for several
minutes after the deploy script pushed dev.8 to the releases repo.

**Root cause:** GitHub's CDN caches `raw.githubusercontent.com` responses for up to
5 minutes. The first boot check after manifest push fetches the stale version.

**Workaround:** Wait 5+ minutes after deploy, or use the boot auto-check which
fires after Wi-Fi connects (~15s after boot) — by which time the CDN has usually
propagated.

## 5. Device Fetching Old Manifest URL

**Symptom:** Rapid status polling showed Checking → Idle in 6 seconds. No error,
no download. Serial logs revealed device was fetching from `manifest-latest`
release URL, not `raw.githubusercontent.com`.

**Root cause:** The running firmware (dev.7) was built BEFORE WO1 changed
`main.cpp` to use the new URL. The compiled-in manifest URL was still the old
`/releases/download/manifest-latest/manifest.json`. The learned URL was also the
old one.

**Fix:** Updated the old `manifest-latest` release to point at dev.8 (bridge
strategy). After OTA to dev.8, the device uses the new URL for all future checks.

## 6. `gh` CLI Auth Failure

**Symptom:** `gh release delete manifest-latest --repo ...` returned 401 Unauthorized.

**Root cause:** `gh` CLI was not authenticated for the releases repo. The
`GITHUB_TOKEN` env var had an expired token.

**Fix:** User ran the command manually with their own auth.
