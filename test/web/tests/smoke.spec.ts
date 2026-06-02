import { test, expect } from '@playwright/test';

test('portal loads successfully', async ({ page }) => {
  await page.goto('/');
  await expect(page).toHaveTitle(/Portal/i);
});
