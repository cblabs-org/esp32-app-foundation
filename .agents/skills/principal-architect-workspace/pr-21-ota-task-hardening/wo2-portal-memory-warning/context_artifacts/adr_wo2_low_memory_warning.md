# ADR: WO2 — Low-Memory OTA Warning Banner

## Decision: Advisory-only, non-blocking warning

**Context:**  
When free DRAM (`dram_free_bytes`) is below the OTA task stack requirement (`ota_stack_bytes` = 32768 bytes),
the FreeRTOS OTA task may fail to launch. WO1 added these two fields to the `POST /api/ota {"action":"status"}`
response. WO2's job is to surface this condition in the portal UI.

**Decision:**  
The warning is purely advisory. It does **not** disable `btn-ota-check` or any other control.
PSRAM fallback is in place on the firmware side, so low DRAM is not an absolute hard block —
it's a "heads up" that task creation may be delayed or fail under extreme conditions.

**Why advisory-only?**  
- The firmware has a PSRAM fallback path; blocking the UI would be overly conservative.  
- Principal Architect spec explicitly states: "The warning must be rendered on page load and must
  not disable the 'Check for Updates' button."  
- Disabling the button would confuse users whose device actually works fine (PSRAM compensates).

## Decision: Inline `display:none` default, toggled in `loadStatus()`

**Context:**  
The warning div must not flash briefly on initial render before data loads.

**Decision:**  
The `<div id="ota-low-memory-warning">` carries `style="display:none"` as an inline attribute
in the HTML template. `loadStatus()` sets `warningEl.style.display = 'block'` or `'none'`
after the API response is received.

**Why not CSS class toggle?**  
- `app.css` is explicitly out of scope (strict boundary: touch only `ota.js`).  
- Inline style toggling is self-contained within the single permitted file.

## Decision: Guard clauses for backward compatibility

**Context:**  
Devices running firmware without WO1 will not have `dram_free_bytes` or `ota_stack_bytes` in
their status response.

**Decision:**  
Both element existence (`warningEl && detailEl`) and API field existence
(`ota.dram_free_bytes !== undefined && ota.ota_stack_bytes !== undefined`) are checked before
any access. If either is missing, the block silently no-ops — the banner stays hidden.

## Placement in `loadStatus()`

The warning rendering block is placed **after** the Card 2 settings block but **inside the `try`
block**, so it only runs if both API calls succeeded. If the status call fails, the `catch` block
renders the firmware error and the warning banner remains hidden (correct behaviour).
