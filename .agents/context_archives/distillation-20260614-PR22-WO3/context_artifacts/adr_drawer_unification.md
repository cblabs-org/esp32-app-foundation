# ADR: Portal Drawer Unification (WO3)

## Decision: Native `<details>`/`<summary>` over JS-driven pattern

**Why:** The old `.drawer`/`.drawer-trigger` CSS class pattern used JS to toggle
`.active` class and `max-height`. Native `<details>` is semantically correct,
provides `aria-expanded` automatically, requires zero JS for toggle, and is the
W3C/browser baseline. The JS pattern was confirmed dead code (zero HTML references).

## Decision: CSS border chevron over Unicode `▶`

**Why:** U+25B6 (▶) is a filled glyph — visually heavy, font-dependent, reads
as a "play button". All major design systems (Bootstrap 5, GitHub Primer, Adobe
Spectrum, Material Design 3) use SVG or CSS border-based open chevrons. The CSS
border trick (`width/height box + border-right/bottom + rotate`) is:
- GPU-composited (no layout/paint on transition)
- Inherits `currentColor` (works for both yellow advanced-drawer and muted info-drawer)
- Zero font dependency

**Direction:** Right→Down (−45°→+45°), NOT Down→Up (Material/Android 180° flip).
Rationale: our drawers use native `<details>` whose browser default is Right→Down.
Spectrum, GitHub, Carbon all use Right→Down for disclosure controls. Down→Up is
better for full-page app accordions where you signal scroll direction.

## Decision: Two drawer variants (advanced-drawer vs info-drawer)

**Why:** OTA and Time drawers are "danger zone" controls (advanced settings that
can break things). WiFi diagnostics is informational only. Yellow warning colouring
on the diagnostics drawer would be misleading. Split:
- `article.advanced-drawer` — yellow border, warning-coloured label, bold weight
- `article.info-drawer` — neutral border, muted label, normal weight

## Decision: Status dot left of label in info-drawer summary

**Why:** `● Network Diagnostics ❯` reads as status→label→action. The dot tells
you the connection state before you process the label. Right placement
(old: `Network Diagnostics ● ❯`) puts the dot between the label and chevron,
competing with the chevron as a trailing affordance.

## A11y additions applied to both variants
- `prefers-reduced-motion`: `transition: none` — WCAG 2.3.3
- `focus-visible` outline: keyboard tab → Enter/Space works with visible ring
