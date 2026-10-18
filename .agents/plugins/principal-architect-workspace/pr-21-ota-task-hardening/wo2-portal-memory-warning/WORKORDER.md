---
name: pr-21-wo2
description: Handoff instructions for Work Order 2 — Portal low-memory OTA warning banner
complexity: standard
---

# pr-21-ota-task-hardening / WO2 — Portal Low-Memory OTA Warning

**Active branch:** `pr-21-ota-task-hardening-wo2-portal-warning`

You are an expert senior software engineer and execution-focused worker agent.
**DO NOT trigger or act as the principal-architect.**
I need you to completely execute Work Order 2.

---

## Objective

Add a visible, non-blocking advisory warning to the portal OTA tab when the device reports
insufficient free DRAM to guarantee successful OTA task creation. The warning must be rendered
on page load and must not disable the "Check for Updates" button.

---

## Context (read before touching any file)

WO1 added two new fields to the `POST /api/ota {"action":"status"}` response:

| Field | Type | Meaning |
|-------|------|---------|
| `dram_free_bytes` | `uint32_t` | Current free internal DRAM at query time |
| `ota_stack_bytes` | `uint32_t` | Stack bytes required for the OTA FreeRTOS task (32768) |

When `dram_free_bytes < ota_stack_bytes`, OTA may fail to launch (though PSRAM fallback is
also in place — this warning is advisory, not a hard block).

---

## Strict Boundary Box

- **Touch ONLY**: `web/portal/ota.js`
- **Do NOT** modify any firmware `.cpp`/`.hpp` file, `app.css`, `index.html`, or any other
  portal `.js` file.
- **Do NOT** make architectural decisions — the design is specified below.
- **Do NOT** branch. You will be spawned into an already-branched workspace
  (`pr-21-ota-task-hardening/wo2-portal-memory-warning`).

---

## Checklist

### Step 1 — Add the warning banner HTML to Card 3 (Updates) scaffold

In the `container.innerHTML = \`...\`` template literal, locate **Card 3 (Updates)**
(the card containing `<button id="btn-ota-check">`).

Insert the following HTML **between** the card's `<header>Updates</header>` tag and the
`<button id="btn-ota-check">` button, as the first element inside the card:

```html
<!-- Low-memory advisory — shown when free DRAM < OTA stack threshold -->
<div id="ota-low-memory-warning"
  style="display:none; background:rgba(249,168,37,0.12); border:1px solid var(--warning,#f9a825);
         border-radius:8px; padding:0.6rem 0.8rem; margin-bottom:0.8rem; font-size:0.82rem;
         color:var(--warning,#f9a825); line-height:1.4;">
  ⚠️ <strong>Low memory</strong> — OTA task may fail to start.<br>
  <span id="ota-low-memory-detail"></span>
</div>
```

### Step 2 — Update `loadStatus()` to read and render the warning

In the `loadStatus()` async function (approximately line 172), **after** the existing Card 1
and Card 2 rendering logic (after the `infoEl.innerHTML = rows;` line), add the following
low-memory warning rendering block:

```javascript
// --- Card 3: Low-memory advisory ---
const warningEl = document.getElementById('ota-low-memory-warning');
const detailEl  = document.getElementById('ota-low-memory-detail');
if (warningEl && detailEl && ota.dram_free_bytes !== undefined && ota.ota_stack_bytes !== undefined) {
  const freeMem = ota.dram_free_bytes;
  const required = ota.ota_stack_bytes;
  if (freeMem < required) {
    const freeKB    = Math.round(freeMem / 1024);
    const requiredKB = Math.round(required / 1024);
    detailEl.textContent =
      `Free DRAM: ${freeKB} KB — Required: ${requiredKB} KB.` +
      ` Device has PSRAM fallback enabled but low DRAM may cause task delays.`;
    warningEl.style.display = 'block';
  } else {
    warningEl.style.display = 'none';
  }
}
```

This block must be inside the `try` block of `loadStatus()`, after `infoEl.innerHTML = rows;`.

### Step 3 — Verify the Check for Updates button is NOT disabled by this code

Search `ota.js` for any code that sets `btn-ota-check` disabled in the `loadStatus` path and
confirm none was introduced. The warning is purely advisory — the button remains enabled.

---

## Verification Requirement

1. Read through the final `loadStatus()` function in `ota.js` and confirm:
   - The `ota-low-memory-warning` div is set to `display:block` only when
     `ota.dram_free_bytes < ota.ota_stack_bytes`.
   - The `btn-ota-check` button's `disabled` attribute is **not** set in `loadStatus()`.
   - The detail text correctly formats free DRAM and required DRAM in KB.
2. There are no JavaScript syntax errors (review the code for unclosed braces, missing
   semicolons, or malformed template literals).
3. The `ota-low-memory-warning` div is hidden by default (`display:none`) and only appears
   when the condition is met — it must not flash on initial render.

No hardware or build step is required — this is a pure JavaScript file change.

---

## Done Condition

- `web/portal/ota.js` contains the warning banner HTML and the rendering logic in `loadStatus()`.
- The button is not blocked.
- **STOP and send a message back to me (the Principal Architect)** with a summary of changes
  and confirmation that the verification steps above pass.
  Do not invoke `@distillery` or wrap up until I explicitly approve the work.

Once approved, invoke the `@distillery` skill passing
`.agents/skills/principal-architect-workspace/pr-21-ota-task-hardening/wo2-portal-memory-warning/`
as the target directory. Then append a status update to this WORKORDER.md and terminate.

---

## STATUS UPDATE — 2026-06-11T21:01:00Z

**Status:** ✅ COMPLETE  
**Executed by:** Execution Worker subagent (session `13f55a55-ef7d-4889-aed7-e60211caba10`)  
**Approved by:** Principal Architect (session `8c58afa2-8ca2-4a85-b127-3fb94f785271`)

### Changes delivered
- `web/portal/ota.js`: Two-part edit on branch `pr-21-ota-task-hardening-wo2-portal-warning`
  1. Added `<div id="ota-low-memory-warning">` (hidden by default, `display:none`) inside Card 3,
     between `<header>Updates</header>` and `<button id="btn-ota-check">`.
  2. Added low-memory advisory rendering block in `loadStatus()` (inside `try`, after Card 2 block)
     that reads `ota.dram_free_bytes` / `ota.ota_stack_bytes` and toggles the banner.

### Verification passed
- `node --check web/portal/ota.js` → `SYNTAX OK`
- `btn-ota-check` is never disabled by the new code path (grep returns zero matches)
- Warning hidden by default; condition `dram_free_bytes < ota_stack_bytes` correctly gates display
- Backward-compat guards in place for devices without WO1 fields

### Distillery artifacts written
- `.agents/skills/principal-architect-workspace/pr-21-ota-task-hardening/wo2-portal-memory-warning/context_bridge.md`
- `.agents/skills/principal-architect-workspace/pr-21-ota-task-hardening/wo2-portal-memory-warning/sessions.md`
- `.agents/skills/principal-architect-workspace/pr-21-ota-task-hardening/wo2-portal-memory-warning/context_artifacts/adr_wo2_low_memory_warning.md`
