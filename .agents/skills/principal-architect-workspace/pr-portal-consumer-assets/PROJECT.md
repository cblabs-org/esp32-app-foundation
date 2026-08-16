# Project: pr-portal-consumer-assets

> Close the consumer-facing gap in the portal asset pipeline (issue #111).
> Consuming apps that implement `IWebContributor` with a tab script are forced
> into the raw-string-literal serve pattern, which copies the whole body into
> internal DRAM (Wiser: `heap_min_free` → 136 bytes, 8s serve, intermittent
> `0 bytes received`). The framework enjoys a gzip+PROGMEM pipeline its
> consumers cannot reach. This project makes that pipeline symmetric across
> three layers: **build** (scan the consuming project's `web/`), **serve**
> (a shared `serveGzipAsset()` helper, adopted framework-wide), and **docs**
> (teach the pipeline as canonical; keep raw-string as a documented secondary
> for tiny inline fragments). Closes #111.

## Relationship to prior work

Direct follow-on to **pr-portal-asset-decentralisation** (PR #109, merged
2026-08-14, closing #96), which decentralised *module* web assets through this
same pipeline but stopped at the framework boundary. #111 is the consumer-facing
gap that project left. Its route-decentralisation is what spread the hand-rolled
3-line serve incantation across 7 modules — so the shared helper here is a
natural cleanup of debt that project introduced, not new debt.

## Human Approval

- [x] Decomposition approved by human: 2026-08-16 — user granted explicit
  approval to "proceed autonomously, build and test all environments and then
  stand a draft PR for review." Scope confirmed earlier the same session:
  all three layers; raw-string retained as documented secondary; PA
  decomposition; lightweight-header helper (`include/web/serveGzipAsset.hpp`).

## Design Decisions (pre-agreed with human)

- **Helper home:** new lightweight header `include/web/serveGzipAsset.hpp` with
  an inline free function — low coupling; asset-serving modules do not need to
  depend on the full `WebServerManager` header. (Chosen over a static method.)
- **Raw-string pattern:** retained in docs as a secondary option for small
  (<~1 KB) inline fragments, with a DRAM warning; gzip pipeline is canonical.
- **Output dir unchanged:** generated assets still land in
  `include/portal_assets/` (already on the framework include path; sample-app
  reaches it via `-I ../include`).
- **PR shape:** a single draft PR (not a 3-deep stack) — the two WOs are
  stacked internally for execution isolation; the tip is submitted as one PR.

## Work Order Ledger

| WO | Branch | Status | Summary |
|----|--------|--------|---------|
| **[x] [wo1-pipeline-mechanism](wo1-pipeline-mechanism/WORKORDER.md)** | `portal-consumer-assets/wo1-pipeline-mechanism` (base: `main`) | ✅ Complete (`ff841372`) | portalBuilder.py scans `$PROJECT_DIR/web`; new `include/web/serveGzipAsset.hpp`; 7 modules migrated (net −50 lines). native 662/664, esp32dev_ha + crowpanel compile clean. |
| **[ ] [wo2-docs-and-sample](wo2-docs-and-sample/WORKORDER.md)** | `portal-consumer-assets/wo2-docs-and-sample` (base: `wo1`) | ⏳ Pending | Migrate sample-app WeatherModule to the pipeline (`sample-app/web/weather.js` + serveGzipAsset); rewrite portal docs (both locations) + skill fragments to teach the pipeline as canonical, raw-string as secondary. |

## Conflict / Concurrency Analysis

WO1 touches `scripts/portalBuilder.py`, `include/web/serveGzipAsset.hpp` (new),
and 7 framework module `.cpp` files. WO2 touches `sample-app/**`, `docs/**`, and
skill markdown. **No file overlap** — a clean linear stack. WO2 depends on WO1
only semantically (needs the helper + scan to exist and compile).

## Verification Strategy (PA, at tip)

Per human instruction ("build and test all environments"), after both WOs land,
the PA runs a full build/test sweep via the `platformio` MCP:
- **Framework root** (`platformio.ini`): `native` + `native_ha` (unit tests),
  and compile `esp32dev`, `esp32dev_ha`, `esp32s3nano`, `crowpanel-128-rotary`.
- **Sample-app** (`sample-app/platformio.ini`): compile `esp32dev`,
  `esp32s3nano` — this is the true end-to-end #111 validation (consumer `web/`
  scanned → `weather_js.hpp` generated → served via helper).
No hardware flash required; no new firmware functionality → no version tag.

## Session Log

### Session 1 — 2026-08-16 (Decomposition)
- Traced full pipeline: build (`scripts/portalBuilder.py:34-44`, framework-only
  scan), serve (23 hand-rolled `setContent`+`Content-Encoding` sites across 7
  modules; no shared helper), docs (`docs/developer/extending-the-web-portal.md`
  + skill `systems/web-portal/*` canonise the DRAM-heavy raw-string pattern).
- Confirmed sample-app is a live downstream consumer (`sample-app/platformio.ini`
  runs `pre:../scripts/portalBuilder.py`; WeatherModule serves `/weather.js` as a
  raw literal) — the natural end-to-end validation target for #111.
- Established follow-on relationship to pr-portal-asset-decentralisation (#96).
- Decomposed into 2 stacked WOs (mechanism → docs+sample), single draft PR.
