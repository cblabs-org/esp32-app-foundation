---
schema_version: 1
wo_id: WO-327.3
pr_id: PR-327
title: Remediate Round 1 audit findings in docs/release-audits/v1.0.0-beta.1.md
complexity: standard
status: PENDING
allowed_files:
- docs/release-audits/v1.0.0-beta.1.md
forbidden_files:
- docs/design_specification.md
- modules/U8G2Display/
- modules/U8G2GLDisplay/
required_skills:
- framework-expert
verification_command: pytest scripts/tests/ && python3 scripts/doc_lint.py && python3
  scripts/check_doc_cpp_samples.py
max_attempts: 5
max_structural_failures: 2
spec_hash: 2f260f454511244ecc7f722b71b8e1ca6ff27f12106d7455839aa91426397e96
completion: null
---

# WO-327.3 — Remediate Round 1 audit findings in docs/release-audits/v1.0.0-beta.1.md

## 1. Role & Objective

You are executing `WO-327.3` on branch `launch/pr-b-release-audit` for `PR-327` to remediate `BLOCKING-1` and `NON-BLOCKING-1` from `AUDIT-round-1.md` in `docs/release-audits/v1.0.0-beta.1.md` (and reconcile the Verdict advisory summary so all counts and line references match the live repository).

## 2. Strict Boundary Box & Constraints

- **Allowed files (`allowed_files`):** Modify ONLY `docs/release-audits/v1.0.0-beta.1.md` plus Part II (`status`, `completion`, and `## Completion Report`) of this `WORKORDER.md`.
- **Forbidden files (`forbidden_files`):** Never touch `docs/design_specification.md`, `modules/U8G2Display/`, `modules/U8G2GLDisplay/`, `CHANGELOG.md`, or any file outside `allowed_files`.
- **Strict Redaction & No Personal Paths:** Never write personal filesystem paths (`/Users/...`, `/home/...`), real Wi-Fi SSIDs, LAN IPs/subnets, or device MAC addresses in `docs/release-audits/v1.0.0-beta.1.md` or `WORKORDER.md` prose.
- **No `git add`/`git commit` or `pa wo verify`:** Leave all edits **unstaged** (`git diff --cached --name-only` empty) for Coordinator receipt stamping, set `status: ESCALATED_TO_PA`, `completion.status: ESCALATED_TO_PA`, `completion.verification_summary <= 280` chars, and `completion.escalation_request` (`blocker_type: structural`, `reason_code: COORDINATOR_RECEIPT_STAMPING`, `target_role: pa`), then send results back via `send_message`.

## 3. Line-Item Instructions

### 3.1 `BLOCKING-1` — Next-Version Beta Tag Command (`docs/release-audits/v1.0.0-beta.1.md:164`)
1. Inspect `scripts/next_version.py` and verify via `python3 scripts/next_version.py --channel beta` (`v0.0.12-beta.5`) vs `python3 scripts/next_version.py --channel beta --bump major` (`v1.0.0-beta.1`) and `python3 scripts/next_version.py --channel beta --target 1.0.0` (`v1.0.0-beta.1`).
2. Update line `L164` in `docs/release-audits/v1.0.0-beta.1.md` so `python3 scripts/next_version.py --channel beta` is replaced with `python3 scripts/next_version.py --channel beta --bump major` (or `--target 1.0.0`) before `git tag v1.0.0-beta.1 && git push origin v1.0.0-beta.1` after merging `PR-327`.

### 3.2 `NON-BLOCKING-1` — Line Citations, Sub-Counts, & Build Timing Precision
1. **`CHANGELOG.md` Line Citations (`L32`, `L51`, `L132`, `L133`):**
   - Inspect `CHANGELOG.md` directly and update the four shifted line ranges resulting from the `+3`-line `Release audit:` cross-link insertion (`L50-52`):
     - `L32`: `CHANGELOG.md:10-87` -> `CHANGELOG.md:10-90`
     - `L51`: `CHANGELOG.md:53-56` -> `CHANGELOG.md:54-59`
     - `L132`: `CHANGELOG.md:54-56` -> `CHANGELOG.md:54-59`
     - `L133`: `CHANGELOG.md:59-61` -> `CHANGELOG.md:62-64`
2. **D6 `hardcoded_hex_color` `33` Non-Defect Breakdown (`L48`):**
   - Replace the imprecise sub-counts in Finding `#7` (`L48`) with the verified `41`-hit breakdown (`33` non-defects + `8` genuine literal `#fff` text colors):
     - `20` CSS token definitions + `2` `#issue` comments in `web/portal/app.css` (`22` hits)
     - `3` matching `var()` fallbacks + `2` comments in `web/portal/system.js` (`5` non-defect hits)
     - `6` HTML numeric entities / `#issue` comments across `modules/BLEManager/web/ble.js` (`2`), `modules/MCPServer/web/mcp.js` (`1`), `web/portal/auth.js` (`1`), and `uidesign/crowpanel-128-rotary/lvglpro/preview-bin/lved-runtime.js` (`2`)
     - `8` genuine `#fff` literal text assignments (`3` in `web/portal/system.js:331,502,598`, `2` in `web/display/display.js:300,308`, `1` in `web/portal/credits.js:42`, `1` in `web/portal/keys.js:123`, `1` in `web/portal/utils.js:76`).
3. **D8 `duplicate_driver_registration` `7`-File Breakdown (`L49`, `L122`):**
   - Update Finding `#8` (`L49`) and Dimension 8 (`L122`) to document why `release-audit-checks.sh` (`grep -rl` without `--exclude-dir`) reports `7` files:
     - `2` authored files (`adapters/lvgl/CrowPanel128Adapter.cpp` + `modules/EncoderModule/EncoderModule.cpp` = `3` authored calls: `2` `lv_indev_create()` + `1` `pcnt_unit_config_t`)
     - `1` unit test stub (`test/test_unit/test_lvgl_stubs/test_lvgl_stubs.cpp`)
     - `1` library file (`.pio/libdeps/crowpanel-128-rotary/lvgl/src/drivers/display/lovyan_gfx/lv_lovyan_gfx.cpp`)
     - `3` corresponding files under `tmp/pathb/framework/`.
4. **D7 `crowpanel-128-rotary` Local Build Timing Clarification (`L113`):**
   - Update `L113` to clearly distinguish post-`WO-327.1` local `esp32s3nano` builds (`root` `130.84s`, `sample-app` `126.59s`, `template-app` `106.97s`) from the pre-`WO-327.1` `crowpanel-128-rotary` local build (`218.03s`, `.pio-mcp-workspace/logs/build/build-2026-11-16T10-50-28-765Z-a76e3a60.log`), alongside `crowpanel-128-rotary` passing in CI Run `43891720641`.

### 3.3 Verdict Advisory Reconciliation (`docs/release-audits/v1.0.0-beta.1.md:158`)
- Update `L158` to clarify `"6 advisory categories (8 advisory rows + 2 info/boundary rows across D1–D10)"` so the Verdict summary reconciles cleanly with the 10-row Findings table (`L42-51`).

## 4. Verification, Receipt & Handoff

1. Confirm `python3 scripts/next_version.py --channel beta --bump major` and `python3 scripts/next_version.py --channel beta --target 1.0.0` both output `v1.0.0-beta.1`.
2. Run the mandatory verification command:
   ```bash
   pytest scripts/tests/ && python3 scripts/doc_lint.py && python3 scripts/check_doc_cpp_samples.py
   ```
3. Run a redaction & secret check on `docs/release-audits/v1.0.0-beta.1.md` (zero `/Users/`, `/home/`, personal emails, real SSIDs, raw LAN IPs, or device MACs).
4. Confirm `git status --short` shows only `docs/release-audits/v1.0.0-beta.1.md` and this `WORKORDER.md` modified, and `git diff --cached --name-only` is empty.
5. Populate Part II of this `WORKORDER.md` (`status: ESCALATED_TO_PA`, `completion.status: ESCALATED_TO_PA`, `completion.verification_summary <= 280` chars, `completion.escalation_request` with `blocker_type: structural`, `reason_code: COORDINATOR_RECEIPT_STAMPING`, `target_role: pa`, and concise `## Completion Report`), then reply via `send_message`.

## Completion Report
<!-- Written by the Tier 3 Worker at task completion -->

_Pending execution._
