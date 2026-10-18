# ADR: Arduino String Migration Scope

## Context

The compliance audit flagged Arduino `String` usage as a memory safety concern (fragmentation
from dynamic reallocation on a heap-constrained embedded device). WO6 migrated all modules to
`std::string`. WO11 extended this to all authored libraries.

## Decision: Three-Tier Migration Strategy

### Tier 1: Modules (WO6) — Full migration ✅
All `modules/` code now uses `std::string` exclusively. No Arduino `String` in internal logic.

### Tier 2: Authored Libraries (WO11) — Full migration ✅
All libraries under `lib/` are project-authored (copyright "ESP32 App Foundation (c) 2026"):
- **GithubClient** — constructor, members, public APIs all `std::string`
- **OTADownloadEngine** — `fetchSignature()`, `downloadAndFlash()` params all `std::string`
- **MD5Utils** — return types and params all `std::string`
- **Logger** — internals migrated (`std::vector<std::string> secrets`, unified `redactInPlace()`)

### Tier 3: Framework API Interop — LEFT ALONE (deliberate)
At boundaries where ESP32 framework APIs impose Arduino `String`:
- **ArduinoJson** — `serializeJson()` returns `String`, `doc["key"].as<String>()`
- **PsychicHttp** — `PsychicRequest::body()`, multipart upload filename
- **ESP32 base64** — `base64::encode()` returns `String`

These are NOT migrated. Converting at the boundary (via `.c_str()` or `String(x.c_str())`)
is the correct interop pattern. Migrating would break upgradability to upstream library versions.

## Logger Overload Design

Logger keeps a `registerSecret(const String& secret)` convenience overload for callers that
still work with Arduino `String` (e.g. at framework API boundaries). Internally it delegates
to the `std::string` version — the delegation direction was reversed from the original
(previously `std::string` delegated to `String`, now `String` delegates to `std::string`).

## Key Decisions

1. **All `lib/` code is authored** — verified via copyright headers. No third-party libraries
   exist in `lib/`. This was the user's concern: "Public libraries we are importing should be
   left alone so we can upgrade to upstream new versions."
2. **ArduinoJson 7 supports `std::string`** natively — future incremental migration possible
   but not required.
3. **`handleSignedUpload` in OTAUpdateManager** keeps `const String&` for `filename` — this
   is an external constraint from PsychicHttp's multipart upload API.
