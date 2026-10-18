# ADR: Build System Certificate Bundle Fix

## Decision

Switched from `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=n` (NONE) to `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_CMN=y` (Common CAs) in `sdkconfig.defaults`.

## Context

The previous session's WO1 set `CERTIFICATE_BUNDLE=n` to minimize flash usage. This worked in the build environment at the time, but a subsequent ESP-IDF platform update (or build cache invalidation) exposed a latent cmake bug:

When `CERTIFICATE_BUNDLE=n`, the ESP-IDF build system's `target_add_binary_data()` cmake function fails to generate the expected `.S` assembly stubs for the cert bundle. This cascading failure breaks *unrelated* components that also use `target_add_binary_data()` (e.g., `esp_https_server`'s self-signed cert DER files).

**Root cause chain:**
1. `CERTIFICATE_BUNDLE=n` → `gen_crt_bundle.py` not invoked → no `.S` file for cert data
2. cmake's `target_add_binary_data()` state is corrupted for the build target
3. Other components calling `target_add_binary_data()` inherit the broken state
4. Build fails with `crt.S: No such file or directory` errors

## Alternatives Rejected

- **`DEFAULT_FULL`**: ~100 Mozilla root CAs, ~27 KB flash. Overkill for this project.
- **`CERTIFICATE_BUNDLE=n` + manual `.S` stubs**: Attempted via `gen_cert_asm.py` script — worked temporarily but fragile and non-portable.
- **Stub header `esp_crt_bundle.h`**: Only suppresses compile errors, doesn't fix the cmake pipeline.

## Outcome

`DEFAULT_CMN` provides ~35 common CAs in ~14 KB flash — a good tradeoff. The cmake pipeline works correctly because the cert bundle generation step runs normally, producing valid `.S` files.
