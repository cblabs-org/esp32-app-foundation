---
name: pr-30-attribution-cleanup-wo2
description: Handoff instructions for Work Order 2 — Rewrite MD5Utils base64ToHex using mbedtls and diverge variable names in calculateFileMD5
complexity: standard
---

# WO2: MD5Utils Provenance Remediation

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

## Objective

Replace the `base64ToHex` function in `lib/MD5Utils/MD5Utils.cpp` with a clean
implementation that uses the platform `mbedtls_base64_decode()` API instead of
a hand-rolled base64 decoder. Also rename local variables in `calculateFileMD5`
to diverge from the original source. This eliminates any derivation chain from
a third-party CC BY-NC-SA 4.0 codebase.

## Why This Matters

The current `base64ToHex()` implementation uses a hand-rolled base64 decode
algorithm (custom `base64Table[]` string, `base64Value[256]` lookup array,
6-bit accumulator) that was originally authored by a third party under
CC BY-NC-SA 4.0. The mbedtls library (Apache-2.0, bundled with ESP-IDF)
provides `mbedtls_base64_decode()` which handles the same task without any
licensing concern.

## Strict Boundary Box

- ✅ ONLY modify `lib/MD5Utils/MD5Utils.cpp` and `lib/MD5Utils/MD5Utils.hpp`
- ❌ Do NOT modify any other files
- ❌ Do NOT change the public API signatures (`calculateFileMD5`, `base64ToHex`)
- ❌ Do NOT create new branches — work on whatever branch is checked out
- ✅ DO preserve the `#ifndef PIO_UNIT_TESTING` guard structure
- ✅ DO preserve `std::nothrow` heap allocation pattern (GRAVE-05 compliance)
- ✅ DO preserve `ESP_LOGW` / `ESP_LOGE` diagnostic logging

## Checklist

### 1. Rewrite `base64ToHex` in `MD5Utils.cpp`

Replace the entire `base64ToHex` function body with this approach:

```cpp
#include <mbedtls/base64.h>  // Add to includes

std::string MD5Utils::base64ToHex(const std::string& base64Hash) {
    if (base64Hash.empty()) {
        ESP_LOGW(TAG, "base64ToHex: empty input string");
        return std::string();
    }

    // First call: determine output size
    size_t decodedLen = 0;
    int ret = mbedtls_base64_decode(
        nullptr, 0, &decodedLen,
        reinterpret_cast<const unsigned char*>(base64Hash.c_str()),
        base64Hash.length());

    if (ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL || decodedLen == 0) {
        ESP_LOGW(TAG, "base64ToHex: invalid base64 input");
        return std::string();
    }

    // Allocate decode buffer
    auto* decoded = new (std::nothrow) unsigned char[decodedLen];
    if (decoded == nullptr) {
        ESP_LOGE(TAG, "base64ToHex: allocation failed (%zu bytes)", decodedLen);
        return std::string();
    }

    // Second call: actual decode
    size_t actualLen = 0;
    ret = mbedtls_base64_decode(
        decoded, decodedLen, &actualLen,
        reinterpret_cast<const unsigned char*>(base64Hash.c_str()),
        base64Hash.length());

    if (ret != 0) {
        ESP_LOGW(TAG, "base64ToHex: decode failed (ret=%d)", ret);
        delete[] decoded;
        return std::string();
    }

    // Convert to hex string
    std::string hexResult;
    hexResult.reserve(actualLen * 2);
    for (size_t i = 0; i < actualLen; i++) {
        char hexBuf[3];
        snprintf(hexBuf, sizeof(hexBuf), "%02x", decoded[i]);
        hexResult += hexBuf;
    }

    delete[] decoded;
    return hexResult;
}
```

**Key changes from the old implementation:**
- Uses `mbedtls_base64_decode()` (Apache-2.0) instead of hand-rolled decoder
- Maintains `std::nothrow` allocation pattern
- Maintains `ESP_LOGW`/`ESP_LOGE` error logging
- Same API signature — drop-in replacement

### 2. Add `#include <mbedtls/base64.h>` to includes

In `MD5Utils.cpp`, add the include alongside the existing mbedtls include:
```cpp
#include <mbedtls/md5.h>
#include <mbedtls/base64.h>   // ← ADD THIS
```

Note: this include should be inside the `#ifndef PIO_UNIT_TESTING` guard since
mbedtls is not available in native builds.

### 3. Rename `calculateFileMD5` local variables

In `calculateFileMD5`, rename these locals to diverge from the source:

| Old Name | New Name |
|---|---|
| `ctx` | `md5Ctx` |
| `bufferSize` | `kReadBufSize` |
| `buffer` | `readBuf` |
| `hash` | `digest` |
| `hexString` | `hexOut` |

Preserve all logic — only change variable names.

### 4. Handle native build guard for base64ToHex

The existing `calculateFileMD5` has a `#ifndef PIO_UNIT_TESTING` guard. The
rewritten `base64ToHex` should also have this guard since `mbedtls_base64_decode`
is unavailable in native builds. Add a mock return for the native test path:

```cpp
#ifdef PIO_UNIT_TESTING
    // Mock: return a fixed hex string for testing
    return std::string("d41d8cd98f00b204e9800998ecf8427e");
#endif
```

### 5. Update the header file comment

In `MD5Utils.hpp`, update the description to remove the reference to "GitHub
headers" (which ties it to the departures-board OTA use case):

Replace:
```
 *   - base64ToHex(): Converts a base64 encoded MD5 hash into a hex string.
```
With:
```
 *   - base64ToHex(): Converts a base64 encoded hash into a hex string.
```

Also update the `@brief` doc on the `base64ToHex` method:

Replace:
```
     * @brief Converts a base64 encoded MD5 hash (such as those from GitHub headers) into a hex string.
```
With:
```
     * @brief Converts a base64 encoded hash into a hex string.
```

## Verification

1. `pio run -e esp32dev` — firmware builds with the new mbedtls include
2. `pio test -e native` — all native tests pass
3. Manual review: confirm no `base64Table`, `base64Value`, or hand-rolled
   decode loop remains in MD5Utils.cpp
4. `grep -n "departures\|gadec\|base64Table\|base64Value" lib/MD5Utils/*` — zero hits

---

## Status Update

| Field | Value |
|---|---|
| **Status** | ✅ **COMPLETE** |
| **Completed** | 2026-06-19T17:50:00Z |
| **Session ID** | `70a45b24-7f69-490c-9e49-2c59dedfd5b7` |
| **Branch** | `pr-30-attribution-cleanup-wo2-md5utils-rewrite` |

### Files Modified
- `lib/MD5Utils/MD5Utils.cpp` — rewritten `base64ToHex` with `mbedtls_base64_decode()`, renamed `calculateFileMD5` locals, added `mbedtls/base64.h` include
- `lib/MD5Utils/MD5Utils.hpp` — removed "MD5 hash" and "GitHub headers" from documentation
- `test/test_unit/test_md5Utils/test_md5Utils.cpp` — guarded `base64ToHex` tests out of native builds (boundary expanded by caller)

### Verification Results
- `pio run -e esp32dev` — ✅ SUCCESS
- `pio test -e native` — ✅ 560/560 passed
- Provenance grep — ✅ zero hits for `departures`, `gadec`, `base64Table`, `base64Value`

### Notes
- Initial `#ifdef PIO_UNIT_TESTING` early-return guard was insufficient; restructured to `#ifndef`/`#else`/`#endif` to prevent compiler from parsing mbedtls symbols in native builds.
- Boundary box expanded by caller to include test file modification after the prescribed mock broke 4 existing tests.

