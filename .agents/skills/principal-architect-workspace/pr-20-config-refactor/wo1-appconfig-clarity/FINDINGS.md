# WO1 Findings — `appconfig.js` Scope Clarification

> Investigation by the WO1 worker (autonomous Sonnet); captured into this file by the
> Principal Architect because the harness blocks subagents from writing `.md` deliverables.
> No source modified; no stash dropped (pending user approval).

## 1. What `appconfig.js` does

Despite the generic name, `web/portal/appconfig.js` is **the Display tab controller**, not a
general app-config module. It is named after the **"App" tab** contributed by
`U8G2DisplayModule::getTabName()` (`modules/U8G2Display/U8G2DisplayModule.hpp:358` → `"App"`).

- **Endpoints:** exclusively `/api/display/*` — `GET /api/display/status`, `POST /api/display/brightness`, `/flip`, `/sleep` (`enabled`, `dimTimeout`, `blankTimeout`, `action: wake|sleep`), `/clockFontSize`. It never touches `/api/config`, `/api/system`, `/api/wifi`, `/api/ota`, `/api/mcp`, or `/api/time`.
- **Status:** **Active** — registered in `web/portal/app.js:406` (`'appconfig.js'` load entry); target `#tab-app` is generated because the display module contributes the "App" tab. Lazily inits on `tab-changed`, loads live data from `/api/display/status`. Fully wired, not dormant scaffolding.
- The filename is a **legacy misnomer**; functionally it's the Display settings panel.

## 2. Stash verdict — `stash@{1}` / `580f2f5` ("portal-tab-wip")

**REDUNDANT — fully superseded by landed commit `ef7be1e`. Safe to drop, pending user approval.**

- `git rev-parse stash@{1}` = `580f2f5…` (confirms index drift `stash@{2}`→`stash@{1}` as flagged).
- Stash touches `include/portal_assets/app_js.hpp` (stale gzip blob bump), `modules/WebServer/WebServerManager.cpp` (adds `appconfig_js.hpp` include + `/appconfig.js` route), `web/portal/app.js` (adds `'appconfig.js'` load entry).
- `git diff main HEAD -- web/portal/ include/portal_assets/appconfig_js.hpp modules/WebServer/WebServerManager.cpp` → **empty** (identical to main).
- Commit `ef7be1e` ("feat: implement App config tab and clockFontSize persistence", 2026-06-02) already introduced the **byte-for-byte identical** include, route registration, and `app.js` entry.
- Only "extra" content in the stash is a **stale** `app_js.hpp` gzip blob (pre-dating the Display-tab rewrite) — superseded build artifact, no remaining value.
- Origin: abandoned WIP from the `adhoc-app-tab-rules-bug` session, later re-implemented from scratch and merged via `ef7be1e`.

**Recommendation:** `git stash drop stash@{1}` — **only after explicit user approval.** Not dropped by the worker.

## 3. Per-field portal-exposure map

| Field | Exposed in portal? | Tab / File | WO2 implication |
|---|---|---|---|
| `hostname` | **No** | — | Pure C++/module migration (WO2a), no portal coordination |
| `timezone` | **Yes** | Time tab / `time.js` (`config.timezone`) | Already a persisted module key — portal edits the module value, not FrameworkConfig; verify `resetConfig()` default only |
| `firmwareUpdatesEnabled` | **No** | — | Pure C++/module migration (WO2b), no portal coordination |
| `otaQuietHour` | **No** | — | Pure C++/module migration (WO2b), no portal coordination |
| `mcpReadOnly` | **Yes** | MCP tab / `mcp.js` (`config.readOnly`, `#mcp-toggle-readonly`) | Already a persisted module key — portal edits the module value; verify secure `resetConfig()` default (`true`) only |

> **PA note:** `timezone` and `mcpReadOnly` are *already persisted module config* (confirmed in PROJECT.md current-state analysis). The portal edits the **module's** persisted value via REST→ConfigManager, **not** the `FrameworkConfig` boot default. So moving the default into `resetConfig()` does **not** change the portal contract — the only thing that changes is the factory-reset default source. The worker's "coordination needed" caveat reduces to *"ensure the `resetConfig()` defaults are sane"* — low risk, no portal code change expected.

## 4. Duplicate or complement?

**Complements — no duplication.** `appconfig.js` is scoped entirely to display settings via
`/api/display/*`; none of the five WO2-migrating fields appear in it. The "appconfig.js" / "app
config migration" naming collision is coincidental. **WO2 can proceed without touching
`appconfig.js`, `app.js` tab wiring, or `/api/display/*`.**

## 5. Native unit-test baseline — 🔴 RED (PA ran it; pre-existing breakage)

`pio test -e native` (PlatformIO 6.1.19) result: **215/219 cases pass, but 4 suites ERRORED**
(build failures, not assertion failures):
`test_application`, `test_mcpServer`, `test_otaUpdateManager`, `test_u8g2Display`.

**All 4 share one root cause** — two compile errors in `OTAUpdateManager` (which every errored
suite links):

```
modules/OTAUpdateManager/OTAUpdateManager.hpp:426:35:
    error: unknown type name 'String'; did you mean 'std::string'?
modules/OTAUpdateManager/OTAUpdateManager.cpp:926:24:
    error: out-of-line definition of 'handleSignedUpload' does not match any declaration
```

1. Arduino `String` type used in a header compiled under the native (host) build, where `String`
   is not in scope.
2. `handleSignedUpload` definition signature drifted from its declaration.

**Provenance:** pre-existing on `main` — WO1 made no source changes and PR-20's planning commits
touched only docs/symlinks. Likely from in-flight OTA work (`pr-16-backup-overhaul` is WIP and
touches OTA `IConfigurable`).

**⚠️ Blocking precondition for WO2:** the baseline must be **green before WO2a starts**, otherwise
WO2's "all tests pass" gate is meaningless and WO2-introduced regressions can't be distinguished
from this pre-existing red. Recommend a small **pre-flight fix** (WO0) to repair these two
`OTAUpdateManager` compile errors and restore a green native build first.

## Verification

`git status --short` clean during/after investigation — no tracked source modified.
