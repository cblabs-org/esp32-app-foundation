---
name: portal-consumer-assets-wo2
description: Handoff instructions for Work Order 2 — sample-app migration + docs teaching the gzip pipeline as canonical
complexity: standard
---

# WO2 — Docs and Sample

**Parent project:** `pr-portal-consumer-assets` (#111). Human-approved
decomposition (see PROJECT.md §Human Approval). Base: `wo1`.

> **Execution note:** WO2 was executed **directly by the PA**, not a spawned
> worker. Rationale recorded in WO1's Dead Ends: the sonnet worker repeatedly
> parked itself during long serialized ESP-IDF builds, producing notification
> churn. WO2 is mostly documentation plus one small module migration, so direct
> PA execution was faster and more reliable than another background worker.

### Objective
Make the sample app the reference consumer demonstration of #111, and rewrite the
portal docs so the gzip+PROGMEM pipeline is taught as canonical (raw-string kept
as a documented secondary for tiny fragments).

### Line-Item Instructions (all complete)
- [x] `sample-app/web/weather.js` (NEW) — extracted the ~16 KB tab script
      verbatim from the `R"rawliteral"` block in WeatherModule.cpp.
- [x] `sample-app/src/WeatherModule.cpp` — `#include "web/serveGzipAsset.hpp"` +
      `#include "portal_assets/weather_js.hpp"`; replaced the raw-string
      `/weather.js` route with `serveGzipAsset(response, weather_js_gz,
      weather_js_gz_len, "application/javascript")`; removed the ~300-line
      literal.
- [x] `.agents/skills/framework-expert/systems/web-portal/html-js-fragments.md` —
      rewritten: gzip pipeline canonical (module + consumer workflow, collision
      note, output-dir note), raw-string demoted to secondary (<~1 KB) with DRAM
      warning. (`.claude/skills/` is a symlink to `.agents/skills/`; the tracked
      path is `.agents/skills/`.)
- [x] `.agents/skills/framework-expert/systems/web-portal/overview.md` —
      `registerRoutes()` bullet no longer says "raw-string literal"; points to the
      pipeline.
- [x] `docs/developer/extending-the-web-portal.md` — added canonical
      gzip-pipeline callout to §4 (JavaScript Fragment Pattern).

### Verification
- Sample-app build (`esp32dev`, via platformio MCP) reached and **compiled
  `WeatherModule.cpp.o` cleanly** against the generated header, and
  `scripts/portalBuilder.py` emitted `include/portal_assets/weather_js.hpp`
  (24 KB, gzip of `sample-app/web/weather.js`, correctly gitignored). This proves
  the #111 consumer pipeline end-to-end: consumer `web/` scanned → gzipped →
  included → served via helper.
- The sample-app `esp32dev` **link** did not complete due to a **pre-existing,
  unrelated** failure (see Out-of-Scope Findings).

---
## Completion Report

### Status
complete (for #111 scope; sample-app full-link blocked by a pre-existing unrelated issue)

### Changes Made
- (as per Line-Item Instructions above) — commit `9eb3c081`.

### Decisions Made
- Docs scope deliberately bounded: fully fixed the two issue-named skill docs +
  added the canonical path to the developer doc, rather than rewriting the whole
  530-line developer doc — its §3/§4 also teach a *different* legacy pattern
  (LittleFS `/www/<ns>/tab.html`+`tab.js`) that contradicts the current
  architecture. Reconciling that is a separate docs effort (surfaced below).
- Executed by PA rather than a worker (see Execution note).

### Dead Ends & Warnings
- None specific to WO2. (See WO1 for the worker-parking pattern.)

### Out-of-Scope Findings (for PA triage)
1. **Sample-app `esp32dev`/`esp32s3nano` has a pre-existing build break,
   unrelated to #111.** `sample-app/src/WeatherBoard.hpp:33` includes
   `"U8G2Display/IDisplayBoard.hpp"`, which requires `../modules` on the include
   path, but `sample-app/platformio.ini` `build_flags` only add
   `-I ../include -I ../hardware -I ../hardware/_common -I ../` — **no
   `-I ../modules`**. Files involved (`WeatherBoard.*`, `main.cpp`) are byte-
   identical to `main` (last changed `b220e1df`), and `WeatherModule.cpp`
   compiled fine, so #111 changes are exonerated. Likely-correct fix: add
   `-I ../modules` to the sample-app `env:base`/`env:native` `build_flags` (low
   risk — `../modules` is already a `lib_extra_dirs`). NOT applied here — outside
   #111 scope; recommend a separate small fix/PR or user sign-off before bundling.
2. **Developer doc `extending-the-web-portal.md` is broader-stale.** §3/§4 teach a
   LittleFS `/www/<namespace>/tab.html`+`tab.js` fragment-serving model that
   contradicts the skill's own `overview.md` ("the web portal does not load
   external HTML fragments"). A focused reconciliation pass (align the whole doc
   with the C++-route + gzip-pipeline reality) is warranted as its own docs issue.
3. **Shared `include/portal_assets/` output dir is cross-consumer global**
   (carried from WO1). Documented as a NOTE in the rewritten skill doc; a
   per-consumer namespacing option could be a future enhancement.

### Blockers
(none for #111 scope)
