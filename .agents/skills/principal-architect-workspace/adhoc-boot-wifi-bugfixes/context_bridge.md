---
title: Boot & WiFi Hotfix — PKCS7 Corruption + WiFi Wizard Modal + %zu printf
distilled_at: 2026-06-11T22:13:00+01:00
original_plan_id: standalone
branch: pr-21-ota-task-hardening
artifacts:
  - context_artifacts/adr_pkcs7_padding.md
  - context_artifacts/adr_wifi_wizard_modal.md
  - context_artifacts/graveyard_debugging.md
---

# Boot & WiFi Hotfix — Three Ad-Hoc Fixes on pr-21

## Executive Summary

A serial monitor debugging session on 2026-06-11 found and fixed three bugs on
`pr-21-ota-task-hardening`. The device was failing WiFi authentication on every
boot (`AUTH_EXPIRE`) and never showing the WiFi wizard modal in AP mode.
All three fixes were tested on-device and verified via UniFi — the ESP32 now
connects to `cb-iot` on first attempt (IP: 192.168.20.186).

## What Was Done

| Commit | Bug | Fix |
|---|---|---|
| `dff3b22` | PKCS7 padding appending garbage bytes to decrypted password → `AUTH_EXPIRE` | `return 0` not `return paddedLen` in `removePKCS7Padding()` |
| `a0c91dc` | WiFi Wizard modal never shown in AP mode | CONFIGURING tick polls `getAPMode()`, pushes `wifiWizard` modal once via `_wifiWizardShown` guard |
| `a0c91dc` | `%zu` logging as literal `"zu"` | 9 occurrences replaced with `%u` + `(unsigned)` cast (Newlib nano limitation) |

## Next Steps

These commits sit on `pr-21-ota-task-hardening`. They are independent of the
OTA task hardening scope and should be included in that PR submission or
cherry-picked depending on stack state.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_pkcs7_padding.md` — Root cause of PKCS7 silent corruption: why `return paddedLen` was catastrophic, why it was invisible in logs
- `context_artifacts/adr_wifi_wizard_modal.md` — Why `_onEnterConfiguring()` fires too early, tick-loop polling strategy, hostname-vs-SSID distinction, alternatives rejected
- `context_artifacts/graveyard_debugging.md` — Four dead-ends: broken `pio pkg exec` sandbox, credential storage hypothesis, `%zu` as symptom theory, 5GHz band hypothesis
