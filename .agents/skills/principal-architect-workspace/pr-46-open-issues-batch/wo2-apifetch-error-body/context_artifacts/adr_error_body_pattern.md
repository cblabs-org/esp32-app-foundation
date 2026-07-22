# ADR: Error Body Preservation Pattern in apiFetch

## Context

`window.apiFetch()` threw plain `new Error('HTTP ' + status)` on non-2xx responses. The structured JSON error body returned by the ESP32 firmware (e.g., `{ "error": "invalid field" }`) was discarded before any portal client could read it.

## Decision

Attach `.status` (number) and `.body` (parsed JSON, text, or null) to the Error object at both throw sites.

### Key Constraints

1. **`err.message` format is frozen** — must remain `'HTTP ' + status` for backwards compatibility. Existing catch blocks throughout all portal fragments match on this string pattern.

2. **Body read must be fault-tolerant** — a 4xx/5xx response with an empty body, a binary body, or a network-interrupted body must not produce a *different* error that masks the HTTP status. Hence the inner `try/catch` setting `err.body = null` on failure.

3. **Content-type sniffing** — JSON is attempted only when `Content-Type` header contains `application/json`. Everything else falls back to `.text()`. This matches the success path's existing logic.

4. **Both throw sites need the pattern** — the auth retry path (after 401 interceptor) and the main error path are independent code flows that both can encounter non-2xx responses.

## Alternatives Considered

- **Subclassing Error** — rejected; older browsers and the ES5-style codebase make prototype chain issues likely. Simple property attachment is safe and sufficient.
- **Wrapping in a response-like object** — rejected; the existing API contract is `reject(error)`, not `reject(response)`. Changing this would break all existing `.catch()` handlers.
