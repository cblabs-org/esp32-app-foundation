# PR-29 — TLS Memory Optimization

**Branch:** `pr-29-tls-optimization`
**Base:** `main`
**Status:** ✅ COMPLETE — All 3 WOs merged, ready for PR submission
**Implementation Plan:** [implementation_plan.md](../../../../.gemini/antigravity/brain/07cb3857-55da-4085-b60a-2fecec6a6dec/implementation_plan.md)
**Analysis:** [tls_optimization_analysis.md](../../../../.gemini/antigravity/brain/07cb3857-55da-4085-b60a-2fecec6a6dec/tls_optimization_analysis.md)

---

## Objective

Reduce per-connection TLS heap footprint (~18–23 KB idle savings) and flash usage
(~64–74 KB) by applying mbedTLS configuration optimizations in `sdkconfig.defaults`.
All changes are config-only — zero source code modifications.

---

## Work Order Ledger

| WO | Topic | Complexity | Status | Dependencies |
|---|---|---|---|---|
| **[x] [wo1-sdkconfig-changes](wo1-sdkconfig-changes/WORKORDER.md)** | Apply TLS optimisation settings to `sdkconfig.defaults` | `mechanical` | ✅ MERGED `268218c` | none |
| **[x] [wo2-build-test-measure](wo2-build-test-measure/WORKORDER.md)** | Build esp32s3nano, measure heap/flash before & after, run OTA + HTTPS portal + GitHub API regression tests | `standard` | ✅ MERGED `5e000e5` — [RESULTS](wo2-build-test-measure/RESULTS.md) | WO1 |
| **[x] [wo3-skill-documentation](wo3-skill-documentation/WORKORDER.md)** | Write `tls-memory-optimization.md` reference, update framework-expert SKILL.md | `standard` | ✅ MERGED `9366eef` | WO2 |

### Sequencing

```
WO1 (config) → WO2 (build+test+measure) → WO3 (documentation)
```

All WOs are serialised. WO3 depends on WO2's measured results.

---

## Key Design Decisions

1. **D1 — Cert bundle NONE**: Default to `NONE` (0 built-in CAs). Framework connections
   use pinned CAs or `setInsecure()`. Downstream devs override via board delta files.
2. **D2 — Dynamic buffers**: `CONFIG_MBEDTLS_DYNAMIC_BUFFER=y` — frees TX/RX when idle.
   Savings apply to steady-state (portal idle), not during active OTA transfer.
3. **D3 — Error strings kept**: `CONFIG_MBEDTLS_ERROR_STRINGS` stays enabled (flash, not heap).
4. **D4 — S3 CA pinning compatibility**: The `NONE` + `setCACert()` pattern is
   complementary to future OTA security work (pinning Amazon Root CA 1 for CDN).

---

*Created: 2026-06-18*
