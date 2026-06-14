# ADR: Canonical Toggle CSS in app.css (Section 9b)

## Context

Two toggle switches in the Display tab (Flip Screen, Screensaver Enable) appeared permanently grey regardless of their checked state. The firmware correctly returned `true`/`false` and the checkbox `checked` attribute was being set, but the visual colour never updated.

## Root Cause

The toggle HTML in `appconfig.js` rendered as:
```html
<span class="toggle-slider" style="background:var(--border, #dee2e6); position:absolute; ..."></span>
```

The dynamically-injected CSS rule `.toggle-switch input:checked + .toggle-slider { background: var(--primary) }` was overridden by the inline `style` attribute on the `<span>`. **Inline styles always win** over class selectors regardless of `:checked` state — this is fundamental CSS specificity (specificity weight: inline = 1000, class = 10).

## Decision

Move all toggle CSS to a canonical, static section in `app.css` (section 9b). The CSS class rules in a stylesheet have lower specificity than inline styles — but the fix is to **remove the inline `background`** from the span, not to escalate specificity with `!important`.

## Why NOT `!important`

Using `!important` in the class rule would work but is brittle: it would override any intentional inline overrides and is hard to debug. Removing the inline attribute is the correct, clean fix.

## Why NOT keep the dynamic injection

The dynamic `<style>` injection was idempotent but fragile:
- Order-dependent: injected only on first tab activation
- Invisible to CSS linters and dev-tools stylesheet inspectors
- Redundant once the inline override is removed

## Result

Section 9b in `app.css` contains the full toggle component definition. The `.toggle-slider` class now controls background colour. The `::before` knob rules and the `input:checked` state rules are all co-located and statically delivered with the page.
