# ADR: Removing the Dynamic display-toggle-styles Injection Block

## Context

`appconfig.js` `initDisplayTab()` contained a block that created a `<style id="display-toggle-styles">` element and appended it to `<head>` on first tab activation. This block contained:

1. `.toggle-switch input:checked + .toggle-slider { background: var(--primary, #2196F3); }`
2. `.toggle-switch .toggle-slider::before { ... knob CSS ... }`
3. `.toggle-switch input:checked + .toggle-slider::before { transform: translateX(22px); }`
4. `#display-font-group label:has(input:checked) { border-color: var(--primary, ...); ... }`

## Decision

Remove the entire block. Migrate all four rules to `app.css` section 9b.

## Risk: Font-Group Rule

Rule #4 (`#display-font-group label:has(input:checked)`) is used by the clock font size selector (Small / Medium / Large pills). This rule **must not be dropped**. It was explicitly migrated to section 9b in `app.css` before the injection block was removed. Verified by the passing `clock font selector` test in `display.spec.ts`.

## Safety Verification

- The `display-toggle-styles` block was guarded by `!document.getElementById('display-toggle-styles')` to prevent duplicate injection — this guard is now irrelevant since the block is gone.
- No other JS file references `display-toggle-styles` (verified by grep).
- All 13 display tests pass after removal, confirming no regressions.

## Result

`appconfig.js` no longer performs any runtime CSS injection. All styles are delivered statically via `app.css`. The tab is fully functional on first activation without requiring any script-injected styles.
