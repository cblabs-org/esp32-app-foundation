---
title: Boot & WiFi Bug Investigation — Three Fixes on pr-21
distilled_at: 2026-06-11T22:13:00+01:00
original_plan_id: standalone
branch: pr-21-ota-task-hardening
artifacts:
  - context_artifacts/adr_pkcs7_padding.md
  - context_artifacts/adr_wifi_wizard_modal.md
  - context_artifacts/graveyard_debugging.md
---

# Boot & WiFi Bug Investigation — Three Fixes on pr-21

## Executive Summary

A serial monitor debugging session on 2026-06-11 identified and fixed three bugs on
`pr-21-ota-task-hardening`. The device was booting but failing to connect to WiFi
(`AUTH_EXPIRE` on every attempt) and not showing the WiFi wizard modal in AP mode.
All three issues were fixed, tested on-device, and confirmed working — the ESP32 now
connects to `cb-iot` on first attempt (192.168.20.186).

## What Was Done

### Bug 1 — PKCS7 Padding Corruption (CRITICAL) ✅ Fixed
- **File:** `lib/DeviceCrypto/DeviceCrypto.cpp` line 101
- **Commit:** `dff3b22`
- **Fix:** `return paddedLen` → `return 0` in the invalid-padding fallback branch
- **Effect:** Decrypted WiFi passwords were silently padded with 1–15 garbage bytes,
  causing `AUTH_EXPIRE` on every connection attempt. Fix verified by on-device test:
  `auth → assoc → run` on first attempt, device appeared on UniFi at 192.168.20.186.

### Bug 2 — WiFi Wizard Modal Never Shown in AP Mode ✅ Fixed
- **Files:** `modules/Application/Application.cpp`, `modules/Application/Application.hpp`
- **Commit:** `a0c91dc`
- **Fix:** Added AP-mode detection in the CONFIGURING tick loop; pushes
  `wifiWizard` modal (hostname + "192.168.4.1") once via `_wifiWizardShown` guard.
  Flag reset in `_onEnterRunning()` for regression recovery.
- **Key insight:** `_onEnterConfiguring()` fires too early (before WifiManager::begin()).
  AP mode is detected asynchronously ~8s later — must be polled in the tick loop.

### Bug 3 — `%zu` Logging as Literal "zu" ✅ Fixed
- **File:** `lib/DeviceCrypto/DeviceCrypto.cpp` (9 occurrences)
- **Commit:** `a0c91dc`
- **Fix:** All `%zu` replaced with `%u` + explicit `(unsigned)` casts.
- **Cause:** Newlib nano (ESP-IDF) does not support the `%z` length modifier.

## Current Branch State

```
pr-21-ota-task-hardening
  dff3b22  fix(crypto): return 0 on invalid PKCS7 padding byte instead of paddedLen
  a0c91dc  fix(display,crypto): wire WiFi wizard modal + fix %zu printf format
```

Device is connected and running. No outstanding issues from this session.

## Next Steps

These fixes are on the PR-21 stack. They are independent of the OTA task hardening
work (the original PR-21 scope) and should be clean to merge. If the PR-21 branch
already has a PR open, these commits can be included as additional fixes in the same
PR or cherry-picked to main depending on the PR workflow.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_pkcs7_padding.md` — Full root cause analysis of the PKCS7 silent corruption bug: why it was invisible in logs, why `return paddedLen` was catastrophic, decision rationale
- `context_artifacts/adr_wifi_wizard_modal.md` — Why `_onEnterConfiguring()` can't be used, tick-loop polling strategy, alternatives rejected (EventBus, deferred callbacks), hostname-vs-SSID distinction
- `context_artifacts/graveyard_debugging.md` — Four dead-ends: broken `pio pkg exec` esptool, credential storage suspected first, `%zu` suspected as deeper symptom, 5GHz band hypothesis; plus stale monitor claim note
