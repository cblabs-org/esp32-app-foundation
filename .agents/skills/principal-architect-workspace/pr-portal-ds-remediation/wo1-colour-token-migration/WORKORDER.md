---
name: portal-ds-remediation-wo1
description: Handoff instructions for Work Order 1 — Colour Token Migration
complexity: mechanical
---

# WO1 — Colour Token Migration

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Replace all hard-coded hex/rgba colour values and absolute `#fff`/`#000`/`#ccc`/`#999` in JS fragment inline styles with the correct semantic CSS custom property (`var(--*)`) references. This is the single biggest remediation for design system compliance — **21 replacements across 10 files**.

## Strict Boundary Box
- **ONLY** modify files in `web/portal/`
- **ONLY** change inline style colour values and JS-assigned `.style.color` / `.style.background` properties
- Do **NOT** change CSS class names, HTML structure, component logic, or any behaviour
- Do **NOT** modify `app.css` `:root` or `[data-theme="dark"]` custom property definitions — they are correct
- Do **NOT** touch `index.html`, `app.js`, `utils.js`, `screenshot.js`, or `screenshot.html` unless listed below
- Do **NOT** modify any firmware C++ code

## Tooling Constraints
- Use the `platformio` MCP tools for any build verification if needed
- For file editing, use `replace_file_content` or `multi_replace_file_content`

## Design System Colour Token Reference

| Token | Light Value | Use For |
|---|---|---|
| `var(--primary)` | `#2196F3` | Brand accent, links, active states |
| `var(--primary-hover)` | `#1976D2` | Hover states |
| `var(--primary-alpha)` | `rgba(33,150,243,0.1)` | Tinted backgrounds |
| `var(--success)` | `#2e7d32` | Success states, connected, green |
| `var(--error)` | `#c62828` | Error states, disconnected, red |
| `var(--warning)` | `#f9a825` | Warning states, amber |
| `var(--text)` | `#212529` | Primary text |
| `var(--text-muted)` | `#6c757d` | Secondary/muted text |
| `var(--bg)` | `#ffffff` | Page background |
| `var(--card-bg)` | `#f8f9fa` | Card surface |
| `var(--border)` | `#dee2e6` | Borders, dividers |

## Line-Item Instructions

### Category A: Off-palette hex colours → semantic tokens

- [ ] `web/portal/ota.js` **line 33** — `background:#ccc` → `background:var(--border)`
- [ ] `web/portal/ota.js` **line 48** — `background:#ccc` → `background:var(--border)`
- [ ] `web/portal/ota.js` **line 171** — `'#ccc'` → `'var(--border)'`
- [ ] `web/portal/mcp.js` **line 63** — `'#ccc'` → `'var(--border)'`
- [ ] `web/portal/mcp.js` **line 79** — `'#999'` → `'var(--text-muted)'`  
  AND on the same line: `'#ff8f00'` → `'var(--warning)'`
- [ ] `web/portal/mcp.js` **line 80** — `'#999'` → `'var(--text-muted)'`  
  AND on the same line: `'#43a047'` → `'var(--success)'`
- [ ] `web/portal/mcp.js` **line 129** — `'#ccc'` → `'var(--border)'`
- [ ] `web/portal/credits.js` **line 36** — `#6c63ff` → `var(--primary)` (remove the `var(--accent,...)` fallback pattern — just use `var(--primary)`)
- [ ] `web/portal/credits.js` **line 55** — `#6c63ff` → `var(--primary)` (same — simplify to `color:var(--primary)`)
- [ ] `web/portal/time.js` **line 87** — `'#999'` → `'var(--text-muted)'`

### Category B: Hard-coded `#fff` / `#000` → semantic tokens

These are text colours on dynamically styled badges/buttons where the text must contrast with a coloured background. The correct pattern depends on context:
- If the background is a semantic colour (success/error/warning), use `color: #fff` via a CSS class rather than inline — BUT since these are dynamically generated elements, the pragmatic fix is `var(--bg)` which is `#ffffff` in light and `#121212` in dark. **However**, for text on a coloured badge background that needs to be white in both themes, keep `#fff` — it's not a token violation, it's intentional contrast.

**Decision rule:** If the element has a coloured background (green/red/blue badge), `#fff` is correct contrast — leave it. If the element uses `var(--*)` background tokens, the text should also use tokens.

Review each instance and apply this rule:

- [ ] `web/portal/wifi.js` **line 185** — `'#000'` on a mode badge. Review: if badge background is a light colour, this is contrast text → **change to `var(--text)`** (adapts to theme)
- [ ] `web/portal/wifi.js` **line 189** — `'#fff'` on a mode badge with coloured background → **keep `#fff`** (intentional contrast on coloured bg)
- [ ] `web/portal/wifi.js` **line 193** — `'#fff'` on a mode badge with coloured background → **keep `#fff`**
- [ ] `web/portal/wifi.js` **line 510** — `'#fff'` on a `forgetBtn` with danger background → **keep `#fff`**
- [ ] `web/portal/system.js` **line 274** — `'#fff'` on `restartBtn` with danger background → **keep `#fff`**
- [ ] `web/portal/credits.js` **line 37** — `color:#fff` on a licence badge with coloured background → **keep `#fff`**
- [ ] `web/portal/display.js` **line 252** — `'#fff'` on a status badge → review background. If coloured → **keep**. If using `var(--*)` bg → **change to `var(--bg)`**
- [ ] `web/portal/display.js` **line 256** — `'#000'` on a status badge → **change to `var(--text)`**
- [ ] `web/portal/display.js` **line 260** — `'#fff'` on a status badge → review background → **keep if coloured bg**
- [ ] `web/portal/keys.js` **line 123** — `'#fff'` on a button with coloured background → **keep `#fff`**
- [ ] `web/portal/utils.js` **line 76** — `'#fff'` on a button with coloured background → **keep `#fff`**

**Important:** For the items marked "keep `#fff`", add a comment `/* intentional: contrast on coloured bg */` after the value so future audits don't flag them.

## Verification Requirement
1. After all changes, run a grep audit:
   ```
   grep -rn '#ccc\|#999\|#6c63ff\|#ff8f00\|#43a047' web/portal/
   ```
   This should return **zero results**.
2. Visually inspect that no colours broke by reviewing the inline style context around each change.
3. **CRITICAL:** Before committing, you MUST ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

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
