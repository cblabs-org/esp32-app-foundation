import { test, expect } from '@playwright/test';

test('portal loads successfully', async ({ page }) => {
  await page.goto('/');
  // The /api/app endpoint returns the app name which becomes the document title
  // Wait for the page to be fully loaded by checking for apiFetch
  await page.waitForFunction(() => (window as any).apiFetch !== undefined);
  // The title is set dynamically from /api/app → name field
  const title = await page.title();
  expect(title.length).toBeGreaterThan(0);
});
