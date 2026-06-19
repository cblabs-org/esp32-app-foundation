---
title: "WO2: MD5Utils Provenance Remediation — Context Bridge"
distilled_at: "2026-06-19T17:50:00Z"
original_plan_id: "pr-30-attribution-cleanup"
artifacts:
  - context_artifacts/adr_mbedtls_rewrite.md
  - context_artifacts/graveyard_guard_structure.md
---

# WO2: MD5Utils Provenance Remediation — Context Bridge

## Executive Summary

WO2 replaced the hand-rolled `base64ToHex()` decoder in MD5Utils with
`mbedtls_base64_decode()` (Apache-2.0) and renamed `calculateFileMD5` local
variables to eliminate all derivation from a third-party CC BY-NC-SA 4.0
codebase. All builds pass (esp32dev + native, 560/560 tests).

## Files Modified

- `lib/MD5Utils/MD5Utils.cpp` — rewritten `base64ToHex`, renamed locals, added `mbedtls/base64.h` include
- `lib/MD5Utils/MD5Utils.hpp` — removed "MD5 hash" and "GitHub headers" from documentation
- `test/test_unit/test_md5Utils/test_md5Utils.cpp` — guarded `base64ToHex` tests out of native builds

## Next Steps

- No further action required for WO2 — all checklist items complete and verified.
- The PR-30 parent plan may have additional work orders to execute.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_mbedtls_rewrite.md` — Decision record for the mbedtls rewrite strategy and variable rename rationale
- `context_artifacts/graveyard_guard_structure.md` — Failed `#ifdef` early-return approach and test breakage resolution
