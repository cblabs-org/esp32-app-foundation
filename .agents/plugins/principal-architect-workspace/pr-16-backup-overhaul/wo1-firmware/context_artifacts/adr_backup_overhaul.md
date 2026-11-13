# ADR: Backup System Route Ordering, AsyncResponseStream, and _meta Envelope

## Context

The backup endpoint `GET /api/config/backup` was silently broken. ESPAsyncWebServer uses prefix-greedy matching — routes registered earlier swallow all requests whose URL starts with the registered prefix. `GET /api/config` was registered before `GET /api/config/backup`, so backup requests never reached the backup handler.

## Decision 1: Route Registration Order

**Decision:** Register `/api/config/backup` and `/api/config/restore` BEFORE `/api/config` in `ConfigManager::registerRoutes()`.

**Why:** ESPAsyncWebServer's `on()` method matches the first registered handler whose prefix matches the incoming path. There is no specificity ranking (unlike e.g. Express.js). The only fix is registration order. Confirmed by live test: after fix, `/api/config/backup` fires the backup handler and `/api/config` still fires the namespace-list handler.

**Alternatives rejected:** None viable — the library does not expose route-priority APIs.

## Decision 2: AsyncResponseStream over String Buffer

**Decision:** Use `AsyncResponseStream` (chunked streaming response) instead of building the full JSON into a `String` and calling `request->send(200, ...)`.

**Why:** The backup JSON document can exceed 4KB when many modules are registered. A heap-allocated `String` of that size risks `String` fragmentation and OOM on ESP32. `AsyncResponseStream` writes directly to the TCP send buffer in chunks, never holding the full payload in heap simultaneously.

**Trade-off:** Slightly more complex code (must call `beginChunkedResponse`, `write`, `end`). The benefit (reliable large-payload delivery) is worth it.

## Decision 3: _meta Envelope Design

**Decision:** `_meta` is written as the **first key** in the exported JSON document (before any module namespace key) and contains:
```json
{
  "backup_version": 1,
  "firmware_version": "unknown",
  "device_id": "<device-wifi-mac>",
  "device_type": "ESP32-S3",
  "datetime": "2026-06-03T22:17:11Z",
  "timezone": "UTC",
  "module_count": 5
}
```

**Why first?** The web portal can read the meta fields without parsing the full document. Also enables partial streaming parsers to detect backup format before processing module data.

**`firmware_version` = "unknown":** The project does not yet have a build-time version injection mechanism. Marked as TODO.

**`timezone` = "UTC" (default):** See `adr_ota_iconfigurable.md` for the coupling decision.

## Decision 4: importAllConfig() Unknown Key Handling

**Decision:** Use a two-pass approach in `importAllConfig()`: (1) iterate registered modules, match their namespace key in the JSON, import it; (2) iterate all JSON top-level keys and log at DEBUG level any key that does not match a registered module namespace (e.g. `_meta`).

**Why:** Silently skipping unknown keys makes the restore path forward/backward compatible. The DEBUG log ensures operability without spamming production logs.
