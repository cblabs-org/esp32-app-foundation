# Graveyard: Failed Playwright Wait Strategies

## Problem

The clock font test needed to wait for content rendered by `appconfig.js` in `#tab-app` before firing the radio change event.

## Failed Attempt 1: `waitFor({ state: 'visible' })` on hidden radio

```typescript
await page.locator('input[name="display-font-size"]').first().waitFor({ state: 'visible' });
```

**Error:** `locator.waitFor: Test timeout of 30000ms exceeded. 63 × locator resolved to hidden <input value="1" type="radio" name="display-font-size"/>`

**Why it failed:** The radio inputs have `style="display:none"` intentionally (they're styled via their label wrappers). Playwright correctly considers them hidden. Waiting for `visible` state will never resolve.

## Successful Strategy: `#tab-app.active` with `attached` state

```typescript
await page.locator('#tab-app.active').waitFor({ state: 'attached' });
```

**Why this works:**
- `#tab-app` is always in the DOM (defined in `index.html`)
- When the App tab is clicked, app.js adds the `active` class to `#tab-app`
- The `active` class being present means appconfig.js has been triggered to render the clock font radios
- `state: 'attached'` just checks that the element with that selector exists in the DOM — doesn't require visibility
- After `#tab-app.active` is attached, the radio inputs exist and can be `evaluate()`-d regardless of their display state
