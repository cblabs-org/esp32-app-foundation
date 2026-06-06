---
name: pr-14-wo4-release-channels
description: Handoff instructions for Work Order 4 — Release Channels
---

# WO4 — Release Channels

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Extend the OTA pipeline with release channel support (`stable`/`beta`/`dev`). The device's OTA channel determines which GitHub releases it considers:
- `stable` → only non-prerelease `/releases/latest`
- `beta`/`dev` → scans `/releases` and matches releases tagged with the channel's pre-release suffix

Also implement implicit downgrade: when the channel changes from `beta` to `stable`, allow the device to "downgrade" to a lower-version stable release.

**Reduced scope**: OTAUpdateManager already implements `IConfigurable` (PR-16) with `_channel[16]` at schema v1. This WO extends the config to v2, adds `lastCheckedChannel`, and wires channel-aware GitHub queries.

## Strict Boundary Box

- ✅ You MAY modify `lib/GithubClient/GithubClient.hpp` and `.cpp` (add pre-release query method)
- ✅ You MAY modify `modules/OTAUpdateManager/OTAUpdateManager.hpp` and `.cpp` (channel logic, schema v2)
- ✅ You MAY create test files in `test/test_unit/`
- ❌ Do NOT modify `modules/Application/` — completed in WO2
- ❌ Do NOT modify `lib/OTADownloadEngine/` — completed in WO2
- ❌ Do NOT modify `lib/OTAManifest/` — completed in WO3
- ❌ Do NOT modify `modules/ConfigManager/` — that's WO5
- ❌ Do NOT modify `web/portal/` — that's WO6
- ❌ Do NOT modify `sdkconfig.defaults`
- ❌ Do NOT create branches or manage git

## Project House Style

- PascalCase for files/types, standard license header
- Doxygen comments on all public methods
- `#pragma once`, `#ifndef NATIVE_BUILD` guards
- Prefer `char[]` over Arduino `String` for new fields (assessment flagged `String` fragmentation risk in GitHub/OTA flow)

## Context: Current State

**OTAUpdateManager** already has:
- `_channel[16] = "stable"` — IConfigurable field, loaded from `/mod/ota.json`
- `_autoCheck = true` — IConfigurable field
- `applyConfig()` / `serializeConfig()` — reads/writes `channel` and `autoCheck`
- `getSchemaVersion()` returns 1
- `migrateConfig()` — no-op at v1
- `checkForUpdate()` uses `GithubClient::getLatestRelease()` (stable-only)

**GithubClient** currently has:
- `getLatestRelease()` — queries `/repos/{owner}/{repo}/releases/latest` (only non-prerelease)
- Stores `releaseId` (tag name), `releaseAssets`, `releaseAssetURL[]`, `releaseAssetName[]`
- No way to query pre-releases

## Implementation Checklist

### 1. Add `getReleases()` to GithubClient

- [ ] In `GithubClient.hpp`, add:
  ```cpp
  bool isPreRelease;  // Set by the release query methods
  ```
  as a public member alongside `releaseId`, `releaseAssets`, etc.
  
- [ ] Add a new method declaration:
  ```cpp
  /**
   * @brief Query all releases (including pre-releases) and find the latest
   *        release matching the specified channel.
   * @param channel Channel to filter by ("stable", "beta", "dev")
   * @return true if a matching release was found
   */
  bool getLatestReleaseForChannel(const char* channel);
  ```

- [ ] In `GithubClient.cpp`, implement `getLatestReleaseForChannel()`:
  - If `channel` is `"stable"` (or empty/null), delegate to existing `getLatestRelease()` and set `isPreRelease = false`
  - If `channel` is `"beta"` or `"dev"`:
    - Query `/repos/{owner}/{repo}/releases` (returns up to 30 releases)
    - Parse the JSON array response
    - For each release, check:
      - `prerelease == true` (it IS a pre-release)
      - The tag name contains the channel string (e.g., tag `v3.1.0-beta.1` contains `"beta"`)
    - Take the **first matching** pre-release (GitHub returns newest first)
    - Populate `releaseId`, `releaseAssets`, `releaseAssetURL[]`, `releaseAssetName[]` from that release
    - Set `isPreRelease = true`
    - Return `true` if found, `false` if no matching pre-release exists
  - The API endpoint is: `GET /repos/{owner}/{repo}/releases`
  - Use the same HTTPS/auth pattern as `getLatestRelease()`

### 2. Extend OTAUpdateManager IConfigurable to schema v2

- [ ] In `OTAUpdateManager.hpp`, add a new private member:
  ```cpp
  char _lastCheckedChannel[16] = "";  ///< Last channel used for update check
  ```

- [ ] In `OTAUpdateManager.hpp`, update `getSchemaVersion()` inline or in .cpp to return `2`

- [ ] In `OTAUpdateManager.cpp`, update `applyConfig()` to also read `lastCheckedChannel`:
  ```cpp
  const char* lcc = section["lastCheckedChannel"] | "";
  strlcpy(_lastCheckedChannel, lcc, sizeof(_lastCheckedChannel));
  ```

- [ ] In `OTAUpdateManager.cpp`, update `serializeConfig()` to also write `lastCheckedChannel`:
  ```cpp
  section["lastCheckedChannel"] = _lastCheckedChannel;
  ```

- [ ] In `OTAUpdateManager.cpp`, update `migrateConfig()` for v1→v2:
  ```cpp
  bool OTAUpdateManager::migrateConfig(JsonObject& section, int fromVersion) {
      if (fromVersion < 2) {
          section["lastCheckedChannel"] = "";
          ESP_LOGI(TAG, "Migrated OTA config v%d → v2: added lastCheckedChannel", fromVersion);
      }
      return true;
  }
  ```

- [ ] In `resetConfig()`, also reset `_lastCheckedChannel`:
  ```cpp
  _lastCheckedChannel[0] = '\0';
  ```

### 3. Wire channel-aware update checks in OTAUpdateManager

- [ ] In `checkForUpdate()`, replace:
  ```cpp
  GithubClient github(cfg.otaGithubRepo, ...);
  bool success = github.getLatestRelease();
  ```
  with:
  ```cpp
  GithubClient github(cfg.otaGithubRepo, ...);
  bool success = github.getLatestReleaseForChannel(_channel);
  ```

- [ ] In `performUpdateTask()`, similarly replace `github.getLatestRelease()` with `github.getLatestReleaseForChannel(_channel)`

- [ ] **Implicit downgrade logic**: In `checkForUpdate()`, after the SemVer comparison:
  ```cpp
  // Normal case: remote > local → update available
  if (remote > local) {
      state_ = OTAState::Idle;
      return true;
  }
  
  // Implicit downgrade: channel changed since last check
  // e.g., switched from beta to stable — allow "downgrade" to the latest stable
  if (strcmp(_channel, _lastCheckedChannel) != 0 && _lastCheckedChannel[0] != '\0') {
      ESP_LOGI(TAG, "Channel changed ('%s' → '%s') — allowing implicit version change",
               _lastCheckedChannel, _channel);
      strlcpy(_lastCheckedChannel, _channel, sizeof(_lastCheckedChannel));
      // Save the channel change to config
      if (app_ != nullptr) {
          app_->getConfigManager().saveModuleConfig(this);
      }
      state_ = OTAState::Idle;
      return true;  // Signal update available even if remote <= local
  }
  
  // Update lastCheckedChannel
  strlcpy(_lastCheckedChannel, _channel, sizeof(_lastCheckedChannel));
  ```

### 4. Create tests `test/test_unit/test_ota_channel/test_ota_channel.cpp`

- [ ] GoogleTest suite `OTAChannelTest`
- [ ] Test: SemVer `v3.1.0-stable > v3.1.0-beta.1` (channel ordering — uses existing SemVer lib)
- [ ] Test: SemVer `v3.1.0-beta.2 > v3.1.0-beta.1` (same channel, higher pre-num)
- [ ] Test: SemVer `v3.2.0-dev.1 > v3.1.0` (higher minor overrides channel)
- [ ] Test: `stringToChannel("beta") == Channel::BETA`
- [ ] Test: `stringToChannel("dev") == Channel::DEV`
- [ ] Test: `stringToChannel("stable") == Channel::STABLE`
- [ ] Test: `channelToString(Channel::BETA)` returns `"beta"`
- [ ] Test: channel round-trip: `stringToChannel(channelToString(ch)) == ch` for all channels

## Verification

- [ ] `pio test -e native -f test_ota_channel` — all channel tests pass
- [ ] `pio test -e native` — full suite passes (zero regressions)
- [ ] `pio run -e esp32dev` — hardware build succeeds
- [ ] `pio run -e esp32s3nano` — hardware build succeeds

## Completion

Once all checklist items are verified, **STOP and send a message back to the Principal Architect** confirming:
1. All tests pass
2. Both hardware builds succeed
3. Any issues or decisions that need escalation

Do NOT wrap up, distill, or commit until the PA explicitly approves.

---

## Execution Status

**Status:** ✅ COMPLETE  
**Executed:** 2026-06-05T08:52Z – 2026-06-05T08:57Z  
**Session ID:** `df360f0f-82b2-4a31-b136-5bfd2f8bf35c`  
**Approved by PA:** 2026-06-05T08:57Z

### Verification Results

| Check | Result |
|-------|--------|
| `pio test -e native -f test_ota_channel` | ✅ 15/15 passed |
| `pio test -e native` (full suite) | ✅ 505/505 passed — zero regressions |
| `pio run -e esp32dev` | ✅ Build succeeded |
| `pio run -e esp32s3nano` | ✅ Build succeeded |

### Files Modified

- `lib/GithubClient/GithubClient.hpp` — added `isPreRelease`, `getLatestReleaseForChannel()`
- `lib/GithubClient/GithubClient.cpp` — implemented channel-aware release queries
- `modules/OTAUpdateManager/OTAUpdateManager.hpp` — schema v2, `_lastCheckedChannel`
- `modules/OTAUpdateManager/OTAUpdateManager.cpp` — v2 config, channel-aware checks, implicit downgrade

### Files Created

- `test/test_unit/test_ota_channel/test_ota_channel.cpp` — 15-test GoogleTest suite

### Escalations

None.
