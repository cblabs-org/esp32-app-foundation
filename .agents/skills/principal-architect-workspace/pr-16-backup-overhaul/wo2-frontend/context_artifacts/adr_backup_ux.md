# ADR: Backup Tab UX Improvements (WO2)

## Decision 1 — File size guard raised to 16 KB

**What:** Client-side `file.size > 8192` check replaced with `file.size > 16384`.

**Why:** WO1 expanded the backup payload to include a `_meta` block (7 fields) plus `_v` version keys on every module namespace. The pre-WO1 payload was routinely under 4 KB; post-WO1 a 5-module device easily exceeds 8 KB when pretty-printed. The 16 KB cap aligns with the server-side limit change made in WO1 firmware.

---

## Decision 2 — Metadata summary uses `response._meta`, not the filename

**What:** After download, the success message is built from `response._meta` if present, otherwise falls back to the bare filename.

**Why:** `apiFetch` already calls `response.json()` internally (via `ApiQueue`) and returns the parsed JS object. There is no streaming blob to inspect for headers server-side; the data is already in memory. Reading `_meta` directly from the parsed object is the only reliable path — inspecting HTTP response headers (e.g. `Content-Disposition`) is not possible after `ApiQueue` consumes the response.

Fallback to filename ensures graceful degradation for devices still on pre-WO1 firmware that return no `_meta`.

---

## Decision 3 — Module preview uses `FileReader` in the `change` event, not `file.text()` in the click handler

**What:** A `FileReader` reads and parses the JSON immediately on file selection. The parsed module list is shown in `backup-restore-status`, filtered with `Object.keys(parsed).filter(k => k !== '_meta')`.

**Why:** Using the async `change` event means the user sees `Will restore: wifi, mcp, time, display, ota` before they decide to click Upload. This is a UX-first decision. Delaying to the click handler would make the preview too late.

`FileReader` is used instead of the modern `file.text()` because the change handler is synchronous-event-scoped and the FileReader callback pattern works more predictably across older WebKit targets the ESP32 portal may run on (embedded browser in a mobile app, etc.).

---

## Decision 4 — Shallow spread + `delete` for `_meta` stripping

**What:**
```js
const payload = { ...parsed };
delete payload._meta;
```

**Why:** The spec called for a belt-and-braces approach. Using a shallow spread rather than a deep clone is intentional — the module sub-objects should be sent as-is (they contain `_v` keys the server uses for versioned upsert). Only the top-level `_meta` key needs removal. `JSON.stringify(payload)` is used instead of the original `text` string to ensure the stripped payload is serialised correctly.
