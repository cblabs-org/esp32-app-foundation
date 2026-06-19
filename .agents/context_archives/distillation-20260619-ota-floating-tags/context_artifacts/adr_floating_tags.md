# ADR: Floating Tag Architecture for OTA Polling

## Status: Accepted

## Context

The ESP32 OTA system originally used the GitHub Releases API with pagination (`/releases?per_page=10&page=N`) to scan through all releases and find the best candidate for the device's subscribed channel. This had three critical problems:

1. **Memory exhaustion**: Each page response could be 10–50KB of JSON. On the ESP32 (320KB DRAM), parsing multiple pages caused heap fragmentation and out-of-memory crashes.
2. **Latency**: Each page required a full TLS handshake + HTTP round-trip (2–5 seconds on ESP32). With many releases, this could take 30+ seconds.
3. **Rate limiting**: GitHub's unauthenticated API allows 60 requests/hour. A device checking every 30 minutes with 3-page pagination burns 6 requests/hour per device — a fleet of 10 devices would exceed the limit.

## Decision

Replace the paginated scan with **floating tags** — lightweight GitHub Releases (`latest-dev`, `latest-beta`, `latest-rc`, `latest-stable`) whose underlying Git tag is force-moved to the latest commit of that channel by `deploy_release.py`.

The device now performs exactly **two** HTTP requests:
1. `GET /releases/latest` — stable baseline (always the highest non-prerelease).
2. `GET /releases/tags/latest-{channel}` — floating tag for the subscribed channel.

The higher SemVer of the two candidates wins.

### The Title Contract

GitHub's API returns the literal tag name (e.g. `"latest-dev"`) in the `tag_name` field of the floating release JSON. This is not a parseable SemVer string. Rather than downloading the `manifest.json` asset to determine the version (which would add another HTTP request and memory allocation), we encode the semantic version in the GitHub Release **title** (`name` field).

- `deploy_release.py` creates the floating release with `--title v0.0.11-dev.13`
- `GithubClient.cpp` detects `tag_name` starting with `"latest-"` and reads SemVer from `name` instead

This is a **critical cross-component contract**. Both sides must agree on the format.

### Alternatives Considered

- **Option B: Download `manifest.json` from the floating release** — Rejected because it requires a third HTTP request per check cycle and ~8KB of additional heap allocation.
- **Option C: Encode version in `body` (release notes)** — Rejected because the JSON `body` field is intentionally excluded from the ArduinoJson filter to avoid allocating potentially large release notes in DRAM.
- **Option D: Use GitHub's GraphQL API** — Rejected because it requires authentication (no anonymous access) and the query syntax is more complex to maintain on the ESP32.

## Consequences

- OTA checks are now O(1) regardless of how many releases exist in the repository.
- The `deploy_release.py` script has a new Step 6 that manages the floating tag lifecycle (delete old → create new → upload assets).
- Existing floating tags created before this session may have incorrect titles (e.g. `"Latest Dev Build"` instead of the SemVer). The next deployment will automatically fix them.
- The `--clobber` flag is used on asset uploads to handle re-deployment to the same tag.
