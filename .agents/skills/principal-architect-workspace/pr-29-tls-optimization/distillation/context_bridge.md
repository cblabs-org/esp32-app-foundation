---
title: "PR-29 TLS Memory Optimization — Principal Architect Session"
distilled_at: 2026-06-18T17:00:00Z
original_plan_id: pr-29-tls-optimization
artifacts:
  - context_artifacts/adr_tls_optimization.md
  - context_artifacts/adr_ota_ca_pinning.md
  - context_artifacts/graveyard_build_failures.md
  - context_artifacts/state_branch_and_tags.md
---

# Context Bridge — PR-29 TLS Memory Optimization

## Executive Summary

Completed WO1–WO3 of the TLS memory optimization PR: applied 7 mbedTLS sdkconfig settings, measured +18.8 KB free heap (+12.7%) and +54 KB heap_min_free (+62.7%) on ESP32-S3 NanoS3, documented everything in the framework-expert skill. Discovered during live OTA testing that the GitHub CDN uses a different CA (Let's Encrypt) than the API (Sectigo), breaking manifest downloads when cert bundle is NONE. Applied a temporary `setInsecure()` fix for the manifest; WO4 (dual-CA pinning with graceful fallback) is written and handed off to the OTA architect.

> [!WARNING]
> **PR numbering error:** This work is labelled PR-29 throughout (branch name, commits, work orders, MEMORY.md registry) but should be **PR-27**. The branch, commits, and all references need renumbering. The user has asked the next architect to clean this up.

## Next Steps

1. **OTA architect picks up WO4**: Work order at `.agents/skills/principal-architect-workspace/pr-29-tls-optimization/wo4-ota-ca-pinning/WORKORDER.md`. Creates `ota_root_cas.h` (Sectigo E46 + ISRG Root X1), updates OTAUpdateManager with CA-first + `setInsecure()` fallback, tests OTA end-to-end.

2. **Deploy dev.10 as GitHub release**: Tag `v0.0.10-dev.10` is on HEAD (`5415b48`) but not yet deployed as a GitHub Release. Run:
   ```bash
   cd /Users/mcneillm/Documents/Projects/esp32-app-foundation
   unset GITHUB_TOKEN
   python3 scripts/deploy_release.py --local --build --private-key private_key.pem
   ```
   (Requires user passphrase for signing.)

3. **After WO4 merges**: Deploy `dev.11` with the dual-CA pinning, trigger OTA from the device (running `dev.10`) to validate the full pinned-CA OTA path.

4. **Merge to main**: Once WO4 is verified, merge `pr-29-tls-optimization` → `main`. User decides whether to cut a beta.

5. **Update framework-expert skill**: WO4 should update `performance/tls-memory-optimization.md` with the dual-CA bundle details, fallback logic, and the CDN CA discovery.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

## Deep Context Menu

- `context_artifacts/adr_tls_optimization.md` — Architecture decisions for mbedTLS tuning (dynamic buffers, cert bundle NONE, peer cert, ALPN constraint)
- `context_artifacts/adr_ota_ca_pinning.md` — Why dual-CA pinning with fallback, storage options evaluated (NVS/LittleFS/compiled), partition safety during OTA
- `context_artifacts/graveyard_build_failures.md` — ALPN compile guard, serial port locking, CDN cert mismatch, JSON IncompleteInput
- `context_artifacts/state_branch_and_tags.md` — Exact branch topology, tag locations, commit hashes, device state
