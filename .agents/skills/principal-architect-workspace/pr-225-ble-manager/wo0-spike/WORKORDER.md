---
name: pr-225-ble-manager-wo0
description: Build-only spike to choose the NimBLE library, finalise the BLE sdkconfig fragment and measure S3 budgets
complexity: standard
---

# WO-0 — Build Spike: Library Choice, sdkconfig Fragment, Budgets

**Parent project:** [`pr-225-ble-manager`](../PROJECT.md) (approval provenance: PROJECT.md → *Approval Record*)
**Parent issue:** [#225](https://github.com/matthewmcneill/esp32-app-foundation/issues/225). Design: RFC rev 2, §16 and §18.
**Branch:** `ble-manager/wo0-spike` (base: `main`, created by the PA)

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Produce measured evidence that settles three questions: (1) which NimBLE C++ library works under this repo's dual `arduino, espidf` build; (2) the final contents of `hardware/_common/sdkconfig.ble.defaults`; (3) the flash/RAM cost on ESP32-S3 and whether 3 peripheral + 1 central connections, plus advertising and scanning, fit the controller's activity budget. **No feature code is kept.**

## Strict Boundary Box
- **Keep (final state of the branch):** `hardware/_common/sdkconfig.ble.defaults`, the `[ble]` mix-in and `[env:esp32s3nano_ble]` in the root `platformio.ini`, and `wo0-spike/FINDINGS.md` (in this WO folder).
- **Temporary spike harness:** `src/ble_spike.cpp` (plus at most a guarded call site in `src/main.cpp`) and a spike-only build flag `-D BLE_SPIKE`. Commit it as **one** commit (`spike(ble): harness`), then remove it in a **second** commit (`spike(ble): remove harness`), so history preserves it but the branch tip is clean.
- **Do NOT** create `modules/BLEManager/`, touch `Application`, `include/`, any other module, `sample-app/`, or `sync_sdkconfig.py` (that is WO-1).
- **Do NOT** change `sdkconfig.defaults` (BT stays disabled for every non-BLE env), and do not change the existing envs.
- **Do NOT** flash hardware. Build-only.
- Do NOT create branches.

## Tooling Constraints
- Use the **`platformio` MCP** for every build: `build_project` (env `esp32s3nano_ble`, and `esp32s3nano` for the baseline), `check_task_status`, `get_lock_status`, `clean_project` if the sdkconfig cache is stale. **No raw `pio` CLI** (`.agents/rules/04-mcp-tooling.md`).
- First ESP-IDF builds take 2–8 min. **Poll `check_task_status` in-session every 30–45 s. Don't park between checks.** Success = clean link + `firmware.bin`. Ignore the known `pre_build_test` Playwright false-`[FAILED]`.
- Delete the generated `sdkconfig.esp32s3nano_ble` whenever the fragment changes (the sync script doesn't watch the fragment yet).
- Temporary files go in the repo `tmp/` (`.agents/rules/02-tmp_workspace.md`).

## Line-Item Instructions

### 1. sdkconfig fragment and env (kept)
- [ ] Create `hardware/_common/sdkconfig.ble.defaults` starting from RFC rev 2 §16.2: `CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, all four roles, `MAX_CONNECTIONS=4`, `MAX_BONDS=4`, `NVS_PERSIST=y`, `MEM_ALLOC_MODE_EXTERNAL=y`, `EXT_ADV=n`, `CONFIG_ESP_COEX_SW_COEXIST_ENABLE=y`. Give it a house-style header comment explaining its purpose and that it overrides `sdkconfig.defaults:26`.
- [ ] In the root `platformio.ini`, add a `[ble]` section (`build_flags = -D WITH_BLE`; `lib_deps` = the candidate library) and `[env:esp32s3nano_ble]` extending `env:esp32s3nano`. It merges `${ble.*}` and sets `board_build.cmake_extra_args = -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;hardware/esp32s3nano/sdkconfig.defaults;hardware/_common/sdkconfig.ble.defaults"`.

### 2. Baseline
- [ ] Build `esp32s3nano` (unchanged) and record `firmware.bin` size, the linker's DRAM/IRAM usage, and the flash usage line.

### 3. Candidate A — `h2zero/esp-nimble-cpp`
- [ ] Add it through the mechanism that works for this dual-framework project (IDF component via `idf_component.yml`, or PIO `lib_deps`). Record exactly what worked.
- [ ] Spike harness (`src/ble_spike.cpp`, under `#if defined(BLE_SPIKE) && defined(WITH_BLE)`). The harness must **exercise every API RFC §5 depends on**, so the linker keeps them:
  - `NimBLEDevice::init`; set the preferred MTU;
  - create a server with **two** services (0x1826 with 3 characteristics, 0x180D with 1 notify characteristic) and start it;
  - legacy advertising with a custom AD payload (flags, service data 16-bit, 16-bit UUID list, appearance) **plus** a scan response carrying the name; start advertising;
  - set the GAP preferred peripheral connection parameters;
  - server callbacks: connect/disconnect/subscribe;
  - a scan with explicit interval/window, passive and active;
  - a client **direct connect to an address with an explicit address type**; service discovery; `subscribe()` on a characteristic;
  - security: bonding + Just Works (`setSecurityAuth`, `setSecurityIOCap`), `NimBLEDevice::getNumBonds()`, `deleteBond(addr)`;
  - the NimBLE host running on its own task. Record the host task's core and stack size.
- [ ] Build `esp32s3nano_ble` with `-D BLE_SPIKE`. Record: success or failure, **any duplicate-symbol or duplicate-host errors** (a sign of IDF host + bundled host collision), warnings, `firmware.bin` size, DRAM/IRAM, and PSRAM-relevant sections.

### 4. Candidate B — `h2zero/NimBLE-Arduino@^2`
- [ ] Repeat §3 with NimBLE-Arduino (same harness; adjust includes/API only where they genuinely differ, and note each difference).
- [ ] Record specifically whether NimBLE-Arduino **uses the IDF host from sdkconfig** or its **own bundled host/config** (`nimconfig.h`). If it bundles its own, record whether the fragment's values take effect.

### 5. Controller activity budget
- [ ] From the IDF docs/Kconfig for the IDF version in use, and the generated `sdkconfig.esp32s3nano_ble`, determine the S3 controller limit on concurrent activities/connections (e.g. `CONFIG_BT_CTRL_BLE_MAX_ACT` or its equivalent). Confirm that **4 connections + 1 advertising set + 1 scan** fit. If a Kconfig value must be raised, add it to the fragment and rebuild.

### 6. Remove the harness
- [ ] Commit the harness (single commit), then remove `src/ble_spike.cpp`, the `src/main.cpp` hook and `-D BLE_SPIKE` in a second commit. Leave the **winning** library in `[ble] lib_deps`, chosen by the PROJECT.md D3 criteria (tie-break: esp-nimble-cpp). If the winner is an IDF component rather than a PIO lib, keep the mechanism that worked and document it in FINDINGS.md.
- [ ] Rebuild `esp32s3nano_ble` **without** the spike to confirm the kept plumbing builds cleanly. The binary delta should then be ≈ 0 against baseline, because nothing references NimBLE. Record it.
- [ ] Rebuild `esp32dev` and `esp32s3nano` to confirm they're unaffected.

### 7. FINDINGS.md
- [ ] Write `wo0-spike/FINDINGS.md` containing:
  - a candidate comparison table (build result, mechanism, host used, flash Δ, DRAM Δ, IRAM Δ, warnings, API gaps);
  - the **recommended library** against the PROJECT.md D3 criteria (tie-break: esp-nimble-cpp);
  - the final fragment contents with a one-line rationale per key;
  - the controller activity-budget verdict;
  - any API gap versus RFC §5 (e.g. whether address type is exposed on scan results, whether per-connection preferred parameters are possible, whether the subscribe-changed callback gives the characteristic handle);
  - risks for WO-1…WO-3.
- [ ] Record **measured numbers only**, with the MCP task IDs and log excerpts they came from.

## Verification Requirement
- `esp32s3nano_ble` builds clean at the branch tip (no spike); `esp32dev` and `esp32s3nano` build unchanged.
- FINDINGS.md exists and every number in it traces to a build log.
- **Before committing, ask the user whether they're happy with the changes.**

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box* (e.g. a sync-script flaw, an IDF/NimBLE upstream bug), record it under *Out-of-Scope Findings* below. Don't file issues. The PA triages them.

---
## Completion Report
<!-- Written by the worker at task completion -->

### Status
complete | partial | blocked

### Changes Made
- `path/to/file`: what changed and why

### Decisions Made
- Decision description and rationale

### Dead Ends & Warnings
- What was attempted, why it failed, and what to avoid

### Out-of-Scope Findings
- Finding description and suggested action

### Blockers
(none)
