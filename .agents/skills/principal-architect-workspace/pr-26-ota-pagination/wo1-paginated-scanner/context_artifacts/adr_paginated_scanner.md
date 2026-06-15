# ADR: Paginated Release Scanner with Early Exits

## Context

The previous implementation of `getLatestReleaseForChannel` fetched all repository releases in a single HTTP request using `/repos/{repo}/releases`. This became problematic for repositories with a large number of releases because the response payload could exceed memory buffers or cause HTTP parsing/deserialization errors such as `IncompleteInput` under ArduinoJson.

## Decision

We replaced the single-request logic with a three-step paginated scanning strategy in the hardware build (`#ifndef NATIVE_BUILD`):

1. **Establish a Stable Baseline**: We first query `/repos/{repo}/releases/latest` to retrieve the latest stable release. This serves as our baseline.
2. **Paginate with Early Exits**: We scan the `/releases` endpoint page by page (up to 10 pages, 5 releases per page). For each release:
   - **Exit 1 (Stable Release Hit)**: If we hit a stable release in the scan, we stop immediately. Since we already have the latest stable baseline from Step 1, any subsequent releases in the scan (which are ordered newest-first) will be older.
   - **Exit 2 (Better Candidate Found)**: If we find a pre-release that meets the channel stability floor and is newer (via SemVer comparison) than our current best candidate, we select it as the winner and stop scanning. Since releases are returned in newest-first order, the first acceptable pre-release we find is guaranteed to be the newest acceptable one.
   - **Exit 3 (Empty Page)**: If a page returns zero releases, we stop scanning.
3. **Commit Winner**: If a valid release meeting the floor is found, we copy it to the member fields.

Additionally, we forced the HTTP client to use `HTTP/1.0` via `http.useHTTP10(true);` to disable chunked transfer encoding, which prevents connection leaks and improves response streaming reliability on ESP32.

## Consequences

- Improved memory footprint by filtering JSON payloads and parsing pages of size 5 rather than the entire release history.
- Network activity is minimized: in most cases, the scanner will stop on the first page.
- Native mocks remain unchanged, preserving compatibility with native unit tests.
