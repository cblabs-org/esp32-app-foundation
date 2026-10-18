# PR-29 — TLS Memory Optimization

**Branch:** `pr-29-tls-optimization`
**Base:** `main`
**Status:** 🔄 WIP — WO4 in progress
**Implementation Plan:** [implementation_plan.md](../../../../.gemini/antigravity/brain/07cb3857-55da-4085-b60a-2fecec6a6dec/implementation_plan.md)
**Analysis:** [tls_optimization_analysis.md](../../../../.gemini/antigravity/brain/07cb3857-55da-4085-b60a-2fecec6a6dec/tls_optimization_analysis.md)

---

## Objective

Reduce per-connection TLS heap footprint (~18–23 KB idle savings) and flash usage
(~64–74 KB) by applying mbedTLS configuration optimizations in `sdkconfig.defaults`.
Also add a curated OTA CA bundle with graceful fallback for transport security.

---

## Work Order Ledger

| WO | Topic | Complexity | Status | Dependencies |
|---|---|---|---|---|
| **[x] [wo1-sdkconfig-changes](wo1-sdkconfig-changes/WORKORDER.md)** | Apply TLS optimisation settings to `sdkconfig.defaults` | `mechanical` | ✅ MERGED `268218c` | none |
| **[x] [wo2-build-test-measure](wo2-build-test-measure/WORKORDER.md)** | Build esp32s3nano, measure heap/flash before & after, run OTA + HTTPS portal + GitHub API regression tests | `standard` | ✅ MERGED `5e000e5` — [RESULTS](wo2-build-test-measure/RESULTS.md) | WO1 |
| **[x] [wo3-skill-documentation](wo3-skill-documentation/WORKORDER.md)** | Write `tls-memory-optimization.md` reference, update framework-expert SKILL.md | `standard` | ✅ MERGED `9366eef` | WO2 |
| **[ ] [wo4-ota-ca-pinning](wo4-ota-ca-pinning/WORKORDER.md)** | Create dual-CA `ota_root_cas.h`, add graceful fallback in OTAUpdateManager | `standard` | 🔄 IN PROGRESS | WO2 |

### Sequencing

```
WO1 (config) → WO2 (build+test+measure) → WO3 (documentation)
                                         → WO4 (OTA CA pinning)
```

WO3 and WO4 are independent of each other; both depend on WO2.

---

## Key Design Decisions

1. **D1 — Cert bundle NONE**: Default to `NONE` (0 built-in CAs). Framework connections
   use pinned CAs or `setInsecure()`. Downstream devs override via board delta files.
2. **D2 — Dynamic buffers**: `CONFIG_MBEDTLS_DYNAMIC_BUFFER=y` — frees TX/RX when idle.
   Savings apply to steady-state (portal idle), not during active OTA transfer.
3. **D3 — Error strings kept**: `CONFIG_MBEDTLS_ERROR_STRINGS` stays enabled (flash, not heap).
4. **D4 — Curated OTA CA bundle**: Dual-CA header (`ota_root_cas.h`) with Sectigo E46
   (GitHub API) + ISRG Root X1 (GitHub CDN). ~2.5 KB flash. Graceful fallback to
   `setInsecure()` if CA validation fails — firmware signature is the trust anchor.
5. **D5 — CDN uses different CA**: `objects.githubusercontent.com` uses Let's Encrypt,
   not Sectigo. Discovered during OTA testing — the Sectigo-only pinning broke
   manifest downloads when cert bundle was set to NONE.

---

*Created: 2026-06-18*
