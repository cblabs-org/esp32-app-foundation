# Graveyard: Paginated GitHub API Scanning

## What Was Tried

The original `GithubClient::getLatestReleaseForChannel()` used a paginated loop:

```
GET /repos/{owner}/{repo}/releases?per_page=10&page=1
GET /repos/{owner}/{repo}/releases?per_page=10&page=2
... (up to 5 pages)
```

Each page was parsed with ArduinoJson, filtering for releases matching the subscribed channel's risk floor.

## Why It Failed

1. **Silent version comparison bug**: When the code was refactored to use floating tags but the `tag_name` field still contained `"latest-dev"`, `parseVersionTag("latest-dev")` returned `{0,0,0,0,0}` — a zeroed-out version. This silently caused the floating tag candidate to always lose the comparison against the stable baseline, making the device think it was already up to date.

2. **Log evidence of the bug** (captured from serial monitor):
   ```
   I (12648) GITHUB: Stable baseline: latest-stable
   I (15325) GITHUB: Selected release: latest-stable (floor='dev')
   I (15326) OTAUpdateManager: Best release for 'dev' channel: latest-stable (7 assets)
   I (15327) OTAUpdateManager: No update needed — local 0.0.11-dev.12 is current or newer
   ```
   The selected release was `latest-stable` — a version that parses to `0.0.0` — which is always "older" than the running firmware. So no update was ever triggered.

3. **Memory pressure from pagination** (earlier session): The paginated approach required allocating buffers for each page of JSON (10–50KB each). On the ESP32's 320KB DRAM, this caused heap fragmentation. Combined with the TLS client's ~40KB allocation, the system was frequently running out of memory during OTA checks.

## Fix Applied

- `GithubClient.cpp`: Added `strncmp(tag, "latest-", 7)` guard in both `getLatestRelease()` and `getLatestReleaseForChannel()`. When detected, SemVer is parsed from the `name` (title) field instead of `tag_name`.
- `deploy_release.py`: Changed `--title` from human-friendly text to the exact semantic version tag.

## Do Not Retry

- Do NOT go back to paginated scanning. The memory and latency costs are prohibitive.
- Do NOT try to parse the version from the `body` field — it is intentionally excluded from the JSON filter to save heap.
- Do NOT assume `tag_name` will ever contain a parseable SemVer for floating releases — this is a GitHub API constraint, not a bug.
