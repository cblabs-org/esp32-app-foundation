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

test('shows loading splash during load and removes when portal is ready', async ({ page }) => {
  let resolveTabs: () => void;
  const tabsGate = new Promise<void>(resolve => {
    resolveTabs = resolve;
  });

  // Delay /api/tabs to keep the portal in the loading state
  await page.route('**/api/tabs', async route => {
    await tabsGate;
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify([]),
    });
  });

  await page.goto('/');

  // Verify splash is visible with correct text, ARIA attributes, and spinner
  const splash = page.locator('#loading-splash');
  await expect(splash).toBeVisible();
  await expect(splash).toHaveAttribute('role', 'status');
  await expect(splash).toHaveAttribute('aria-live', 'polite');
  await expect(splash).toContainText('Loading, please wait…');
  await expect(splash.locator('.splash-spinner')).toBeVisible();

  // Allow boot to complete
  resolveTabs!();

  // Verify portal reaches ready state and splash is dismissed
  await expect(page.locator('body')).toHaveClass(/portal-ready/);
  await expect(splash).not.toBeAttached();
});

test('removes loading splash even if api/tabs fails during boot', async ({ page }) => {
  let rejectTabs: () => void;
  const tabsGate = new Promise<void>(resolve => {
    rejectTabs = resolve;
  });

  await page.route('**/api/tabs', async route => {
    await tabsGate;
    await route.abort('failed');
  });

  await page.goto('/');

  const splash = page.locator('#loading-splash');
  await expect(splash).toBeVisible();

  // Fail /api/tabs
  rejectTabs!();

  // Even on failure, fallback boot sequence completes and marks portal-ready
  await expect(page.locator('body')).toHaveClass(/portal-ready/);
  await expect(splash).not.toBeAttached();
});

