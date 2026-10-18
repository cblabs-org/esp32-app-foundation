# ADR: Web Portal Testing Architecture

## Decision: Zero-Dependency Mock Server

**Choice:** Raw `http.createServer()` using only Node.js built-ins (`http`, `fs`, `path`).
**Rejected:** Express.js or other npm runtime dependencies.
**Why:** The server's sole purpose is to serve static portal files and return JSON mocks. Express adds unnecessary weight. The departures-board reference proved this pattern works at scale. Zero runtime deps means `node server.js` works immediately without `npm install` (only Playwright dev-deps need installing).

## Decision: Route-Level Mocking over Server-Level

**Choice:** Most API mocking is done per-test via Playwright's `page.route()`, not in server.js.
**Why:** Each test needs full control over response shapes, error states, and timing. Server.js provides only baseline mocks needed for the SPA to boot without JS errors. Individual specs then layer on overrides. This pattern was proven in departures-board where only 3 of 20+ endpoints were mocked in the server.

## Decision: TypeScript for Tests, JavaScript for Server

**Choice:** `.spec.ts` test files with TypeScript, `server.js` in plain JavaScript.
**Why:** TypeScript gives type safety on Playwright's complex API (`Page`, `Route`, `Locator`), catching mistakes at compile time. The server is trivial enough that TypeScript adds no value — it's just `if/else` JSON responses.

## Decision: Build Gate Ordering (Tests Before Builder)

**Choice:** `run_web_tests.py` runs BEFORE `portalBuilder.py` in platformio.ini.
**Why:** Tests validate source portal files. If tests pass, the builder then minifies/gzips them into PROGMEM. This catches logic bugs before the minification step, which is the more common failure mode. A future enhancement could add a second test pass after minification to catch builder-induced regressions.

## Decision: `sys.exit(1)` Hard Gate

**Choice:** Test failure calls `sys.exit(1)`, halting the entire PlatformIO build.
**Why:** This is the critical enforcement mechanism. Without it, broken portal code gets flashed to hardware. The `SKIP_WEB_TESTS` env var provides an escape hatch for firmware-only work.

## Decision: Skip Screenshot Endpoint

**Choice:** Do not mock `/api/display/screenshot`.
**Why:** It returns raw binary framebuffer data via `fetch()` (bypassing `apiFetch()`), requires custom header reading (`X-Display-Width`, `X-Display-Height`), and the screenshot page is a standalone HTML file not part of the main SPA. User decision to defer.
