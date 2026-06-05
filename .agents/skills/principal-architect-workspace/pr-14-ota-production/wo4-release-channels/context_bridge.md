---
title: "WO4 Release Channels — Execution Context Bridge"
distilled_at: "2026-06-05T08:57:00Z"
original_plan_id: "pr-14-ota-production"
artifacts:
  - context_artifacts/adr_channel_aware_queries.md
  - context_artifacts/state_schema_v2.md
---

# WO4 — Release Channels: Context Bridge

## Executive Summary

Implemented full OTA release channel support (`stable`/`beta`/`dev`) across GithubClient and OTAUpdateManager. GithubClient gained `getLatestReleaseForChannel()` which delegates stable queries to the existing `/releases/latest` endpoint and scans `/releases` for pre-releases matching beta/dev channels. OTAUpdateManager was upgraded to IConfigurable schema v2 with `lastCheckedChannel` tracking and implicit downgrade logic when the channel changes.

## What Was Done

- **GithubClient**: Added `isPreRelease` flag, `getLatestReleaseForChannel(const char* channel)` method, and `/releases` endpoint support
- **OTAUpdateManager**: Schema v1→v2 migration, `_lastCheckedChannel[16]` field, channel-aware `checkForUpdate()` and `performUpdateTask()`, implicit downgrade detection
- **Tests**: 15-test `OTAChannelTest` suite covering SemVer cross-channel comparisons, `stringToChannel`/`channelToString` round-trips

## Verification Results

| Check | Result |
|-------|--------|
| `pio test -e native -f test_ota_channel` | ✅ 15/15 passed |
| `pio test -e native` (full suite) | ✅ 505/505 passed — zero regressions |
| `pio run -e esp32dev` | ✅ Build succeeded (RAM: 14.9%, Flash: 88.0%) |
| `pio run -e esp32s3nano` | ✅ Build succeeded (RAM: 15.5%, Flash: 59.3%) |

## Next Steps

No further action required for WO4. The following WOs may build on this:
- **WO5 (ConfigManager)**: Will consume the schema v2 config written here
- **WO6 (Web Portal)**: Will add UI for channel selection, reading/writing the `channel` field

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_channel_aware_queries.md` - Architecture decisions: why stable delegates, why strstr for tag matching, why first-match from /releases
- `context_artifacts/state_schema_v2.md` - Final schema v2 field layout, migration logic, and implicit downgrade state machine
