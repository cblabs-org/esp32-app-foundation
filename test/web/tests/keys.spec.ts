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

const MOCK_KEYS = [
  { id: 'weather_key',   hasValue: true },
  { id: 'wifi_password', hasValue: true, managedBy: 'WiFi', exclusive: true },
  { id: 'api_token',     hasValue: true },
];

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

async function bootAndNavigateToKeys(page: Page, keys = MOCK_KEYS) {
  await page.route('**/api/app', route => route.fulfill({ json: { name: 'Foundation Test App' } }));
  await page.route('**/api/tabs', route => route.fulfill({ json: MOCK_TABS }));
  await page.route('**/api/system', route => route.fulfill({ json: MOCK_SYSTEM }));
  await page.route('**/api/wifi/status', route => route.fulfill({ json: MOCK_WIFI_STATUS }));
  await page.route('**/api/wifi/scan', route => route.fulfill({ json: { networks: [] } }));
  await page.route('**/api/keys', route => {
    if (route.request().method() === 'GET') {
      return route.fulfill({ json: keys });
    }
    return route.fulfill({ json: { status: 'ok' } });
  });

  await page.goto('/');
  await page.waitForFunction(() => (window as any).apiFetch !== undefined);
  await page.locator('#bottom-nav .tab-link.active').waitFor();

  // Navigate to System → Keys
  await page.locator('#bottom-nav [data-target="tab-system"]').click();
  await expect(page.locator('#pill-bar')).toBeVisible();
  await page.locator('#pill-bar .pill-btn[data-target="tab-keys"]').click();
  await expect(page.locator('#tab-keys')).toHaveClass(/active/);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

test.describe('Key Vault Sub-pill', () => {
  test('key list renders IDs with masked dots', async ({ page }) => {
    await bootAndNavigateToKeys(page);

    const keysList = page.locator('#keys-list');
    // Wait for keys to load
    await expect(keysList).toContainText('weather_key');
    await expect(keysList).toContainText('api_token');

    // Each key should display masked dots (••••••••)
    await expect(keysList).toContainText('••••••••');
  });

  test('wifi_password key shows managed badge and no delete button', async ({ page }) => {
    await bootAndNavigateToKeys(page);

    const keysList = page.locator('#keys-list');
    await expect(keysList).toContainText('wifi_password');

    // Should show "Managed by WiFi" badge
    await expect(keysList).toContainText('Managed by WiFi');

    // wifi_password should NOT have a delete button
    const wifiDeleteBtn = keysList.locator('.btn-key-delete[data-id="wifi_password"]');
    await expect(wifiDeleteBtn).toHaveCount(0);
  });

  test('add key flow — enter ID + value, POST sent, key appears in refreshed list', async ({ page }) => {
    // Start with initial keys, track mutations
    let currentKeys = [...MOCK_KEYS];

    await page.route('**/api/app', route => route.fulfill({ json: { name: 'Foundation Test App' } }));
    await page.route('**/api/tabs', route => route.fulfill({ json: MOCK_TABS }));
    await page.route('**/api/system', route => route.fulfill({ json: MOCK_SYSTEM }));
    await page.route('**/api/wifi/status', route => route.fulfill({ json: MOCK_WIFI_STATUS }));
    await page.route('**/api/wifi/scan', route => route.fulfill({ json: { networks: [] } }));
    await page.route('**/api/keys', async route => {
      if (route.request().method() === 'POST') {
        // Simulate adding the key
        currentKeys = [...currentKeys, { id: 'new_api_key' }];
        return route.fulfill({ json: { status: 'saved' } });
      }
      return route.fulfill({ json: currentKeys });
    });

    await page.goto('/');
    await page.waitForFunction(() => (window as any).apiFetch !== undefined);
    await page.locator('#bottom-nav .tab-link.active').waitFor();

    // Navigate to System → Keys
    await page.locator('#bottom-nav [data-target="tab-system"]').click();
    await expect(page.locator('#pill-bar')).toBeVisible();
    await page.locator('#pill-bar .pill-btn[data-target="tab-keys"]').click();
    await expect(page.locator('#tab-keys')).toHaveClass(/active/);
    await expect(page.locator('#keys-list')).toContainText('weather_key');

    // Fill in new key fields
    await page.locator('#keys-name-input').fill('new_api_key');
    await page.locator('#keys-value-input').fill('secret_value_123');

    // Click Add Key
    await page.locator('#btn-keys-add').click();

    // Should show success status
    await expect(page.locator('#keys-add-status')).toContainText('saved successfully');

    // The new key should appear in the refreshed list
    await expect(page.locator('#keys-list')).toContainText('new_api_key');
  });

  test('delete key shows armed confirmation then sends DELETE', async ({ page }) => {
    await bootAndNavigateToKeys(page);

    // Wait for keys to render
    await expect(page.locator('#keys-list')).toContainText('weather_key');

    const deleteBtn = page.locator('.btn-key-delete[data-id="weather_key"]');

    // First click — arm
    await deleteBtn.click();
    await expect(deleteBtn).toContainText('Confirm?');

    // Mock the DELETE endpoint
    await page.route('**/api/keys**', async route => {
      if (route.request().method() === 'DELETE') {
        return route.fulfill({ json: { status: 'deleted' } });
      }
      // After deletion, return keys without the deleted one
      return route.fulfill({ json: [{ id: 'wifi_password' }, { id: 'api_token' }] });
    });

    // Second click — confirm delete
    await deleteBtn.click();

    // Key should be removed from the list after reload
    await expect(page.locator('#keys-list')).not.toContainText('weather_key');
  });

  test('empty state shows appropriate message when no keys exist', async ({ page }) => {
    await bootAndNavigateToKeys(page, []);

    const keysList = page.locator('#keys-list');
    await expect(keysList).toContainText('No keys stored');
  });
});
