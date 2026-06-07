---
title: OTA Hotfix + Documentation Session — 2026-06-06/07
distilled_at: 2026-06-07T01:45:00Z
original_plan_id: standalone
artifacts:
  - context_artifacts/adr_ota_boot_check_gate.md
  - context_artifacts/adr_release_channel_floor.md
---

# Context Bridge — OTA Docs & Hotfix Session

## Executive Summary

A documentation and hotfix session that shipped `docs/build_and_release_process.md`,
fixed an OTA boot-check modal bug (spurious "Firmware Update Failed" on boot),
added a `WiFi.status()` connectivity gate to `tick()`, aligned the SemVer
channel enum to contiguous 2-bit values, and merged the user's risk-floor
channel model improvements to `GithubClient`.

## Commits This Session (newest first)

| SHA | Summary |
|-----|---------|
| `8dfd942` | distillery: screensaver TWDT session archive |
| `7e5d9fd` | chore: close TWDT crash todo item |
| `842a7e5` | fix: setPowerSave(1) for true hardware sleep |
| `7383f2b` | fix: clear display buffer on blank |
| `026354b` | fix: TWDT crash — add vTaskDelay, skip SPI on blank |
| `767ee4f` | fix: USB JTAG console + localtime_r thread safety |
| `ea48bbc` | fix: setContrast(0) instead of setPowerSave(1) |
| `088df9d` | merge: PR-19 sdkconfig layering |
| `b28fb96` | fix: full_version_str NameError regression |
| `9f07b65` | refactor: MAX_OTA_BOARDS from default_envs |
| `380e96c` | fix: gate OTA auto-check on WiFi.status() == WL_CONNECTED |
| `77318d0` | fix: contiguous 2-bit channel enum, 6-bit PRE_NUM |
| `4a23f4a` | fix: suppress error modal on silent auto-check failures |
| `427495d` | docs: NVS → LittleFS correction in build doc |
| `40c9050` | docs: add release channels section |
| `c707026` | docs: address key rotation review comments |

## Next Steps for a Future Agent

1. **Publish v0.0.5** — User must run from terminal:
   ```bash
   python3 scripts/deploy_release.py --tag v0.0.5 --local --private-key private_key.pem
   ```
2. **Screensaver toggle switches still grey** — UI state binding bug in
   `display.js` / CSS. Check `stash@{1}` (`34f07b6`) — may already have fix.
3. **Standardise Advanced sections** — Yellow collapsible drawers across all
   portal tabs. OTA card is reference implementation.
4. **Build Display primary tab** — Module-owned bottom-nav tab via
   `IWebContributor` for screensaver, flip, brightness, screenshot controls.
5. **Clarify `appconfig.js` portal tab** — Stash `stash@{2}` (`580f2f5`)
   holds route plumbing; determine if already merged, then drop stash.
6. **Slim down `FrameworkConfig`** — Move hostname, timezone, etc. into
   module `resetConfig()` methods.
7. **Extend sample app** — Weather widget + 2-panel diagnostics carousel.
8. **Developer documentation + framework-expert skill** — See todo_list.md.

## Key Architectural Facts

### OTA Boot Check (as of `380e96c`)
- `tick()` returns early if `WiFi.status() != WL_CONNECTED`
- `_hasBootChecked` only set when network is genuinely up
- `_silentCheck = true` on all auto-check paths; `false` on manual API action
- GitHub query failure on auto-check → `Idle`, no modal; on manual → `Error` + modal

### Channel Enum (as of `77318d0`)
- Contiguous 2-bit: `DEV=0, BETA=1, RC=2, STABLE=3`
- Old non-contiguous (DEV=0, BETA=2, RC=3, STABLE=15) is retired
- `inject_version.py` `_CHANNEL_MAP` updated to match

### Release Channel Model (`GithubClient`)
- Risk-floor: subscription is minimum stability, not exact track
- `beta` subscriber sees beta + rc + stable (highest SemVer wins)
- Single-pass scan with best-candidate tracking; committed at end

### OTA Config Persistence
- All OTA config (channel, autoCheck, checkHour) → `/mod/ota.json` on LittleFS
- **Not NVS** — NVS is reserved for WiFi credentials and ESP-IDF OTA slot tracking

### Signing Key Protection
- `private_key.pem` — AES-256 encrypted, `.gitignore`d
- Key rotation requires two-step transitional release (old key signs
  firmware containing new public key; see docs/build_and_release_process.md §12)

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_ota_boot_check_gate.md` — Why the WiFi gate + _silentCheck design was chosen over alternatives
- `context_artifacts/adr_release_channel_floor.md` — Risk-floor channel model, enum changes, GithubClient rewrite rationale
