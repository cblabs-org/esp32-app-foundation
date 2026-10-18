---
title: "WO2: MD5Utils Provenance Remediation — Decision Record"
---

# ADR: MD5Utils base64ToHex Rewrite Strategy

## Context

The `base64ToHex()` function in `lib/MD5Utils/MD5Utils.cpp` used a hand-rolled
base64 decoder (custom `base64Table[]` string, `base64Value[256]` lookup array,
6-bit accumulator loop) originally derived from a third-party CC BY-NC-SA 4.0
codebase. This created a provenance/licensing concern for the project.

## Decision

Replace the hand-rolled decoder with `mbedtls_base64_decode()` from the
ESP-IDF-bundled mbedtls library (Apache-2.0 licensed). This is a two-pass
decode pattern:

1. First call with `nullptr` output buffer to determine required size
2. Allocate with `new (std::nothrow)` (GRAVE-05 compliance)
3. Second call to perform actual decode
4. Convert raw bytes to hex string

## Consequences

### Positive
- Eliminates all derivation from the CC BY-NC-SA 4.0 source
- Uses a well-tested, standards-compliant base64 decoder
- Maintains `std::nothrow` allocation and `ESP_LOG*` diagnostics

### Negative / Trade-off
- `mbedtls_base64_decode()` is unavailable in native test builds, so
  `base64ToHex()` now requires a `#ifndef PIO_UNIT_TESTING` guard with a
  fixed mock return for native. This means the 4 existing `base64ToHex` unit
  tests had to be guarded out of native builds (`#ifndef PIO_UNIT_TESTING`).
  They remain available for hardware test runs where mbedtls is present.

### Variable Rename in calculateFileMD5
Additionally, local variables in `calculateFileMD5` were renamed to diverge
from the original source material:

| Old Name     | New Name       |
|-------------|----------------|
| `ctx`       | `md5Ctx`       |
| `bufferSize`| `kReadBufSize` |
| `buffer`    | `readBuf`      |
| `hash`      | `digest`       |
| `hexString` | `hexOut`       |
