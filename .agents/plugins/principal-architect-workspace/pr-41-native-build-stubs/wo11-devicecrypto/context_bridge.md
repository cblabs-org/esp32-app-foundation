---
title: DeviceCrypto Native Build Stubs Migration
distilled_at: 2026-07-01T07:49:00Z
original_plan_id: pr41-wo11
artifacts:
  - context_artifacts/adr_devicecrypto_native_stubs.md
---

### Executive Summary
Removed all `#ifdef NATIVE_BUILD` guards from the `DeviceCrypto` library and extended the native mbedTLS and Preferences platform stubs to support linker requirements for test builds. Both native and `esp32s3nano` targets compile successfully.

### Next Steps
- Verify the status update in `WORKORDER.md`.
- Proceed to the next work order in the `pr-41-native-build-stubs` stack.

### Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_devicecrypto_native_stubs.md` - ADR explaining the removal of inline compilation guards and reliance on platform stubs.
