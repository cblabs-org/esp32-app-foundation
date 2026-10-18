# ADR: Channel-Aware GitHub Release Queries

## Decision: Stable Delegates to `/releases/latest`

**Context:** GitHub's `/releases/latest` endpoint only returns non-prerelease releases. For stable channel queries, this is the exact behavior we want.

**Decision:** `getLatestReleaseForChannel("stable")` delegates directly to the existing `getLatestRelease()` method, which queries `/releases/latest`. This avoids parsing the full releases array when only the latest stable is needed.

**Rationale:** Memory efficiency on ESP32 — parsing a single release JSON object is significantly cheaper than parsing an array of up to 30.

## Decision: Use `strstr()` for Tag-Channel Matching

**Context:** Pre-release tags follow the pattern `v3.1.0-beta.1`, `v3.1.0-dev.3`. We need to determine if a release tag "belongs" to a given channel.

**Decision:** Use `strstr(tagName, channel)` — simple substring search.

**Rationale:** This is intentionally loose matching. A tag like `v3.1.0-beta.1` contains `"beta"`, which matches. This works because our channel strings (`"beta"`, `"dev"`) are unlikely to appear as false positives in version tags. A more rigid regex parser would consume unnecessary flash on MCU.

**Trade-off:** A tag like `v3.1.0-debug.1` would falsely match `"dev"` (since "dev" is not a substring of "debug" — actually this is fine). Edge case: a tag `v3.1.0-beta-dev.1` would match both channels — acceptable since such tags shouldn't exist in practice.

## Decision: First-Match from `/releases` (Newest First)

**Context:** GitHub returns releases sorted newest-first by default.

**Decision:** Take the first release from the `/releases` array that matches `prerelease == true` and has the channel substring in its tag.

**Rationale:** This gives us the latest pre-release for the channel without needing any SemVer comparison during the scan phase. The SemVer comparison happens later in `checkForUpdate()` against the local version.

## Decision: Implicit Downgrade via `lastCheckedChannel`

**Context:** When a user switches from `beta` to `stable`, the latest stable release may have a lower version number than the installed beta. Without special handling, the SemVer comparison would say "no update available".

**Decision:** Track `_lastCheckedChannel` in the config. If `_channel != _lastCheckedChannel` and `_lastCheckedChannel` is non-empty, signal "update available" regardless of version comparison direction.

**Rationale:** This allows the device to "downgrade" to the appropriate channel release. The one-shot nature (it persists the new channel immediately) prevents infinite update loops.
