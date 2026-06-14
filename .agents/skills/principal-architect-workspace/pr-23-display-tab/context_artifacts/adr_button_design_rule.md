# ADR: Portal Button Design Rule — Filled vs Outline

## Decision

**Filled (default `button`)** = the single primary action per card — what the user came to do.  
**Outline (`button.outline`)** = secondary/supporting actions — available, but not the default intent.  
**Secondary (`button.secondary`)** = neutral/rarely-used actions.

## CSS Source (`app.css`)

```css
button {
  background: var(--primary);   /* filled blue */
  color: white;
}

button.outline {
  background: transparent;
  color: var(--primary);
  border: 2px solid var(--primary);
}

button.secondary {
  background: var(--surface);
  color: var(--text);
}
```

## Application in PR-23

### Settings sub-pill (display.js)
- **Wake Display** — `class="outline"` ✅ (equal-weight reversible command, paired with Sleep)
- **Sleep Display** — `class="outline"` ✅ (equal-weight reversible command, paired with Wake)
- *Rationale:* Both are operational commands of equal weight. Neither is "the reason the user is here". Using outline for both avoids visual competition in a side-by-side layout.

### Screenshot sub-pill (screenshot.js)
- **Capture** — filled (default) ✅ (primary intent — user comes here to capture)
- **Download PNG** — `class="outline"` ✅ (secondary — only available post-capture, exports file)
- *Initial bug:* Download was also rendered filled, creating two competing primary buttons. Fixed in `564b0b2`.

## Pixel Colour Preset Buttons (screenshot.js)

Small preset buttons (Amber/White/Green) use `outline` base + `.active` for the selected state. `.btn-ss-colour.active` is defined in `app.css` to mirror `.pill-btn.active` (filled primary). This gives clear selection feedback without the presets competing with the main Capture/Download row.
