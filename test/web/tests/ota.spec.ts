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
  build_serial: '20260601-001',
  sdk_version: 'v5.4.1',
  chip_model: 'ESP32-S3',
  chip_revision: 1,
  chip_cores: 2,
  flash_size: 8388608,
  partition: 'app0',
  uptime: 3661,
  chip_temp: 48.5,
  heap_total: 249856,
  heap_free: 124928,
  heap_min_free: 45000,
  heap_max_alloc: 110000,
  psram_total: 2097152,
  psram_free: 1800000,
  storage_used: 786432,
  storage_total: 1048576,
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

async function bootAndNavigateToOta(page: Page) {
  await page.route('**/api/app', route => route.fulfill({ json: { name: 'Foundation Test App' } }));
  await page.route('**/api/tabs', route => route.fulfill({ json: MOCK_TABS }));
  await page.route('**/api/system', route => route.fulfill({ json: MOCK_SYSTEM }));
  await page.route('**/api/wifi/status', route => route.fulfill({ json: MOCK_WIFI_STATUS }));
  await page.route('**/api/wifi/scan', route => route.fulfill({ json: { networks: [] } }));

  await page.goto('/');
  await page.waitForFunction(() => (window as any).apiFetch !== undefined);
  await page.locator('#bottom-nav .tab-link.active').waitFor();

  // Navigate to System → OTA
  await page.locator('#bottom-nav [data-target="tab-system"]').click();
  await expect(page.locator('#pill-bar')).toBeVisible();
  await page.locator('#pill-bar .pill-btn[data-target="tab-ota"]').click();
  await expect(page.locator('#tab-ota')).toHaveClass(/active/);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

test.describe('OTA Update Sub-pill', () => {
  test('firmware info card renders version and partition from /api/system', async ({ page }) => {
    await bootAndNavigateToOta(page);

    const firmwareInfo = page.locator('#ota-firmware-info');
    await expect(firmwareInfo).toContainText('v1.0.0-test');
    await expect(firmwareInfo).toContainText('app0');
  });

  test('check for update flow — POST checkGithub, shows status message', async ({ page }) => {
    await page.route('**/api/ota', route => {
      const body = route.request().postData();
      if (body && body.includes('checkGithub')) {
        return route.fulfill({ json: { msg: 'Update check started' } });
      }
      if (body && body.includes('status')) {
        return route.fulfill({ json: { state: 'idle', lastError: '' } });
      }
      return route.fulfill({ json: { status: 'ok' } });
    });

    await bootAndNavigateToOta(page);

    const checkBtn = page.locator('#btn-ota-check');
    const statusEl = page.locator('#ota-check-status');

    // Click check for updates
    await checkBtn.click();

    // Status should show the response message
    await expect(statusEl).toContainText('Update check started');
  });

  test('manual upload — file input accepts .bin, FormData POST to /update', async ({ page }) => {
    let uploadPostReceived = false;

    await page.route('**/update', route => {
      uploadPostReceived = true;
      return route.fulfill({ json: { status: 'ok' } });
    });

    await bootAndNavigateToOta(page);

    // Set a .bin file on the file input
    const fileInput = page.locator('#ota-file');
    await fileInput.setInputFiles({
      name: 'firmware.bin',
      mimeType: 'application/octet-stream',
      buffer: Buffer.from([0x00, 0x01, 0x02, 0x03]),
    });

    // Click Upload Firmware
    await page.locator('#btn-ota-upload').click();

    // Should show success
    await expect(page.locator('#ota-upload-status')).toContainText('successful');
    expect(uploadPostReceived).toBe(true);
  });

  test('rollback button uses armed confirmation (3s timeout)', async ({ page }) => {
    await page.route('**/api/ota', route => {
      return route.fulfill({ json: { status: 'ok' } });
    });

    await bootAndNavigateToOta(page);

    const rollbackBtn = page.locator('#btn-ota-rollback');
    await expect(rollbackBtn).toContainText('Rollback to Previous');

    // First click — arm
    await rollbackBtn.click();
    await expect(rollbackBtn).toContainText('Confirm Rollback?');

    // Second click — confirm
    await rollbackBtn.click();
    await expect(rollbackBtn).toContainText('Rolling back...');
    await expect(rollbackBtn).toBeDisabled();
  });

  test('error state — update check fails, shows error message', async ({ page }) => {
    // Make the OTA endpoint fail
    await page.route('**/api/ota', route => {
      return route.fulfill({ status: 500, body: 'Internal Server Error' });
    });

    await bootAndNavigateToOta(page);

    const checkBtn = page.locator('#btn-ota-check');
    await checkBtn.click();

    // Should show error
    await expect(page.locator('#ota-check-status')).toContainText('Failed to check');
  });
});
