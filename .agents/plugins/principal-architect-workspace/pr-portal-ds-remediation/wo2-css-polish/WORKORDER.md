---
name: portal-ds-remediation-wo2
description: Handoff instructions for Work Order 2 — CSS Polish (monospace stack)
complexity: mechanical
---

# WO2 — CSS Polish

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Fix the three incomplete monospace font-family declarations in `app.css` to match the design system's `--font-mono` token. This is a tiny, surgical WO — **3 replacements in 1 file**.

## Strict Boundary Box
- **ONLY** modify `web/portal/app.css`
- **ONLY** change `font-family` declarations for monospace
- Do **NOT** change any other CSS properties, custom properties, or selectors
- Do **NOT** touch any JS files, HTML, or firmware code

## Tooling Constraints
- Use `multi_replace_file_content` for efficiency (3 non-contiguous edits in one file)

## Design System Reference

The DS specifies this monospace stack:
```css
--font-mono: "Courier New", ui-monospace, SFMono-Regular, Menlo, monospace;
```

The portal currently uses an incomplete version:
```css
font-family: 'Courier New', monospace;
```

## Line-Item Instructions

- [ ] `web/portal/app.css` **line 783** — Change `font-family: 'Courier New', monospace;` → `font-family: 'Courier New', ui-monospace, SFMono-Regular, Menlo, monospace;`
- [ ] `web/portal/app.css` **line 931** — Same change
- [ ] `web/portal/app.css` **line 1116** — Same change

## Verification Requirement
1. Run: `grep -n "font-family.*Courier" web/portal/app.css` — all 3 lines should now show the expanded stack.
2. Confirm no other `font-family` declarations were accidentally modified.
3. **CRITICAL:** Before committing, you MUST ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
(pending)

### Changes Made
(pending)

### Decisions Made
(pending)

### Dead Ends & Warnings
(pending)

### Out-of-Scope Findings
(pending)

### Blockers
(pending)
