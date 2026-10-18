---
name: pr-26-wo1
description: Handoff instructions for Work Order 1 — Paginated Release Scanner
complexity: standard
---

# Role

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Rewrite `getLatestReleaseForChannel()` in `GithubClient.cpp` to use paginated GitHub API requests with three early-exit conditions, replacing the current single-request approach that fails with `IncompleteInput` on large release lists. Also add `useHTTP10(true)` to the existing `getLatestRelease()` for consistency.

# Strict Boundary Box

- **ONLY modify:** `lib/GithubClient/GithubClient.cpp`
- **Do NOT touch:** any file in `modules/`, `web/`, `platformio.ini`, or any `.hpp` files
- **Do NOT modify:** the `#else` native mock block (lines 302–313) — it must remain unchanged
- **Do NOT modify:** the `getLatestRelease()` function beyond adding the single `useHTTP10(true)` line
- **Do NOT modify:** the `GithubClient` constructor or header file

# Line-Item Instructions

## Change 1a — Add `useHTTP10(true)` to `getLatestRelease()`

In `lib/GithubClient/GithubClient.cpp`, inside the `getLatestRelease()` function, find these two adjacent lines (around line 78):

```cpp
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(client, apiGetLatestRelease);
```

Insert `http.useHTTP10(true);` between them:

```cpp
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.useHTTP10(true);
    http.begin(client, apiGetLatestRelease);
```

## Change 1b — Rewrite `getLatestReleaseForChannel()` hardware block

In `lib/GithubClient/GithubClient.cpp`, the function `getLatestReleaseForChannel()` starts at line 170. The hardware implementation is inside the `#ifndef NATIVE_BUILD` block (lines 185–300). Replace the ENTIRE content between `#ifndef NATIVE_BUILD` (line 185) and the line just before `#else` (line 301), keeping the `#ifndef NATIVE_BUILD` directive itself and the `#else` block unchanged.

**Keep unchanged (lines 170–184):**
```cpp
bool GithubClient::getLatestReleaseForChannel(const char* channel) {
    // Stable (or unset) delegates to the single-release fast-path
    if (channel == nullptr || channel[0] == '\0' || strcmp(channel, "stable") == 0) {
        return getLatestRelease();
    }

    releaseId = "";
    releaseDescription = "";
    releaseAssets = 0;
    isPreRelease = false;

    // Convert the subscriber's channel string to a Channel enum floor value.
    // Any release whose parsed channel >= this floor is an acceptable candidate.
    Channel floor = stringToChannel(channel);
```

**Replace the hardware implementation (lines 185–300) with this exact code:**

```cpp
#ifndef NATIVE_BUILD
    // --- Step 1: Fetch stable baseline via /releases/latest ---
    // This populates member fields with the latest stable release.
    bool hasStableBaseline = getLatestRelease();

    if (hasStableBaseline) {
        LOG_SECURE("GITHUB", "Stable baseline: %s", releaseId.c_str());
    } else {
        LOG_SECURE("GITHUB", "No stable baseline found — scanning all releases");
    }

    // Track best candidate found so far.
    // If stable baseline exists, it is the starting best candidate.
    String bestTag         = hasStableBaseline ? releaseId : "";
    String bestDescription = hasStableBaseline ? releaseDescription : "";
    bool   bestIsPre       = false;
    int    bestAssetCount  = hasStableBaseline ? releaseAssets : 0;
    String bestAssetName[MAX_RELEASE_ASSETS];
    String bestAssetURL[MAX_RELEASE_ASSETS];
    FirmwareVersion bestVer = {};
    if (hasStableBaseline) {
        bestVer = parseVersionTag(releaseId.c_str());
        for (int i = 0; i < bestAssetCount; ++i) {
            bestAssetName[i] = releaseAssetName[i];
            bestAssetURL[i]  = releaseAssetURL[i];
        }
    }

    // --- Step 2: Paginated scan of /releases?per_page=5&page=N ---
    static constexpr int MAX_PAGES    = 10;
    static constexpr int PER_PAGE     = 5;
    bool stopScanning = false;

    for (int page = 1; page <= MAX_PAGES && !stopScanning; ++page) {
        // Build paginated URL
        String pageUrl = apiGetReleases + "?per_page=" + String(PER_PAGE) + "&page=" + String(page);
        LOG_SECURE("GITHUB", "Requesting releases page %d (floor='%s')", page, channel);

        // Fresh TLS connection for each page
        WiFiClientSecure pageClient;
        pageClient.setCACert(githubRootCACert);
        pageClient.setTimeout(15000);

        HTTPClient pageHttp;
        pageHttp.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        pageHttp.useHTTP10(true);
        pageHttp.begin(pageClient, pageUrl);
        pageHttp.addHeader("User-Agent", "esp32/1.0");
        pageHttp.addHeader("X-GitHub-Api-Version", "2022-11-28");
        pageHttp.addHeader("Accept", "application/vnd.github+json");
        if (accessToken.length() > 0) {
            pageHttp.addHeader("Authorization", "Bearer " + accessToken);
        }
        pageHttp.setTimeout(15000);

        int httpCode = pageHttp.GET();
        if (httpCode != HTTP_CODE_OK) {
            LOG_SECURE("GITHUB", "Page %d HTTP error %d — stopping scan", page, httpCode);
            pageHttp.end();
            break;  // Graceful degradation: return best found so far
        }

        // Parse page — array filter for minimal memory usage
        JsonDocument filter;
        filter[0]["tag_name"]   = true;
        filter[0]["name"]      = true;
        filter[0]["prerelease"] = true;
        filter[0]["assets"][0]["name"] = true;
        filter[0]["assets"][0]["url"]  = true;

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, pageHttp.getStream(),
                                                      DeserializationOption::Filter(filter));
        pageHttp.end();

        if (error) {
            LOG_SECURE("GITHUB", "Page %d JSON error: %s — stopping scan", page, error.c_str());
            break;  // Graceful degradation
        }

        JsonArray releases = doc.as<JsonArray>();

        // EXIT 3: Empty page — no more releases
        if (releases.size() == 0) {
            LOG_SECURE("GITHUB", "Empty page %d — stopping scan", page);
            break;
        }

        for (JsonObject release : releases) {
            const char* tagName = release["tag_name"] | "";
            if (tagName[0] == '\0') continue;

            bool isPre = release["prerelease"] | false;

            // EXIT 1: Hit a stable release — we already have the latest stable
            // from Step 1. Everything after this is older. Stop scanning.
            if (!isPre) {
                LOG_SECURE("GITHUB", "Hit stable %s in scan — stopping (page %d)", tagName, page);
                stopScanning = true;
                break;
            }

            // Pre-release: check if it meets the subscriber's floor
            FirmwareVersion candidate = parseVersionTag(tagName);
            bool accepted = (static_cast<uint8_t>(candidate.channel) >= static_cast<uint8_t>(floor));

            if (!accepted) {
                LOG_SECURE("GITHUB", "Skipping %s — does not meet floor '%s'", tagName, channel);
                continue;
            }

            // EXIT 2: Found an acceptable prerelease that beats best candidate.
            // Since releases are newest-first, the first acceptable prerelease
            // is the winner — nothing later can beat it.
            if (bestTag.isEmpty() || candidate > bestVer) {
                LOG_SECURE("GITHUB", "Found acceptable prerelease %s — stopping", tagName);
                bestVer         = candidate;
                bestTag         = tagName;
                bestDescription = release["name"] | "";
                bestIsPre       = true;
                bestAssetCount  = 0;

                JsonArray assetsArray = release["assets"].as<JsonArray>();
                for (JsonObject asset : assetsArray) {
                    if (bestAssetCount >= MAX_RELEASE_ASSETS) break;
                    String name = asset["name"] | "";
                    String url  = asset["url"]  | "";
                    if (name.length() > 0 && url.length() > 0) {
                        bestAssetName[bestAssetCount] = name;
                        bestAssetURL[bestAssetCount]  = url;
                        bestAssetCount++;
                    }
                }
                stopScanning = true;
                break;
            }
        }
    }

    // --- Step 3: Commit the best candidate to member fields ---
    if (bestTag.isEmpty()) {
        snprintf(lastErrorMsg, sizeof(lastErrorMsg),
                 "No release found at or above channel floor '%s'", channel);
        LOG_SECURE("GITHUB", "%s", lastErrorMsg);
        return false;
    }

    releaseId          = bestTag;
    releaseDescription = bestDescription;
    isPreRelease       = bestIsPre;
    releaseAssets      = bestAssetCount;
    for (int i = 0; i < bestAssetCount; ++i) {
        releaseAssetName[i] = bestAssetName[i];
        releaseAssetURL[i]  = bestAssetURL[i];
    }

    LOG_SECURE("GITHUB", "Selected release: %s (floor='%s')", releaseId.c_str(), channel);
    strcpy(lastErrorMsg, "SUCCESS");
    return true;
```

## Change 1c — Update docstring

In the same file, find this line in the docstring above `getLatestReleaseForChannel()` (around line 136):

```
 * A single /releases request is made; the highest SemVer version among
```

Replace with:

```
 * A paginated scan of /releases is made; the highest SemVer version among
```

# Verification

1. Run: `pio test -e native`
2. All tests must pass. The native mock is unchanged, so existing tests should work as before.

# Completion

After verification passes, **STOP and send a message back to the Principal Architect** reporting:
- Whether all tests passed
- Any issues encountered
- Do NOT commit, distill, or wrap up until explicitly told to do so

## Status Update (2026-06-15)

- **Status:** COMPLETE
- **Implementation Details:**
  - Added `http.useHTTP10(true);` to `getLatestRelease()` in `lib/GithubClient/GithubClient.cpp`.
  - Rewrote the hardware block of `getLatestReleaseForChannel()` to use a paginated scan (up to 10 pages of 5 items per page) with early-exit conditions when hitting a stable release, finding an acceptable prerelease that beats the best candidate, or hitting an empty page.
  - Skipped Change 1c because `A single /releases request is made; the highest SemVer version among` resides in `lib/GithubClient/GithubClient.hpp`, which is outside the strict boundary box.
- **Verification:** Ran `pio test -e native`. All 562 tests passed.
