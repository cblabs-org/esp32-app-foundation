---
name: open-issues-batch-wo2
description: "Handoff instructions for Work Order 2: Preserve error body in apiFetch (Issue #31)"
complexity: standard
---

# WO2 — Issue #31: Preserve Error Body in apiFetch

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix `window.apiFetch()` (via `ApiQueue.processNext()`) so that non-2xx responses preserve the structured JSON error body. Currently, the error body is discarded before it can be read by any portal client. Also fix inaccurate documentation in `shared-utilities.md`.

## Parent Project

`pr-46-open-issues-batch` — user approved the decomposition on 2026-07-22. See `PROJECT.md` in the parent directory.

## Strict Boundary Box

- You MUST ONLY modify the files listed in the checklist below.
- You MUST NOT modify any C++ source files or build system files.
- You MUST NOT change the `err.message` format — it MUST remain `'HTTP ' + status` for backwards compatibility.
- You MUST NOT change the success path (lines 53–58 of `app.js`) — only modify the error/throw paths.
- Surface any out-of-scope findings to the PA in your completion message.

## Tooling Constraints

- Use the `platformio` MCP tools for any build verification (not raw `pio` CLI).
- For web tests, run: `cd test/web && npx playwright test`

## Line-Item Instructions

- [ ] **1. Read the current code**: Open `web/portal/app.js` and read `ApiQueue.processNext()` (lines 24–66). Understand the two throw sites:
  - Line 41: auth retry path — `if (!retryResponse.ok) throw new Error('HTTP ' + retryResponse.status);`
  - Line 51: main path — `if (!response.ok) throw new Error('HTTP ' + response.status);`

- [ ] **2. Fix the main error path (line 51)**: Replace the simple throw with body-preserving error construction:

```js
if (!response.ok) {
  var err = new Error('HTTP ' + response.status);
  err.status = response.status;
  try {
    var ct = response.headers.get('content-type');
    err.body = (ct && ct.indexOf('application/json') !== -1)
      ? await response.json()
      : await response.text();
  } catch (e) {
    err.body = null;  // body unreadable/empty — not fatal
  }
  throw err;
}
```

Key requirements:
- `err.message` stays exactly `'HTTP ' + response.status` (backwards compat)
- `err.status` is the numeric HTTP status code
- `err.body` is the parsed JSON object (if content-type is JSON) or text string, or `null` if unreadable
- The body read is wrapped in its own try/catch — a 4xx/5xx with an empty body must not cause a different error

- [ ] **3. Fix the auth retry error path (line 41)**: Apply the same pattern to the auth retry throw:

```js
if (!retryResponse.ok) {
  var retryErr = new Error('HTTP ' + retryResponse.status);
  retryErr.status = retryResponse.status;
  try {
    var rct = retryResponse.headers.get('content-type');
    retryErr.body = (rct && rct.indexOf('application/json') !== -1)
      ? await retryResponse.json()
      : await retryResponse.text();
  } catch (e) {
    retryErr.body = null;
  }
  throw retryErr;
}
```

- [ ] **4. Update `shared-utilities.md`**: Open `.agents/skills/framework-expert/systems/web-portal/shared-utilities.md` and fix the `window.apiFetch` documentation:
  - **Fix return type**: Change from `Promise<Response>` to accurate description — on success, resolves with the parsed JSON object (if response is `application/json`) or text string.
  - **Fix usage example**: Remove the `await response.json()` call — `apiFetch` already parses the body.
  - **Add error documentation**: Document the error shape thrown on non-2xx:
    - `err.message` — `'HTTP ' + status` (string)
    - `err.status` — HTTP status code (number)
    - `err.body` — Parsed JSON error object, text string, or `null` if body was unreadable

- [ ] **5. Run the Playwright web tests** to verify no regressions:
```bash
cd test/web && npx playwright test
```
All existing tests must pass.

## Verification Requirement

After completing all checklist items:
1. Confirm all Playwright tests pass.
2. **STOP and ask the user** if they are happy with the changes.
3. Only after user approval: commit your changes with message `fix(portal): preserve error body in apiFetch on non-2xx (Issue #31)`, then invoke the `@distillery` skill to package the state, passing `.agents/skills/principal-architect-workspace/pr-46-open-issues-batch/wo2-apifetch-error-body/` as the target directory.
4. Append a status update to this WORKORDER.md and send a completion message to the PA.

## Status Log

| Date | Event |
|------|-------|
| 2026-07-22 | Worker began execution — read code, verified provenance |
| 2026-07-22 | Fixed both throw sites in `ApiQueue.processNext()` (main + auth retry) |
| 2026-07-22 | Updated `shared-utilities.md` — corrected return type, usage example, added error shape docs |
| 2026-07-22 | 69/69 Playwright tests passing — zero regressions |
| 2026-07-22 | PA approved — committed as `079f5c6` |
| 2026-07-22 | Distillery packaged — `context_bridge.md`, `sessions.md`, 2 context artifacts |
| 2026-07-22 | **WO2 COMPLETE** |

