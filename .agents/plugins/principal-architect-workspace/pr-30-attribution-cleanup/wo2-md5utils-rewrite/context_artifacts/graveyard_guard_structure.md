---
title: "WO2: MD5Utils Rewrite — Graveyard"
---

# Graveyard: Failed Approaches and Pitfalls

## 1. `#ifdef PIO_UNIT_TESTING` early-return guard (FAILED)

**What was tried:** Initially placed the native mock at the top of
`base64ToHex()` using `#ifdef PIO_UNIT_TESTING` with an early `return`, followed
by the mbedtls code below it (unguarded).

**Why it failed:** The compiler still parses all code in the function body even
after an `#ifdef`-guarded early return. The `mbedtls_base64_decode()` calls and
`MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL` constant below the guard produced
"undeclared identifier" errors in native builds.

**Fix:** Restructured to `#ifndef PIO_UNIT_TESTING` / `#else` / `#endif` so the
two code paths are mutually exclusive compilation units.

## 2. Fixed mock breaks existing tests (EXPECTED)

**What happened:** The WO-prescribed fixed mock return
(`"d41d8cd98f00b204e9800998ecf8427e"`) caused 4 test failures:
- `Base64ToHexKnownVector` — expected a specific hash, got the mock
- `Base64ToHexEmptyInput` — expected empty string, got the mock
- `Base64ToHexInvalidBase64` — expected empty string, got the mock
- `MultipleBase64ToHexCalls` — expected two different hashes, got same mock

**Resolution:** The caller expanded the boundary box to include the test file.
All 4 `base64ToHex` test cases were wrapped in `#ifndef PIO_UNIT_TESTING` guards
so they're compiled out of native builds (where the mock makes them meaningless)
but remain active for hardware test runs.

**Important:** When restoring the `CalculateFileMD5Nonexistent` test after the
guard insertion, care was needed to keep it *outside* the guard since it tests
`calculateFileMD5`, not `base64ToHex`.
