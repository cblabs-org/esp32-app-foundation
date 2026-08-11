---
name: portal-ds-remediation-wo3
description: Handoff instructions for Work Order 3 — Icon Remediation (emoji + sizing)
complexity: mechanical
---

# WO3 — Icon Remediation

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Replace all emoji-as-icons in `keys.js` with proper Feather inline SVGs, and fix all non-standard SVG icon sizes to use the canonical 24×24 dimensions. **15 replacements across 6 files.**

## Strict Boundary Box
- **ONLY** modify the files listed in the checklist below
- **ONLY** change icon markup (SVG attributes, emoji → SVG replacements)
- Do **NOT** change component logic, CSS classes, colours, or any behaviour
- Do **NOT** modify any firmware C++ code

## Tooling Constraints
- Use `replace_file_content` or `multi_replace_file_content` for file editing

## Design System Icon Rules

All icons MUST be:
- **Feather Icons** — inline SVG
- `viewBox="0 0 24 24"` (always)
- `width="24" height="24"` (canonical size — exceptions noted below)
- `fill="none"` (stroke-only)
- `stroke="currentColor"` (inherits text colour)
- `stroke-width="2"`
- `stroke-linecap="round"`
- `stroke-linejoin="round"`

**No emoji-as-icons. No PNG icons. No icon fonts.**

### Size Exception Policy
Some icons are used inline in compact contexts (password-reveal eye in an input field, small lock badges). For these, the **viewBox stays 24×24** but `width`/`height` can be reduced. The PA has decided:
- **20×20** is acceptable for inline input-field icons (eye toggle) — but all SVG attributes must still be present
- **14×14** is acceptable for micro-badge icons — but all SVG attributes must still be present
- **48×48** for the auth login card hero icon is **not acceptable** — this should be styled via CSS (`width: 48px; height: 48px` on the SVG element) while keeping the SVG at `width="24" height="24"` with `viewBox="0 0 24 24"`

### Feather Icon SVG Paths Reference

**Lock** (🔒 replacement):
```html
<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0110 0v4"></path></svg>
```

**Link** (🔗 replacement):
```html
<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M10 13a5 5 0 007.54.54l3-3a5 5 0 00-7.07-7.07l-1.72 1.71"></path><path d="M14 11a5 5 0 00-7.54-.54l-3 3a5 5 0 007.07 7.07l1.71-1.71"></path></svg>
```

**Trash-2** (🗑 replacement):
```html
<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="3 6 5 6 21 6"></polyline><path d="M19 6v14a2 2 0 01-2 2H7a2 2 0 01-2-2V6m3 0V4a2 2 0 012-2h4a2 2 0 012 2v2"></path><line x1="10" y1="11" x2="10" y2="17"></line><line x1="14" y1="11" x2="14" y2="17"></line></svg>
```

For inline/compact usage, scale down `width`/`height` to match context (e.g. `width="16" height="16"` for small badges) while keeping `viewBox="0 0 24 24"`.

## Line-Item Instructions

### Part A: Emoji → Feather SVG replacements in `keys.js`

- [ ] `web/portal/keys.js` **line 65** — Replace comment `🔒` with `lock` and `🔗` with `link` (just the comment text — not functional)
- [ ] `web/portal/keys.js` **line 67** — Replace `🔒` emoji with an inline Feather `lock` SVG at 16×16 (compact badge context): `<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0110 0v4"></path></svg>`
- [ ] `web/portal/keys.js` **line 70** — Replace `🔗` emoji with an inline Feather `link` SVG at 16×16
- [ ] `web/portal/keys.js` **line 79** — Replace `🗑 Delete` with inline Feather `trash-2` SVG at 16×16 + ` Delete`
- [ ] `web/portal/keys.js` **line 114** — Replace `🗑 Delete` with matching SVG + text (same as line 79)
- [ ] `web/portal/keys.js` **line 127** — Replace `🗑 Delete` with matching SVG + text (same as line 79)

### Part B: Non-standard SVG sizes

- [ ] `web/portal/wifi.js` **line 100** — Eye icon at `width="20" height="20"` — this is an input-field toggle, 20×20 is acceptable. **Verify all other SVG attributes** (`fill="none"`, `stroke="currentColor"`, `stroke-width="2"`, `stroke-linecap="round"`, `stroke-linejoin="round"`) are present. Add any that are missing.
- [ ] `web/portal/wifi.js` **line 327** — Lock icon at `width="14" height="14"` — micro-badge context, 14×14 is acceptable. **Verify all SVG attributes** are present.
- [ ] `web/portal/auth.js` **line 22** — Login hero icon at `width="48" height="48"` — Change to `width="24" height="24"` and add CSS inline style `style="width:48px;height:48px"` to achieve the visual size via CSS rather than SVG attributes. Ensure all SVG attributes are present.
- [ ] `web/portal/auth.js` **line 225** — Eye icon at `width="20" height="20"` — password field toggle, 20×20 acceptable. **Verify all SVG attributes** are present.
- [ ] `web/portal/mcp.js` **line 182** — Eye icon at `width="20" height="20"` — **Verify all SVG attributes** present.
- [ ] `web/portal/mcp.js` **line 188** — Copy icon at `width="20" height="20"` — **Verify all SVG attributes** present.
- [ ] `web/portal/credits.js` **line 42** — GitHub icon at `width="18" height="18"` — compact inline context, 18×18 acceptable. **Verify all SVG attributes** present.
- [ ] `web/portal/index.html` **line 19** — Icon at `width="20" height="20"` — header context. **Change to `width="24" height="24"`** (standard size for header icons). Verify all SVG attributes present.
- [ ] `web/portal/utils.js` **line 144** — Signal bars icon at `width="18" height="16"` with non-standard `viewBox="0 0 17 16"` — this is a custom (non-Feather) signal strength icon. **Leave as-is** — it's a bespoke path, not a Feather icon, and the viewBox matches its geometry.

## Verification Requirement
1. Run: `grep -rn '🔒\|🔗\|🗑' web/portal/` — should return **zero** results.
2. For each modified SVG, verify the complete attribute set is present by visual inspection.
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
