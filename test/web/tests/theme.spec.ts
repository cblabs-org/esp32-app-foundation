import { test, expect, Page } from '@playwright/test';

// ---------------------------------------------------------------------------
// Mock data (boot-critical only)
// ---------------------------------------------------------------------------

const MOCK_TABS = [
  { name: 'WiFi',   id: 'wifi',   parent: null,     order: 0   },
  { name: 'System', id: 'system', parent: null,     order: 100 },
  { name: 'Status', id: 'system', parent: 'System', order: 0   },
  { name: 'Keys',   id: 'keys',   parent: 'System', order: 1   },
  { name: 'MCP',    id: 'mcp',    parent: 'System', order: 2   },
  { name: 'Backup', id: 'backup', parent: 'System', order: 3   },
  { name: 'OTA',    id: 'ota',    parent: 'System', order: 4   },
];

const MOCK_SYSTEM = {
  firmware_version: '1.0.0-test',
  sdk_version: 'v5.4.1',
  chip_model: 'ESP32-S3',
  ap_mode: false,
  heap_total: 249856,
  heap_free: 124928,
  heap_min_free: 45000,
  heap_max_alloc: 110000,
  storage_used: 786432,
  storage_total: 1048576,
};

const MOCK_WIFI_STATUS = {
  mode: 'STA',
  ssid: 'TestNetwork',
  ip: '192.168.1.100',
  rssi: -55,
  passExists: true,
  ap_mode: false,
};

/** Set up all boot-critical route mocks. */
async function setupRoutes(page: Page) {
  await page.route('**/api/app', route => route.fulfill({ json: { name: 'Foundation Test App' } }));
  await page.route('**/api/tabs', route => route.fulfill({ json: MOCK_TABS }));
  await page.route('**/api/system', route => route.fulfill({ json: MOCK_SYSTEM }));
  await page.route('**/api/wifi/status', route => route.fulfill({ json: MOCK_WIFI_STATUS }));
  await page.route('**/api/wifi/scan', route => route.fulfill({ json: { networks: [] } }));
}

/** Full portal boot. */
async function bootPortal(page: Page) {
  await setupRoutes(page);
  await page.goto('/');
  await page.waitForFunction(() => (window as any).apiFetch !== undefined);
  await expect(page.locator('#portal-title')).not.toHaveText('Device Portal');
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

test.describe('Theme Toggle', () => {
  test('auto mode follows prefers-color-scheme (dark)', async ({ page }) => {
    // Emulate dark preference BEFORE loading the page
    await page.emulateMedia({ colorScheme: 'dark' });
    await bootPortal(page);

    // Auto mode should be selected
    const autoBtn = page.locator('.theme-btn[data-theme="auto"]');
    await expect(autoBtn).toHaveClass(/theme-btn--active/);

    // Since system is dark, <html> should have data-theme="dark"
    await expect(page.locator('html')).toHaveAttribute('data-theme', 'dark');
  });

  test('light mode sets data-theme correctly', async ({ page }) => {
    await bootPortal(page);

    // Click Light theme button
    await page.locator('.theme-btn[data-theme="light"]').click();

    // Light mode: the html element should NOT have data-theme="dark"
    const htmlEl = page.locator('html');
    await expect(htmlEl).not.toHaveAttribute('data-theme', 'dark');

    // The light button should be active
    await expect(page.locator('.theme-btn[data-theme="light"]')).toHaveClass(/theme-btn--active/);
    await expect(page.locator('.theme-btn[data-theme="dark"]')).not.toHaveClass(/theme-btn--active/);
  });

  test('dark mode sets data-theme="dark"', async ({ page }) => {
    await bootPortal(page);

    // Click Dark theme button
    await page.locator('.theme-btn[data-theme="dark"]').click();

    // html should have data-theme="dark"
    await expect(page.locator('html')).toHaveAttribute('data-theme', 'dark');

    // The dark button should be active
    await expect(page.locator('.theme-btn[data-theme="dark"]')).toHaveClass(/theme-btn--active/);
    await expect(page.locator('.theme-btn[data-theme="auto"]')).not.toHaveClass(/theme-btn--active/);
  });

  test('theme persists in localStorage and survives reload', async ({ page }) => {
    await bootPortal(page);

    // Set dark theme
    await page.locator('.theme-btn[data-theme="dark"]').click();

    // Verify localStorage has the value
    const stored = await page.evaluate(() => localStorage.getItem('portal-theme'));
    expect(stored).toBe('dark');

    // Reload the page — routes survive reload, localStorage persists
    await page.reload();
    await page.waitForFunction(() => (window as any).apiFetch !== undefined);
    await expect(page.locator('#portal-title')).not.toHaveText('Device Portal');

    // After reload, dark should still be active (read from localStorage by initTheme)
    await expect(page.locator('.theme-btn[data-theme="dark"]')).toHaveClass(/theme-btn--active/);
    await expect(page.locator('html')).toHaveAttribute('data-theme', 'dark');
  });

  test('toggle buttons show correct active state for each theme', async ({ page }) => {
    await bootPortal(page);

    // Default auto → auto button active
    await expect(page.locator('.theme-btn[data-theme="auto"]')).toHaveClass(/theme-btn--active/);

    // Switch to light → light active, others not
    await page.locator('.theme-btn[data-theme="light"]').click();
    await expect(page.locator('.theme-btn[data-theme="light"]')).toHaveClass(/theme-btn--active/);
    await expect(page.locator('.theme-btn[data-theme="auto"]')).not.toHaveClass(/theme-btn--active/);
    await expect(page.locator('.theme-btn[data-theme="dark"]')).not.toHaveClass(/theme-btn--active/);

    // Switch to dark → dark active, others not
    await page.locator('.theme-btn[data-theme="dark"]').click();
    await expect(page.locator('.theme-btn[data-theme="dark"]')).toHaveClass(/theme-btn--active/);
    await expect(page.locator('.theme-btn[data-theme="auto"]')).not.toHaveClass(/theme-btn--active/);
    await expect(page.locator('.theme-btn[data-theme="light"]')).not.toHaveClass(/theme-btn--active/);

    // Switch back to auto → auto active, others not
    await page.locator('.theme-btn[data-theme="auto"]').click();
    await expect(page.locator('.theme-btn[data-theme="auto"]')).toHaveClass(/theme-btn--active/);
    await expect(page.locator('.theme-btn[data-theme="light"]')).not.toHaveClass(/theme-btn--active/);
    await expect(page.locator('.theme-btn[data-theme="dark"]')).not.toHaveClass(/theme-btn--active/);
  });
});
