# ADR: Clock Font Test Update

## Context

The `display.spec.ts` test suite has a `beforeEach` that calls `navigateToDisplayTab(page)`, which clicks `tab-display` and waits for `#display-brightness` (in the Settings sub-pill). All tests in the `Display Settings Tab` describe block inherit this navigation.

The clock font test was originally testing a control in the App tab but was grouped under `Display Settings Tab` describe block because, pre-refactor, everything was in `appconfig.js` targeting `#tab-app` (which was called the Display/App tab).

## Decision

Rather than moving the test to a new describe block (which would require a separate `bootPortal` call and lose the shared mock setup), the clock font test explicitly navigates to `tab-app` within its own test body, overriding the `beforeEach` navigation.

## Implementation

```typescript
test('clock font selector sends POST to /api/display/clockFontSize', async ({ page }) => {
  // Clock font is in the App tab (appconfig.js), not the Display/Settings sub-pill
  await page.locator('#bottom-nav .tab-link[data-target="tab-app"]').click();
  // Wait for the App tab container to become active (content rendered by appconfig.js)
  await page.locator('#tab-app.active').waitFor({ state: 'attached' });
  ...
```

The wait strategy uses `#tab-app.active` with `state: 'attached'` because:
- `#tab-app` is always in the DOM (static HTML)
- app.js adds `active` class when the tab is activated
- `attached` is the correct state for an element that is present but may not be visible (the div itself is visible, but its internal radio inputs have `display:none`)
