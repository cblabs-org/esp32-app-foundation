# State: Deploy ↔ GithubClient Contract

## The Contract

The `deploy_release.py` script and `GithubClient.cpp` on the ESP32 share a critical implicit contract for floating tag releases:

### deploy_release.py (Server Side)

```python
# Line ~303 in deploy_release.py
floating_release_cmd = [
    "gh", "release", "create", floating_tag,
    "--title", tag,           # ← MUST be the exact semantic version (e.g. v0.0.11-dev.13)
    "--notes", f"Alias for {tag}"
]
```

### GithubClient.cpp (Device Side)

```cpp
// In getLatestRelease() — stable baseline path
releaseId = doc["tag_name"] | "";
releaseDescription = doc["name"] | "";
if (strncmp(releaseId.c_str(), "latest-", 7) == 0 && !releaseDescription.empty()) {
    releaseId = releaseDescription;  // ← Reads SemVer from title
}

// In getLatestReleaseForChannel() — floating tag path
const char* tagName = doc["tag_name"] | "";
const char* releaseName = doc["name"] | "";
if (strncmp(tagName, "latest-", 7) == 0 && releaseName[0] != '\0') {
    tagName = releaseName;  // ← Reads SemVer from title
}
```

### GitHub API Response for a Floating Release

```json
{
  "tag_name": "latest-dev",           // ← NOT parseable as SemVer
  "name": "v0.0.11-dev.13",          // ← This IS the SemVer (set by --title)
  "body": "Alias for v0.0.11-dev.13"
}
```

## What Breaks If You Change This

| Change | Consequence |
|---|---|
| Change `--title` to human-friendly text | ESP32 parses `0.0.0`, never detects updates |
| Remove the `strncmp` guard in GithubClient | Real floating tags get parsed as `0.0.0` |
| Add `filter["body"] = true` to extract version from notes | Heap allocation spike from large release notes |

## Files Involved

- `scripts/deploy_release.py` — Lines ~299-303
- `lib/GithubClient/GithubClient.cpp` — Lines ~184-187 (stable path), ~364-370 (channel path)
- `.agents/skills/framework-expert/systems/ota/deployment.md` — Documents the constraint
- `docs/developer/ota-update-system.md` — § 4.1.1 Floating Tags
