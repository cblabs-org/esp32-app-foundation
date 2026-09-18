---
name: pr-225-ble-manager-wo5
description: OTA pause/resume coupling, coexistence policy hardening, developer guide, framework-expert skill material, CHANGELOG
complexity: standard
---

# WO-5 — Coexistence, OTA Pause, Docs and Skill

**Parent project:** [`pr-225-ble-manager`](../PROJECT.md) (approval provenance: PROJECT.md → *Approval Record*)
**Parent issue:** [#225](https://github.com/matthewmcneill/esp32-app-foundation/issues/225). Design: RFC rev 2, §6.1, §9, §15, §16.5.
**Branch:** `ble-manager/wo5-coexistence-ota` (base: `ble-manager/wo4-portal-mcp`)
**Gate (PROJECT.md D8):** don't start until the PA confirms that either #227's `OTA_PROGRESS` fix is on `main` (the PA will have synced the stack), **or** the human chose the fallback. The PA states which in the spawn prompt, and it is recorded in PROJECT.md.

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Couple BLEManager to OTA: pause on download start, resume on failure, and let the success path reboot. Harden and document the coexistence policy, then write the consolidated developer documentation, framework-expert skill material and CHANGELOG entry for the module.

## Strict Boundary Box
- **In scope:** `modules/BLEManager/**` (the OTA subscription + coexistence hardening only), `modules/BLEManager/README.md`, `docs/developer/bluetooth.md` (new), `docs/developer/using-optional-modules.md` (BLE section + a link), `docs/developer/README.md` (index link), `.agents/skills/framework-expert/systems/ble/` (new) + the `SKILL.md` trigger list/index, and `CHANGELOG.md`.
- **Do NOT** modify `OTAUpdateManager` or `EventBus` (#227 is its own work), `Application`, or other modules. **Do NOT** change the provider interfaces.
- Do NOT create branches.

## Tooling Constraints
- `platformio` MCP only; poll in-session. Versioning rule 03 before any flash.
- Docs: `house-style-docs`, and `scripts/doc_lint.py` must pass (run with `python3 scripts/doc_lint.py`, a read-only lint).
- The framework-expert skill is repo-owned (not a submodule). Mirror the structure of an existing `systems/<area>/` folder, e.g. `systems/event-bus/` or `systems/ota/`.

## Line-Item Instructions

### 1. OTA coupling (under `#ifdef WITH_OTA`)
- [ ] In `begin(app)`, subscribe to `ApplicationEvents::OTA_PROGRESS`. **The callback runs on the esp_event task.** It copies `complete`/`success` into atomics and sets a flag. No BLE calls happen in the callback.
- [ ] In `tick()`:
  - on the first progress event with `!complete`, call `pause("ota")`, which stops advertising and scans, disconnects peers if `otaDisconnectPeers`, and calls `onStackPaused()`;
  - on `complete && !success`, call `resume()`;
  - on `complete && success`, do nothing (the device reboots).
- [ ] If the PA's spawn prompt says **fallback** (because #227 isn't landed), apply the defensive reading described in PROJECT.md D8 and add a code comment linking #227.

### 2. Coexistence hardening
- [ ] Audit that every scan path (discovery, reconnect initiator) honours: the Running-only rule, `ScanInhibit`, pause, the ≤ 50% duty clamp, and the 25% reconnect duty while any peripheral connection exists. Add native tests for any gap found (pure helpers).
- [ ] The README coexistence section records:
  - the rules table (RFC §9);
  - the `ScanInhibit` usage recipe (e.g. around an HTTPS upload);
  - "preferred vs applied" connection parameters;
  - sdkconfig coexistence notes: **measured** facts only, from WO-0 FINDINGS, and no priority claims that weren't verified;
  - an explicit note that application timer staggering is **not** a coexistence mechanism.

### 3. Documentation
- [ ] `docs/developer/bluetooth.md`: enabling (mix-in, fragment, S3-only), composition in `main.cpp`, both provider contracts with a worked example (a condensed version of the sample providers), the threading contract, events table, config table, portal/REST/MCP, preconditions, security and privacy (redaction, open GATT, slot capacity), limits (static GATT, legacy advertising only, v1 scope) and troubleshooting (nRF Connect PDU checks).
- [ ] `using-optional-modules.md`: add BLEManager to the examples list and link to the guide.
- [ ] `framework-expert/systems/ble/`: a concise skill reference (when to use which provider, the threading rules, pitfalls such as publishing from the host task, the Required-advert overflow, RPA without bond, CONFIGURING). Add BLE keywords to the `SKILL.md` description/trigger list.
- [ ] `CHANGELOG.md`: an "Added: BLEManager optional module (WITH_BLE, ESP32-S3)" entry, noting that consumers need the env mix-in + fragment and that there are no breaking changes.

## Verification Requirement
- `run_tests` native green; builds for root + sample-app `esp32s3nano_ble`; non-BLE envs unchanged; `doc_lint.py` clean.
- **Hardware (if available):** trigger a manual OTA from the System tab with a phone connected to FTMS and the HR source connected. Confirm BLE pauses and peers disconnect. Force an OTA failure (e.g. an unreachable manifest or a bad signature in a dev setup) and confirm BLE resumes: advertising returns and the HR reconnects.
- **Before committing, ask the user whether they're happy with the changes.**

## Issue Surfacing
Record any defect or concern outside your boundary box under *Out-of-Scope Findings*. Don't file issues.

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
