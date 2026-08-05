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

const MOCK_CONFIG = {
  namespaces: ['wifi', 'display', 'ota', 'keys'],
  count: 4,
};

const MOCK_CONFIG_BACKUP = {
  wifi:    { ssid: 'TestNetwork', password: '***' },
  display: { brightness: 80, timeout: 30 },
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

async function bootAndNavigateToBackup(page: Page) {
  await page.route('**/api/app', route => route.fulfill({ json: { name: 'Foundation Test App' } }));
  await page.route('**/api/tabs', route => route.fulfill({ json: MOCK_TABS }));
  await page.route('**/api/system', route => route.fulfill({ json: MOCK_SYSTEM }));
  await page.route('**/api/wifi/status', route => route.fulfill({ json: MOCK_WIFI_STATUS }));
  await page.route('**/api/wifi/scan', route => route.fulfill({ json: { networks: [] } }));
  await page.route('**/api/config', route => route.fulfill({ json: MOCK_CONFIG }));
  await page.route('**/api/config/backup', route => route.fulfill({
    json: MOCK_CONFIG_BACKUP,
    headers: { 'Content-Disposition': 'attachment; filename="config-backup.json"' },
  }));
  await page.route('**/api/config/restore', route => route.fulfill({
    json: { status: 'ok' },
  }));

  await page.goto('/');
  await page.waitForFunction(() => (window as any).apiFetch !== undefined);
  await expect(page.locator('#portal-title')).not.toHaveText('ESP32 App Foundation');

  // Navigate to System → Backup
  await page.locator('#bottom-nav [data-target="tab-system"]').click();
  await expect(page.locator('#pill-bar')).toBeVisible();
  await page.locator('#pill-bar .pill-btn[data-target="tab-backup"]').click();
  await expect(page.locator('#tab-backup')).toHaveClass(/tab-content--active/);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

test.describe('Backup & Restore Sub-pill', () => {
  test('export config triggers a download event', async ({ page }) => {
    await bootAndNavigateToBackup(page);

    // Wait for module list to load (indicates page is ready)
    await expect(page.locator('#backup-module-list')).toContainText('wifi');

    // Listen for download event
    const [download] = await Promise.all([
      page.waitForEvent('download'),
      page.locator('#btn-backup-download').click(),
    ]);

    // Verify download has a filename containing 'config'
    expect(download.suggestedFilename()).toMatch(/config.*\.json/);
  });

  test('config namespace pill badges render from /api/config', async ({ page }) => {
    await bootAndNavigateToBackup(page);

    const moduleList = page.locator('#backup-module-list');

    // Wait for namespaces to render
    await expect(moduleList).toContainText('wifi');
    await expect(moduleList).toContainText('display');
    await expect(moduleList).toContainText('ota');
    await expect(moduleList).toContainText('keys');

    // Module count should show
    await expect(moduleList).toContainText('4 module(s) registered');
  });

  test('import config — file chooser accepts JSON, POST sent to restore endpoint', async ({ page }) => {
    await bootAndNavigateToBackup(page);
    await expect(page.locator('#backup-module-list')).toContainText('wifi');

    // Track that a POST was made to /api/config/restore
    let restorePosted = false;
    await page.route('**/api/config/restore', async route => {
      restorePosted = true;
      return route.fulfill({ json: { status: 'ok' } });
    });

    // Set a valid JSON file via the file input
    const fileInput = page.locator('#backup-restore-file');
    const validConfig = JSON.stringify({ wifi: { ssid: 'RestoredNet' } });
    await fileInput.setInputFiles({
      name: 'config-backup.json',
      mimeType: 'application/json',
      buffer: Buffer.from(validConfig),
    });

    // Verify filename shows
    await expect(page.locator('#backup-file-name')).toContainText('config-backup.json');

    // Click Upload & Restore — first click arms the button
    await page.locator('#btn-backup-restore').click();

    // Second click confirms the armed action
    await page.locator('#btn-backup-restore').click();

    // Should show the reboot overlay (armButton gate + showRebootOverlay)
    await expect(page.locator('#reboot-overlay')).toHaveClass(/reboot-overlay--visible/);
    await expect(page.locator('.reboot-msg')).toContainText('Restoring configuration');
    expect(restorePosted).toBe(true);
  });

  test('invalid import — non-JSON content shows error', async ({ page }) => {
    await bootAndNavigateToBackup(page);
    await expect(page.locator('#backup-module-list')).toContainText('wifi');

    // Set a file with invalid JSON content  
    const fileInput = page.locator('#backup-restore-file');
    await fileInput.setInputFiles({
      name: 'bad-config.json',
      mimeType: 'application/json',
      buffer: Buffer.from('this is not valid json {{{'),
    });

    // Click Upload & Restore
    await page.locator('#btn-backup-restore').click();

    // Should show error about invalid JSON
    await expect(page.locator('#backup-restore-status')).toContainText('Invalid JSON');
  });
});
