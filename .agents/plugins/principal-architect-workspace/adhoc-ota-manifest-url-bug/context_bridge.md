---
title: OTA "No manifest URL configured" — Debug Investigation & Latent Defect Discovery
distilled_at: 2026-06-23T15:40:00Z
original_plan_id: standalone
artifacts: []
---

# Context Bridge — OTA "No manifest URL configured" Investigation

## Executive Summary

Systematic debugging investigation into a persistent `"No manifest URL configured"` error
reported by `ota_get_status` after a clean flash of `v0.0.12-dev.24`. The error was
**not reproducible** on clean builds from committed code — root cause was a dirty working
tree build from a previous agent debugging session that corrupted the `Application` class
memory layout (extra debug fields shifted `_config` member offsets). Two latent code
defects were discovered during the investigation and are documented below for future
remediation.

## Evidence Collected

| Probe | Method | Result |
|-------|--------|--------|
| Pointer chain hex dump | Debug firmware with MCP `ota_get_status` extension | `otaManifestUrl: 0x3f4001d4` → valid `.rodata` string literal |
| Clean committed build | `git stash` + `pio run -t clean` + flash `e5c6a18` | `state: Idle`, no error, boot check succeeded |
| Debug instrumented build | Same code + MCP debug output | `state: Idle`, no error, URL correctly propagated |
| Persisted config dump | `get_config` MCP tool | `learnedManifestUrl` correctly stored in `/mod/ota.json` |
| Original failing firmware | Pre-session `v0.0.12-dev.24` built at 11:13:10 | `state: Error`, `last_error: "No manifest URL configured"` |

## Root Cause: Dirty Working Tree Build

The firmware that exhibited the error was built from a dirty working tree during a
previous debugging session. That session added `const char*` debug fields
(`DEBUG_url_in_main`, `DEBUG_url_in_begin`, `DEBUG_url_after_copy`) to
`Application.hpp` in the public section. These extra members shifted the memory layout
of the `Application` class, causing `OTAUpdateManager::_config` (which stores
`&app.getConfig()`) to read `FrameworkConfig` fields at incorrect offsets.

Specifically, the 12 extra bytes (3 × `const char*`) inserted before `_config` in
the class layout caused `otaManifestUrl` at offset 4 within `FrameworkConfig` to
read zeroed memory instead of the actual `.rodata` pointer.

**No code fix is needed** — the committed source at `e5c6a18` is correct.

## Latent Defects Discovered

### Defect 1: `_silentCheck` Gap in `performUpdateTask()` URL Guard

**File:** `modules/OTAUpdateManager/OTAUpdateManager.cpp` — lines 430-441

**Problem:** The boot auto-check sets `_silentCheck = true` so errors are suppressed.
However, the `otaManifestUrl == nullptr` guard exits via `return` **before** the
`_silentCheck` cleanup code (lines 451-458). If the URL is legitimately null (e.g.,
a downstream app that doesn't configure OTA), the boot check permanently sets
`state: Error` and `last_error: "No manifest URL configured"` — the silent cleanup
never fires.

**Impact:** Low (only affects apps without `otaManifestUrl`), but violates the
documented `_silentCheck` contract from `adhoc-ota-docs-hotfix` (`380e96c`).

**Fix:** Add `_silentCheck` handling to the URL guard:
```cpp
if (cfg.otaManifestUrl == nullptr && learnedManifestUrl_[0] == '\0') {
    if (_silentCheck) {
        _state = OTAState::Idle;
        _silentCheck = false;
        return;
    }
    // ... existing error path ...
}
```

### Defect 2: Redundant/Overly-Strict URL Guard

**File:** `modules/OTAUpdateManager/OTAUpdateManager.cpp` — lines 435, 290

**Problem:** `performUpdateTask()` and `checkForUpdate()` both guard on
`cfg.otaManifestUrl == nullptr` and abort the entire pipeline. However,
`fetchAndVerifyManifest_()` (lines 818-833) builds a URL list from **two** sources:
1. `learnedManifestUrl_` (persisted, populated after first successful fetch)
2. `cfg.otaManifestUrl` (compiled-in from `FrameworkConfig`)

It only fails if **both** are empty. The guard in `performUpdateTask` fires before
`fetchAndVerifyManifest_` gets a chance to check the learned URL.

**Impact:** OTA is blocked even when a valid `learnedManifestUrl_` exists —
the compiled-in URL is treated as mandatory despite the URL resilience design
from PR-36/PR-37.

**Fix:** Remove the `cfg.otaManifestUrl == nullptr` guard from both
`performUpdateTask()` and `checkForUpdate()`. Let `fetchAndVerifyManifest_()`
handle the "no URLs available" case, since it already checks both sources
and produces the same error message.

## Key Architectural Facts

### FrameworkConfig Pointer Chain (Verified Sound)
```
main.cpp: config.otaManifestUrl = "https://..." (string literal → .rodata, immortal)
     ↓ const ref
Application::begin(): _config = config (shallow copy of 3 const char* pointers)
     ↓ &app.getConfig()
OTAUpdateManager::begin(): _config = &app.getConfig() (pointer to Application::_config)
     ↓ *_config
performUpdateTask(): const FrameworkConfig& cfg = *_config
```

All lifetimes are valid: string literal in `.rodata` (immortal), `Application` is
`static` in `main.cpp` (immortal), pointer chain is stable (single assignment in
`begin()`, never reassigned).

### FrameworkConfig Memory Layout (ESP32, 32-bit)
```
Offset 0:  otaPublicKey   (const char*, 4 bytes)
Offset 4:  otaManifestUrl (const char*, 4 bytes)
Offset 8:  otaToken       (const char*, 4 bytes)
Total: 12 bytes
```

### Serial Monitor Limitation
The USB serial port (`/dev/cu.usbserial-5AA70033491`) captures only bootloader ROM
output at 74880 baud. Application-level logs at 115200 baud were not captured due to
the PlatformIO MCP server defaulting to `/dev/cu.debug-console` for upload. The MCP
HTTP telemetry endpoint was used as the primary evidence source instead.

## Next Steps

1. **Fix Defect 1 + 2** — Can be combined into a single commit. Remove the
   redundant URL guard and let `fetchAndVerifyManifest_` handle the null-URL case
   with proper `_silentCheck` support.
2. **Remove debug instrumentation** — Clean up ESP_LOGW/LOGE "DEBUG" lines in
   `main.cpp` and `OTAUpdateManager.cpp`, remove `DEBUG_url_in_ota_begin` member
   from `OTAUpdateManager.hpp`.
3. **Consider adding `_silentCheck` handling to ALL early-exit guards** in
   `performUpdateTask()` (e.g., the `_updatesEnabled` guard at line 422) for
   consistency.
