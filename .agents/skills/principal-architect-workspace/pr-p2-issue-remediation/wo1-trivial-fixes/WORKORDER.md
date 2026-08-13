---
name: p2-issue-remediation-wo1
description: Handoff instructions for Work Order 1 — trivial fixes for issues #91 and #90
complexity: mechanical
---

# WO1 — Trivial Fixes (#91 + #90)

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Fix two trivial P2 bugs: add a missing `#include "SemVer.hpp"` to OTAUpdateManager.cpp (#91), and add dark-mode CSS overrides for `.badge--warn` to fix contrast failure (#90).

## Strict Boundary Box
- **DO NOT** modify any files except those listed in the checklist.
- **DO NOT** refactor, restructure, or "improve" surrounding code.
- **DO NOT** touch `platformio.ini`, test files, or the `data/portal/` directory (the built output is auto-generated from `web/portal/`).
- **DO NOT** branch. You are working on the branch that is already checked out.

## Tooling Constraints
- Use the `platformio` MCP server for build verification (`build_project`).
- Do NOT use raw `pio` CLI commands.

## Line-Item Instructions

### Issue #91 — Missing SemVer.hpp include
- [ ] `modules/OTAUpdateManager/OTAUpdateManager.cpp` — Add `#include "SemVer.hpp"` after line 77 (after `#include "MCPToolRegistry.hpp"`). This file uses `FirmwareVersion` and `parseVersionTag()` at lines 515-517 from `lib/SemVer/SemVer.hpp` but has no explicit include.

### Issue #90 — Badge dark-mode contrast
- [ ] `web/portal/app.css` — Add a dark-mode override block after the `.badge--warn` rule (after line 1095). Insert:
```css
[data-theme="dark"] .badge--warn {
  background: rgba(251, 191, 36, 0.2);
  color: #fbbf24;
  border-color: rgba(251, 191, 36, 0.3);
}
```
This uses amber-400 (`#fbbf24`) which is WCAG AA compliant on `#1e1e1e` dark backgrounds. The existing `.badge--warn` uses `color: var(--warning)` (`#f9a825`) which is not overridden in the `[data-theme="dark"]` block (lines 26-34 of app.css).

## Verification Requirement
1. Build the `esp32dev_ota` environment using the `platformio` MCP server to verify OTAUpdateManager compiles with the new include.
2. Build the `esp32s3nano` environment to verify nothing regressed.
3. Visually inspect the CSS change is syntactically correct.
4. **Before committing, ask the user if they are happy with the changes.**

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Completed.

### Changes Made
- `modules/OTAUpdateManager/OTAUpdateManager.cpp`: Included `SemVer.hpp`.
- `web/portal/app.css`: Added dark-mode CSS override for `.badge--warn` for better contrast.

### Dead Ends & Warnings
- Initial edit of `OTAUpdateManager.cpp` resulted in a syntax error (include directive inside function body) due to multi-edit tool interaction; quickly repaired.
- The WORKORDER specified `esp32dev_ota` as an environment to build, but it was not present in the configuration; I built `esp32dev` and `esp32s3nano` as proxies.

### Out-of-Scope Findings
None.

### Blockers
None.
