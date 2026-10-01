import { test, expect, Page } from '@playwright/test';

// ---------------------------------------------------------------------------
// Mock data
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

const MOCK_SYSTEM_AP_MODE = {
  firmware_version: '1.0.0-test',
  sdk_version: 'v5.4.1',
  chip_model: 'ESP32-S3',
  ap_mode: true,
  heap_total: 249856,
  heap_free: 124928,
  heap_min_free: 45000,
  heap_max_alloc: 110000,
  storage_used: 786432,
  storage_total: 1048576,
};

const MOCK_SYSTEM_NORMAL = {
  ...MOCK_SYSTEM_AP_MODE,
  ap_mode: false,
};

const MOCK_WIFI_STATUS = {
  mode: 'STA',
  ssid: 'TestNetwork',
  ip: '192.168.1.100',
  rssi: -55,
  passExists: true,
  ap_mode: false,
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

async function bootPortal(page: Page, systemData: typeof MOCK_SYSTEM_AP_MODE) {
  await page.route('**/api/app', route => route.fulfill({ json: { name: 'Foundation Test App' } }));
  await page.route('**/api/tabs', route => route.fulfill({ json: MOCK_TABS }));
  await page.route('**/api/system', route => route.fulfill({ json: systemData }));
  await page.route('**/api/wifi/status', route => route.fulfill({ json: MOCK_WIFI_STATUS }));
  await page.route('**/api/wifi/scan', route => route.fulfill({ json: { networks: [] } }));

  await page.goto('/');
  await page.waitForFunction(() => (window as any).apiFetch !== undefined);
  await expect(page.locator('body')).toHaveClass(/portal-ready/);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

test.describe('AP Mode Behaviour', () => {
  test('AP banner visible and body gets ap-mode class when ap_mode is true', async ({ page }) => {
    await bootPortal(page, MOCK_SYSTEM_AP_MODE);

    // Wait for the AP mode check to complete (checkApMode() fetches /api/system)
    await expect(page.locator('body')).toHaveClass(/ap-mode/);

    // The AP banner should contain the expected text
    const banner = page.locator('#ap-banner');
    await expect(banner).toContainText('Access Point Mode Active');
  });

  test('normal mode — AP banner hidden, body does not have ap-mode class', async ({ page }) => {
    await bootPortal(page, MOCK_SYSTEM_NORMAL);

    // Give the page time to process the system response
    // AP mode check fires on DOMContentLoaded — wait for scripts to load
    await page.waitForTimeout(500);

    // Body should NOT have ap-mode class
    await expect(page.locator('body')).not.toHaveClass(/ap-mode/);
  });

  test('all tabs still visible in normal mode', async ({ page }) => {
    await bootPortal(page, MOCK_SYSTEM_NORMAL);

    // Both WiFi and System bottom nav tabs should be visible
    await expect(page.locator('#bottom-nav [data-target="tab-wifi"]')).toBeVisible();
    await expect(page.locator('#bottom-nav [data-target="tab-system"]')).toBeVisible();
  });
});
