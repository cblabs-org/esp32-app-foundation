# ADR: CSS Specificity on Touch Devices — Pill Active State

## Problem

On the CrowPanel touch screen (and any mobile/touch device), tapping a sub-pill button caused the text to become invisible — blue text on a blue background.

## Root Cause

CSS specificity mismatch between `:hover` and `--active` class:

```css
.pill-btn:hover     { color: var(--primary); }  /* specificity: 0,2,0 */
.pill-btn--active   { color: white; }            /* specificity: 0,1,0 — LOSES */
```

On touch devices, `:hover` is "sticky" — it persists after a tap until the user taps elsewhere. When a pill receives both `.pill-btn--active` (white text) AND `:hover` (blue text), the higher-specificity `:hover` wins, producing `color: var(--primary)` on `background: var(--primary)`.

The initial pill (set programmatically on page load without a tap) looked correct because `:hover` was never triggered.

## Fix

Add `.pill-btn--active:hover` alongside `.pill-btn--active` so the active state always wins:

```css
.pill-btn--active,
.pill-btn--active:hover {
  background: var(--primary);
  color: white;
  border-color: var(--primary);
}
```

Both selectors now have specificity 0,2,0, and source order ensures the active rule wins.

## Design Rule

**Any interactive element with a `:hover` colour change MUST also have an `--active:hover` override** to prevent specificity conflicts on touch devices. This applies to all pill-like components, toggle buttons, and similar interactive elements.
