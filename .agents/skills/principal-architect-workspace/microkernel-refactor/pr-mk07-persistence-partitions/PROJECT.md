# PROJECT.md — pr-mk07: Persistence Tiers & Partition Architecture

**WO branch stack base:** `pr-mk05-extract-optionals`
**Depends on:** pr-mk05 (stable module set) **+ hardware-config program (cross-program)** · **Blocks:** —
**Design refs:** MK-14, MK-15, MK-16, MK-17, MK-18 · **Gating decisions:** **OQ-MK-4** (per-channel Golden). OQ-MK-5 ✅ resolved (`P` ≈ 10–15 KB). **Preset B implementation deferred** — all three consumers are Preset A (#48); the seam is built, the asset-OTA machinery is a consumer-triggered follow-on
**Status:** 📋 **PLANNED — awaiting human approval + cross-program coordination**

---

## Architectural Overview

Implements the four-tier persistence model and the consumer-selectable partition
architecture, closing issue #40 (asset/FS updates wiping `/etc` identity). Because
partition tables are the immutable layer and there are **no field devices**, this
is a clean new-layout design. It **depends on the hardware-config program** for
`board.<id>.h` + partition selection, so it is sequenced last and coordinated
with that program's PA.

### Key Design Decisions

| # | Decision | Rationale |
|---|---|---|
| PP-01 | Four tiers: NVS (secrets) / `config` LittleFS partition (`/etc`+`/mod`) / A/B app slots (firmware + compiled-in assets) / disposable `data`/cache | Each matched to data nature (MK-14) |
| PP-02 | Compiled-in assets in the signed A/B image is the **default** | Inherits rollback + signature verify; portal already does this (MK-16) |
| PP-03 | Build the **consumer-selectable preset *seam*** (A compiled-in vs B FS-assets); ship Preset A CSVs now | App developer owns the trade-off; the seam preserves the Preset-B option without building it (MK-16) |
| PP-04 | **Preset B asset OTA (verify-before-commit + compiled-in fallback) is DEFERRED** — no current consumer (all three are Preset A, #48) | The layout-agnostic seam (PP-05) keeps it addable with no rework; building the asset-OTA machinery now = speculative mechanism. **Trigger:** a consumer's compiled-in assets outgrow the 2× A/B flash cost |
| PP-05 | Firmware is **layout-agnostic at runtime** — probe partition table, adapt, config-root path abstraction | One image serves every preset (MK-17) |
| PP-06 | `config` partition sized **per-target**: **1 MB** on 16 MB boards, **256 KB** on 4 MB boards | Immutable layer sized for the worst-case soft feature; a fixed 1 MB was rejected as unaffordable on 4 MB esp32dev (departures-board #48). Real `P` ≈ 10–15 KB → 256 KB carries the ~5× peak with headroom (MK-18) |
| PP-07 | `timezones.json` = re-fetchable cache, not a shipped asset | Disposable tier; device re-pulls (MK-14) |
| PP-08 | **OQ-MK-4:** align with the per-channel-Golden config lifecycle issue | Sets the multiplier; partition already sized to absorb it |

## Strict Boundary Box

- Persistence/partition/mount work **only**. **Do not** re-open module extraction.
- **Coordinate partition CSV + `board.<id>.h` changes with the hardware-config PA**
  — do not run concurrently on `partitions.*.csv` / `platformio.ini`.
- Migration is **new-layout-only** (no field-device compat path — MK-15).

## Tooling Constraints

`platformio` MCP tools (incl. filesystem upload); verify `native`, `esp32dev`,
`esp32s3nano`. Dual-mount uses `esp_vfs_littlefs_register`
(`CONFIG_LITTLEFS_MAX_PARTITIONS=3` already set), **not** the Arduino `LittleFS`
singleton.

## Proposed Work Order Ledger _(PA to finalise; likely a sub-plan)_

- **[ ] wo1 — config-root abstraction:** route `KeyManager`/`AuthManager`/
  `ConfigManager` `/etc` paths through one accessor. `standard`
- **[ ] wo2 — `config` partition + layout-agnostic mount:** dual-mount + boot
  probe + fallback. `architect`
- **[ ] wo3 — partition presets:** blessed A/B `partitions.csv` set (coord.
  hardware-config). `standard`
- **[ ] wo4 — Preset B asset OTA:** ⏸ **DEFERRED — out of this Project's scope**
  (no current consumer; all three are Preset A, #48). The seam (wo2) keeps it
  addable later; spin a follow-on Project when the trigger fires. `architect`
- **[ ] wo5 — sizing + lifecycle alignment:** **per-target `config`** (1 MB on
  16 MB / 256 KB on 4 MB); align with per-channel-Golden issue (OQ-MK-4).
  (`P` resolved ≈ 10–15 KB.) `standard`

## Verification

All three envs build; asset/FS update no longer wipes `/etc`; layout probe works
across presets; identity survives an OTA + a Preset-B asset update; native FS
stub unaffected by the path abstraction.

## Status Log

| Date | Entry |
|---|---|
| 2026-07-28 | Drafted. Awaiting human approval, OQ-MK-4/5, and hardware-config coordination. Sequence last. |
