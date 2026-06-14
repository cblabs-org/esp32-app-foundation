# Graveyard: OTA Beta-Channel IncompleteInput

## Symptom
`JSON Parse Error: IncompleteInput` in `GithubClient::getLatestReleaseForChannel()`
when `channel = "beta"`. Occurs ~3.5s after request starts. Stable channel
(`getLatestRelease()` → `/releases/latest`) works fine.

## Timeline
- Device always defaulted to `channel = "stable"` — the `/releases` list
  endpoint had NEVER been exercised on real hardware before this session.
- User changed channel to "beta" for first time → failure.

## What Was Tried (and reverted)
**Premature fix applied then reverted:** Added `Accept-Encoding: identity` header
to both `getLatestRelease()` and `getLatestReleaseForChannel()` plus diagnostic
log lines (`HTTP %d | Content-Length: %d | Content-Encoding: %s`).

**Why reverted:** User confirmed OTA has worked before on stable channel; the
gzip theory was speculative without evidence. The fix was reverted pending proper
instrumentation-first investigation.

## Root Cause: Unknown (needs investigation)
Candidates:
1. GitHub gzip-compressing large `/releases` list (~30-50KB) while
   `/releases/latest` (~2-3KB) is served uncompressed
2. WiFiClientSecure stream timeout (10s) cutting connection mid-response
3. JsonDocument capacity issue with large array
4. Chunked transfer encoding not handled by `http.getStream()`

## What the Diagnostic Log Would Show
If `Accept-Encoding: identity` + log lines were re-applied and a build flashed,
the new log line `HTTP 200 | Content-Length: XXXX | Content-Encoding: identity`
would confirm or deny the gzip theory.

## Deferred To
`todo.md` item: "Debug and harden the beta-channel OTA GitHub check" — 5-point
investigation plan with session reference `5177dae5`.

## Second Bug Noted (not fixed)
`local` version hardcoded as `Channel::STABLE` in OTAUpdateManager.cpp lines
297 + 482. Device running 0.0.6-beta compares as "0.0.6 stable", beating any
0.0.6-beta.x remote. `BUILD_CHANNEL` needs wiring through `inject_version.py`.
