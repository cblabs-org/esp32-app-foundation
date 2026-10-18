---
name: pr-26-wo2
description: Handoff instructions for Work Order 2 — OTA Logging + Version Display
complexity: mechanical
---

# Role

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Update all OTA log lines in `OTAUpdateManager.cpp` to include the channel name and full `BUILD_VERSION` string (instead of `VERSION_MAJOR.MINOR.PATCH`). Also fix the status API handler to return `BUILD_VERSION` instead of a manually constructed version string.

# Strict Boundary Box

- **ONLY modify:** `modules/OTAUpdateManager/OTAUpdateManager.cpp`
- **Do NOT touch:** `lib/GithubClient/`, `platformio.ini`, `web/`, or any `.hpp` files
- **Do NOT touch:** any `FirmwareVersion local = ...` lines (those are WO3's responsibility)

# Line-Item Instructions

All changes below are simple find-and-replace operations in `modules/OTAUpdateManager/OTAUpdateManager.cpp`. Find the exact string and replace it. There are 8 changes total.

## 2a — `checkForUpdate()` (~line 285)

**Find:**
```cpp
    ESP_LOGI(TAG, "Checking GitHub for firmware updates...");
```
**Replace with:**
```cpp
    ESP_LOGI(TAG, "Checking for updates — local: %s, channel: '%s'", BUILD_VERSION, _channel);
```

## 2b — `checkForUpdate()` (~line 295)

**Find:**
```cpp
    ESP_LOGI(TAG, "Latest release: %s (%d assets)", github.releaseId.c_str(), github.releaseAssets);
```
**Replace with:**
```cpp
    ESP_LOGI(TAG, "Best release for '%s' channel: %s (%d assets)",
             _channel, github.releaseId.c_str(), github.releaseAssets);
```

## 2c — `checkForUpdate()` (the "no update" log)

**Find:**
```cpp
    ESP_LOGI(TAG, "No update available — local version is current or newer");
```
**Replace with:**
```cpp
    ESP_LOGI(TAG, "No update available — local %s is current or newer than remote %s (channel='%s')",
             BUILD_VERSION, github.releaseId.c_str(), _channel);
```

## 2d — `performUpdateTask()` (~line 472)

**Find:**
```cpp
    ESP_LOGI(TAG, "Querying GitHub for latest release of %s", cfg.otaGithubRepo);
```
**Replace with:**
```cpp
    ESP_LOGI(TAG, "Querying GitHub for latest release — repo: %s, channel: '%s', local: %s",
             cfg.otaGithubRepo, _channel, BUILD_VERSION);
```

## 2e — `performUpdateTask()` (~line 480)

**Find:**
```cpp
    ESP_LOGI(TAG, "Found release: %s with %d assets",
             github.releaseId.c_str(), github.releaseAssets);
```
**Replace with:**
```cpp
    ESP_LOGI(TAG, "Best release for '%s' channel: %s (%d assets)",
             _channel, github.releaseId.c_str(), github.releaseAssets);
```

## 2f — `performUpdateTask()` (~line 487)

**Find:**
```cpp
    ESP_LOGI(TAG, "No update needed — local %d.%d.%d is current or newer than remote %s",
             VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, github.releaseId.c_str());
```
**Replace with:**
```cpp
    ESP_LOGI(TAG, "No update needed — local %s is current or newer than remote %s (channel='%s')",
             BUILD_VERSION, github.releaseId.c_str(), _channel);
```

## 2g — `performUpdateTask()` (~line 494)

**Find:**
```cpp
    ESP_LOGI(TAG, "Update available: %s > %d.%d.%d — proceeding with download",
             github.releaseId.c_str(), VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
```
**Replace with:**
```cpp
    ESP_LOGI(TAG, "Update available: %s > %s (channel='%s') — proceeding with download",
             github.releaseId.c_str(), BUILD_VERSION, _channel);
```

## 2h — Status API handler (~line 1222)

**Find:**
```cpp
                    resp["version"]        = String(VERSION_MAJOR) + "." + String(VERSION_MINOR) + "." + String(VERSION_PATCH);
```
**Replace with:**
```cpp
                    resp["version"]        = BUILD_VERSION;
```

# Verification

1. Run: `pio test -e native`
2. All tests must pass.

# Completion

After verification passes, **STOP and send a message back to the Principal Architect** reporting:
- Whether all tests passed
- The count of log lines changed (should be 8)
- Any issues encountered
- Do NOT commit, distill, or wrap up until explicitly told to do so

## Status Update

- **Status**: Completed
- **Date**: 2026-06-15
- **Details**: All 8 changes implemented and successfully verified. All unit tests passed under native environment.

