---
title: "PR-29 TLS Optimization — Final Distillation (Session 2)"
distilled_at: 2026-06-19T10:15:00Z
original_plan_id: pr-29-tls-optimization
artifacts:
  - context_artifacts/adr_tls_optimization.md
  - context_artifacts/adr_ota_ca_pinning.md
  - context_artifacts/adr_cert_bundle_fix.md
  - context_artifacts/adr_ap_scan_fix.md
  - context_artifacts/graveyard_build_failures.md
  - context_artifacts/state_branch_and_tags.md
---

# Context Bridge — PR-29 TLS Memory Optimization (Final)

## Executive Summary

**Session 1** (07cb3857): Applied 7 mbedTLS sdkconfig optimizations, measured +18.8 KB free heap, documented in framework-expert skill. Discovered GitHub CDN CA mismatch. Created WO4–WO10 for remaining work.

**Session 2** (d5f9f607): Fixed critical build failure caused by `CERTIFICATE_BUNDLE=n` breaking cmake's `target_add_binary_data()`. Switched to `DEFAULT_CMN` bundle. Fixed AP shutdown during WiFi scan (driver mode-switch bug). Fixed `_lastApiRequestTime` volatile race. Verified OTA end-to-end with `v0.0.11-dev.7`. Merged to `main` (`aec9808`).

## What Was Accomplished (Session 2)

1. **Build system fix**: `CERTIFICATE_BUNDLE=n` → `DEFAULT_CMN` in `sdkconfig.defaults`. Removed `gen_cert_asm.py` workaround.
2. **AP scan fix**: `WiFi.scanNetworks(true)` in AP mode now explicitly manages mode (AP → AP_STA → scan → AP). Prevents AP teardown.
3. **Thread safety**: `_lastApiRequestTime` marked `volatile` for cross-thread visibility.
4. **TLS documentation**: Updated `tls-tradeoffs.md` with cert bundle configuration guide, developer instructions for removing bundles.
5. **OTA verified**: Full cycle — deploy `v0.0.11-dev.7` to GitHub, device downloads over TLS, signature verified, firmware blessed, running.
6. **Stack merged**: `pr-29-tls-optimization` → `main` (`aec9808` → `18798d0`)

## Remaining Work (Not in PR-29 scope)

These items from the original WO list were **not addressed** in this session and should be tracked separately:

- **WO5** (deploy repo flag) — deploy script flag for split repos
- **WO7** (build channel fix) — `BUILD_CHANNEL` injection via `inject_version.py`
- **WO8** (security tab TLS split) — separate password/auth from TLS in portal
- **WO9** (password restart fix) — device fails to restart when setting password
- **WO10** (MCP key UI bugs) — bearer key display issues when password is set

These are tracked in `.agents/todo/todo.md` (items under 🔒 Security).

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

## Deep Context Menu

- `context_artifacts/adr_tls_optimization.md` — Architecture decisions for mbedTLS tuning (dynamic buffers, cert bundle, peer cert, ALPN)
- `context_artifacts/adr_ota_ca_pinning.md` — Why dual-CA pinning with fallback, CDN CA discovery
- `context_artifacts/adr_cert_bundle_fix.md` — **[NEW]** Why CERTIFICATE_BUNDLE=n breaks cmake and DEFAULT_CMN is the right tradeoff
- `context_artifacts/adr_ap_scan_fix.md` — **[NEW]** WiFi.scanNetworks() tears down AP in AP-only mode; explicit mode management fix
- `context_artifacts/graveyard_build_failures.md` — ALPN compile guard, serial port locking, CDN cert mismatch, JSON IncompleteInput
- `context_artifacts/state_branch_and_tags.md` — Branch topology, tags, commit hashes (outdated — see merged state below)

## Final State

- **Branch**: `pr-29-tls-optimization` merged to `main` at `18798d0`
- **Tag**: `v0.0.11-dev.7` — verified running on esp32dev device
- **Device**: `192.168.20.215`, OTA channel `dev`, heap 205 KB free
