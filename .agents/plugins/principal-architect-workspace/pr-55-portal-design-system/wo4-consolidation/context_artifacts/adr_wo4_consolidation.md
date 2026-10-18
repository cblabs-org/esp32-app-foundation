# ADR: WO4 Consolidation & Signal Bars Promotion

## 1. Card Grid System (RFC #2–5, #13)
Consolidated into BEM card modifiers (`.card--interactive`, `.card--empty`) and a CSS grid container (`.card-grid`, `.card-grid--2col`). Responsive fallback breaks to 1 column below 400px screen width.

## 2. Dynamic Dot Sizing (RFC #6)
Refactored `.dot` from hardcoded 10px dimensions to CSS variable `--dot-size, 10px`. Scaled `box-shadow` values dynamically using CSS `calc()` so glowing effects scale proportionally.

## 3. Subtle Badge Modifier (RFC #7)
Added `.badge--subtle` modifier to existing `.badge` block rather than introducing new `.chip` or `.type-badge` classes to prevent component inflation.

## 4. Row Component Reconciliation (RFC #11)
Maintained `.info-row` (read-only key-value pair) and `.compact-row` (interactive input+button pair) as distinct components based on semantic usage. Explicitly chose not to introduce `.list-item` as its functionality is fully covered by `.info-row`.

## 5. Signal Bars Promotion (RFC #15 + U2)
Promoted local `signalBarsIcon()` and `rssiToBars()` from `wifi.js` to `window.renderSignalBars(rssi)` in `utils.js`. Added an explicit accessibility label contract (`aria-label="Signal: N of 4 bars"`).
