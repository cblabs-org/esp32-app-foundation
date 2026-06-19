# PR-30: Attribution Cleanup & Provenance Remediation

**Goal:** Remove all third-party attribution risk from the codebase prior to
open-source release. Eliminate any reference to the `departures-board` project
and replace the one component with credible derivation risk (`MD5Utils`
`base64ToHex`) with a clean implementation using the platform `mbedtls` API.

**Context:** A full provenance analysis (conversation `87bf8a4b`) identified:
- 1 Gadec copyright header (test file)
- 7 source-code comments referencing `departures-board`
- 1 CSS comment referencing `departures-board`
- 1 function (`base64ToHex`) recognisably derived from gadec's CC BY-NC-SA 4.0 code
- ~50+ references in `.agents/` planning docs (internal, not shipped)

**Gadec's licence:** CC BY-NC-SA 4.0 (Non-Commercial, Share-Alike).
The user is NOT gadec — gadec is a separate author (Brian).

---

## Work Order Ledger

- **[x] [wo1-comment-cleanup](wo1-comment-cleanup/WORKORDER.md):** Remove all
  `departures-board` / `gadec` references from shipped source files. Fix the
  Gadec copyright header on test_main.cpp. 9 edits across 7 files. ✅ Merged `5cde008`
- **[x] [wo2-md5utils-rewrite](wo2-md5utils-rewrite/WORKORDER.md):** Replace the
  hand-rolled `base64ToHex` function with `mbedtls_base64_decode()`. Rename
  `calculateFileMD5` local variables to diverge from gadec naming. Verify
  native + esp32 builds pass. ✅ Merged `308bbab`
- **[x] [wo3-credits-tab](wo3-credits-tab/WORKORDER.md):** Add a new top-level
  Credits tab to the web portal listing all open-source dependencies with
  licence + links. Stylised © icon. New `credits.js` fragment + routes. ✅ Merged `b3c98cd`

---

## Concurrency Analysis

WO1 touches 7 source files (comments only). WO2 touches `lib/MD5Utils/` only.
WO3 touches `web/portal/` + `WebServerManager.cpp` + `index.html` + `app.js`.
**No file overlap between any pair.** Safe to parallelise, but serialisation
is simpler.

**Recommended:** Serialise (WO1 → WO2 → WO3).


---

## Verification

- `pio run -e esp32dev` — firmware builds
- `pio test -e native` — all native tests pass
- `grep -ri "departures" --include="*.cpp" --include="*.hpp" --include="*.h" --include="*.css" --include="*.js" --include="*.html" .` — zero hits
- `grep -ri "gadec" --include="*.cpp" --include="*.hpp" --include="*.h" --include="*.css" --include="*.js" --include="*.html" .` — zero hits
