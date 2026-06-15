# Architecture Decision Record: OTA Reliability & Alignment

## Context
The Over-The-Air (OTA) update mechanism was failing on pre-release updates (development, beta) for two primary reasons:
1. **GitHub client buffer stalling:** Large release lists queried in a single call caused the secure TCP socket (`WiFiClientSecure`) to stall or run out of memory, manifesting as `IncompleteInput` deserialization errors.
2. **Hardcoded version channel verification:** The local firmware version was hardcoded to `Channel::STABLE` inside the update check, rendering version comparisons invalid for dev or beta channels.
Additionally, the web portal showed static, hardcoded progress jumps (50% download, 85% verify) instead of real progress matching the device display (2% -> 100%).

## Decision
1. **Paginated Releases Scan:** We updated `getLatestReleaseForChannel()` to scan GitHub releases page by page (`per_page=5`), and added early-exit criteria to stop scanning:
   - When hitting a stable release (the latest stable is fetched first as a baseline).
   - When finding a prerelease matching the channel floor that is newer than the best candidate.
   - When hitting an empty page.
2. **Dynamic Channel Determination:** We replaced `Channel::STABLE` with `static_cast<Channel>(VERSION_CHANNEL)` and `VERSION_PRE_NUM` build flags injected at compile time.
3. **Exposed Real Progress:** Exposed the download progress float `lastProgress_` in the HTTP status API (`/api/ota`), allowing the portal script (`ota.js`) to display real progress values.

## Consequences
- Highly reliable OTA check even with hundreds of GitHub releases.
- Native unit tests and hardware compilation verify correct channel-floor logic.
- Web assets were rebuilt and verified successfully.
