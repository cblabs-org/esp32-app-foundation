---
name: pr-225-ble-manager-wo4
description: Bluetooth portal tab (ble.js), /api/ble REST routes behind AuthManager, MCP tools with correct mutatesState
complexity: standard
---

# WO-4 — Portal Tab, REST Routes, MCP Tools

**Parent project:** [`pr-225-ble-manager`](../PROJECT.md) (approval provenance: PROJECT.md → *Approval Record*)
**Parent issue:** [#225](https://github.com/matthewmcneill/esp32-app-foundation/issues/225). Design: RFC rev 2, §12, §13, §15.
**Branch:** `ble-manager/wo4-portal-mcp` (base: `ble-manager/wo3-central`)

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Expose BLEManager to humans and agents:
- a **Bluetooth** portal tab (status + one device card per client provider, with Scan/Pair/Forget);
- `/api/ble/*` REST routes behind AuthManager;
- the four MCP tools, with `ble_status` read-only and the rest `mutatesState=true`.

Everything calls the public BLEManager API from WO-1…WO-3; there is no new BLE logic.

## Strict Boundary Box
- **In scope:** `modules/BLEManager/web/ble.js` (new), the `registerRoutes`/`registerTabs`/`getApiPrefix`/`registerMCPTools` implementations in `BLEManager.cpp` (plus small JSON-serialisation helpers in the module), `README.md` (portal/MCP section), and portal web tests if the repo has a pattern for them (`test/web/`).
- **Do NOT** change BLE behaviour, the provider interfaces or the WO-2/3 state machines. If an API is missing for the UI, stop and report it (only read-only accessors may be added, and must be listed in the report).
- **Do NOT** edit `web/portal/*` (core shell) or other modules' tabs. The disconnect-peer action is **out of scope** (v1.1, D11).
- Do NOT create branches.

## Tooling Constraints
- `platformio` MCP only; poll in-session. Versioning rule 03 before any flash.
- **Portal conventions:** read `docs/developer/extending-the-web-portal.md` (tab IIFE lifecycle, `tab-changed`, `utils.js` helpers `apiFetch`/`showToast`), the `framework-design-system` skill (tokens, components, the `templates/module-tab/` scaffold) and `modules/HomeAssistant/web/homeassistant.js` as the reference implementation. Use design-system classes; no inline styling beyond the conventions.
- The asset pipeline is automatic: `scripts/portalBuilder.py` → `include/portal_assets/ble_js.hpp`. Serve it with `include/utils/serveGzipAsset.hpp`.
- `house-style-docs` for C++ comments.

## Line-Item Instructions

### 1. Tab
- [ ] `registerTabs()`: `registry.addTab("Bluetooth", nullptr, <order after existing module tabs>, <Feather bluetooth icon SVG inner>, "/ble.js")`.
- [ ] `ble.js`:
  - **Status card:** state, advertised name, primary/scan-response byte budget + hex (collapsible), peripheral connections with the redacted address and *negotiated* interval, queue drop counters.
  - **Device cards** (one per client provider): label, stored device (name, redacted address, bonded yes/no), connection state / next retry.
    - Scan button (disabled when not Running or when a scan is in progress), result list (name, RSSI, redacted address), Pair per result.
    - Forget with type-to-confirm or an armed-button pattern from the design system.
  - Poll `/api/ble/status` while the tab is visible (≤ 1 Hz); stop polling when hidden.

### 2. REST (`getApiPrefix()` = `/api/ble`; kernel auth applies to `/api/*`)
- [ ] `GET /api/ble/status`: the RFC §12 content. **Full addresses only here** (authenticated); redacted everywhere else.
- [ ] `POST /api/ble/scan` `{providerId, durationMs}` → 202 / 409 (not Running, inhibited, already scanning) / 400.
- [ ] `GET /api/ble/scan/results` → `BLEScanResult[]`.
- [ ] `POST /api/ble/pair` `{providerId, address, addressType}` → 202 / 400 / 404 (unknown provider).
- [ ] `POST /api/ble/forget` `{providerId}` → 200 / 404.
- [ ] Validate every input: address format, `addressType` enum, `durationMs` ≤ 30000, `providerId` exists. Keep `getMaxPostBody()` small (e.g. 512).

### 3. MCP (`registerMCPTools`)
- [ ] `ble_status` (`mutatesState=false`): same content as REST status, but **addresses redacted** (read-only MCP mode may be exposed more widely).
- [ ] `ble_scan` (**true**), `ble_pair` (**true**), `ble_forget` (**true**): each with an explicit JSON input schema (use the `addTool` schema overload) and the same validation as REST.

### 4. Tests
- [ ] Native tests for the JSON serialisers/validators if they are pure (address/type parsing, scan-result serialisation, status serialisation with and without redaction).
- [ ] Web tests only if an existing pattern under `test/web/` covers module tabs. Otherwise note that in the report.

## Verification Requirement
- `run_tests` native green; builds for root + sample-app `esp32s3nano_ble`; non-BLE envs unchanged; `include/portal_assets/ble_js.hpp` is generated.
- **Hardware (if available):** in the portal, see the status, scan, pair the HR source, see it connect, forget it. Check with the MCP tools that `ble_status` works in read-only mode and that the mutating tools are refused in read-only mode. Take screenshots with the browser pane if available.
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
