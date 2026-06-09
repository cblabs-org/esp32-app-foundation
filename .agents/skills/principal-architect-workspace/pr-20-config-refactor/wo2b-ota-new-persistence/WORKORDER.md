---
name: pr-20-config-refactor-wo2b
description: Handoff for WO2b — persist firmwareUpdatesEnabled as OTA module config with a full OTA-tab toggle, delete the dead otaQuietHour field + stale docs, leaving FrameworkConfig at the 3 OTA secrets. Refactor.
complexity: standard
---

# pr-20-config-refactor / WO2b — OTA Config Persistence + FrameworkConfig Final Slim

**Branch:** `pr-20-config-refactor/wo2b-ota-new-persistence` (cut by the PA, off `pr-20-config-refactor/wo2a-persisted-moves`)

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are **NOT** the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Execute the checklist below exactly — no more, no less.

## Objective

Make `firmwareUpdatesEnabled` an **OTAUpdateManager-owned persisted config field** with a full end-to-end OTA-tab toggle, **delete** the dead `otaQuietHour` field (and its stale docs), and remove both from `FrameworkConfig`. This leaves `FrameworkConfig` at its final form: the three OTA secrets only.

## Background (researched — act on this, don't re-litigate)

- `firmwareUpdatesEnabled` is the **master kill-switch on *applying* an update** — read once at `OTAUpdateManager.cpp:408` in `performUpdateTask()`. It is **distinct** from the already-persisted `_autoCheck` (which gates the auto-*check* scheduler at line ~204). Keep both; do not merge them.
- `otaQuietHour` is **dead** — zero code readers anywhere; the existing persisted `_checkHour` already covers OTA scheduling. It survives only as stale docs.
- **The pattern to mirror is `_autoCheck`** — a boolean OTA setting already wired end-to-end: persisted in `serializeConfig`/`applyConfig`/`resetConfig`, settable via the `POST /api/ota` handler, returned in status, and toggled in `web/portal/ota.js`. Replicate that exact path for the new field.

## Design Decisions (fixed — implement as stated)

1. **`firmwareUpdatesEnabled` → persisted OTA config.** Add member `_updatesEnabled` (default **`true`**). Add JSON key `updatesEnabled` to `serializeConfig`/`applyConfig`, default in `resetConfig()`. Replace the `OTAUpdateManager.cpp:408` read `cfg.firmwareUpdatesEnabled` with `_updatesEnabled` (drop the `app_->getConfig()` use *for this field only* — the OTA **secrets** still come from `FrameworkConfig`).
2. **Full OTA-tab toggle.** Wire `updatesEnabled` into the `POST /api/ota` handler, the status/`GET` response, and `web/portal/ota.js` — mirroring `autoCheck` exactly. Use a **distinct UI label** (master "Enable firmware updates" switch) so it isn't confused with the "Auto-check" toggle.
3. **Delete `otaQuietHour`.** Remove it from `FrameworkConfig`; do **not** implement any quiet-hour behaviour (out of scope). Correct/remove its stale doc references.

## Strict Boundary Box

- **DO NOT** touch `_autoCheck`, `_checkHour`, `_channel`, or any other existing OTA config — only add `updatesEnabled`.
- **DO NOT** remove or alter the OTA secrets (`otaPublicKey`, `otaGithubRepo`, `otaGithubToken`) — they stay in `FrameworkConfig` and OTAUpdateManager still reads them via `app_->getConfig()`.
- **DO NOT** perform the OO-review's `app_`-removal refactor.
- **DO NOT** build any quiet-hour feature.
- **DO NOT** branch, merge, or rebase.

## Line-Item Instructions

### OTAUpdateManager (`modules/OTAUpdateManager/`)
- [ ] Add `bool _updatesEnabled` member, default `true`.
- [ ] `resetConfig()`: set `_updatesEnabled = true`.
- [ ] `serializeConfig()`: write `section["updatesEnabled"] = _updatesEnabled`.
- [ ] `applyConfig()`: read `_updatesEnabled = section["updatesEnabled"] | true`.
- [ ] `:408` replace `if (!cfg.firmwareUpdatesEnabled)` → `if (!_updatesEnabled)` (and drop the now-unneeded `cfg`/`getConfig()` reference *if* it's used only for this — keep it if it still reads secrets nearby).
- [ ] `POST /api/ota` handler (~line 1092–1165): handle an incoming `updatesEnabled` bool, mirroring the `autoCheck` branch (validate, assign, log, persist).
- [ ] Status/`GET` response (~line 1170–1190): add `resp["updatesEnabled"] = _updatesEnabled` (mirror `autoCheck`).

### Web portal (`web/portal/ota.js` + asset pipeline)
- [ ] Add an "Enable firmware updates" toggle mirroring the `autoCheck` toggle (read from status, POST on change). Distinct label/id from auto-check.
- [ ] Regenerate the gzipped portal asset header the same way the build does for `ota.js` (follow the existing asset-embedding mechanism; `pio run` runs the web pre-build hook).

### FrameworkConfig (`include/FrameworkConfig.hpp`)
- [ ] Remove **both** `firmwareUpdatesEnabled` and `otaQuietHour` fields + their doc-header lines. Result: only `otaPublicKey`, `otaGithubRepo`, `otaGithubToken` remain.

### Docs cleanup (use the `house-style-docs` skill)
- [ ] `docs/OTADeveloperGuide.md:599` — remove the `otaQuietHour` row; document `updatesEnabled` as OTA module config if a config table exists there.
- [ ] `docs/design_specification.md:331` and `:1938` — remove/correct the `otaQuietHour` references; reflect that `firmwareUpdatesEnabled` is now OTA module config (`updatesEnabled`) and `FrameworkConfig` holds only the 3 OTA secrets.

### Tests
- [ ] `test/test_unit/test_otaUpdateManager` — add: `updatesEnabled` serialize/apply round-trip, `resetConfig()` default `true`, and the kill-switch gating behaviour (update blocked when `false`).
- [ ] If the web test harness (`test/web/`) covers OTA-tab controls, add a check for the new toggle (mirror any existing `autoCheck` web test).

## Verification Requirement
- [ ] **`pio test -e native` → all suites PASSED, 0 ERRORED** (current baseline 522; expect ≥522 with new tests — do not regress).
- [ ] **`pio run -e esp32dev` builds clean** (this also runs the web-test pre-build hook — it must pass).
- [ ] Route all build/test through the `pio-manager` skill; use `systematic-debugging` for any failure.
- [ ] Sanity: confirm `FrameworkConfig` now contains exactly the 3 OTA secrets.

**CRITICAL:** Before distilling or committing, **STOP and ask the user if they are happy**. Only *after* explicit approval invoke `@distillery` (target = this WO dir), append a status line to this `WORKORDER.md`, and terminate.

> *(Spawn note for the PA: include the claude-code adapter's artifact-write authorization block in the spawn prompt.)*
