import { test, expect, Page } from '@playwright/test';

// ---------------------------------------------------------------------------
// Mock data
//
// PORTAL BUG: app.js sorts children with `(a.order || 100)`, so order:0 is
// treated as falsy and becomes 100. Status (order:0→100) sorts AFTER Keys
// (order:1). Tests must explicitly navigate to the Status sub-pill.
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

const MOCK_SYSTEM_GREEN = {
  firmware_version: '1.0.0-test',
  build_serial: '20260601-001',
  sdk_version: 'v5.4.1',
  chip_model: 'ESP32-S3',
  chip_revision: 1,
  chip_cores: 2,
  flash_size: 8388608,
  partition: 'app0',
  uptime: 3661,
  chip_temp: 42.0,
  heap_total: 249856,
  heap_free: 180000,       // ~72% → Green
  heap_min_free: 140000,   // ~56% → Green
  heap_max_alloc: 160000,
  psram_total: 2097152,
  psram_free: 1800000,
  storage_used: 200000,
  storage_total: 1048576,
  ap_mode: false,
  wifi_ssid: 'TestNetwork',
  wifi_ip: '192.168.1.100',
  wifi_mac: 'AA:BB:CC:DD:EE:FF',
  wifi_rssi: -55,
};

/** System data where heap_min_free triggers RED (<25%) */
// Internal-DRAM bars use ABSOLUTE free-byte floors, not percentages (#121):
// green ≥ 80 KB, amber ≥ 40 KB, red < 40 KB. A percentage would show perpetual
// amber on a healthy ESP32. 30000 B is genuinely red; the previous 45000 B was
// chosen when the bar was percentage-based (≈18%) and has rendered amber ever
// since that change landed.
const MOCK_SYSTEM_RED_HEAP = {
  ...MOCK_SYSTEM_GREEN,
  heap_min_free: 30000,    // < 40 KB → Red
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

async function bootAndNavigateToStatus(page: Page, systemData = MOCK_SYSTEM_GREEN) {
  await page.route('**/api/app', route => route.fulfill({ json: { name: 'Foundation Test App' } }));
  await page.route('**/api/tabs', route => route.fulfill({ json: MOCK_TABS }));
  await page.route('**/api/system', route => route.fulfill({ json: systemData }));
  await page.route('**/api/wifi/status', route => route.fulfill({ json: MOCK_WIFI_STATUS }));
  await page.route('**/api/wifi/scan', route => route.fulfill({ json: { networks: [] } }));

  await page.goto('/');
  await page.waitForFunction(() => (window as any).apiFetch !== undefined);
  await expect(page.locator('#portal-title')).not.toHaveText('ESP32 App Foundation');

  // Navigate to System tab → pill bar appears
  await page.locator('#bottom-nav [data-target="tab-system"]').click();
  await expect(page.locator('#pill-bar')).toBeVisible();

  // Explicitly click the Status sub-pill (NOT the default due to order:0||100 bug)
  await page.locator('#pill-bar .pill-btn[data-target="tab-status"]').click();
  await expect(page.locator('#tab-status')).toHaveClass(/tab-content--active/);

  // Wait for system data to render
  await expect(page.locator('#sys-device-info')).toContainText('ESP32');
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

test.describe('System Status Sub-pill', () => {
  test('device info card renders chip model, SDK, and firmware version', async ({ page }) => {
    await bootAndNavigateToStatus(page);

    const deviceInfo = page.locator('#sys-device-info');
    await expect(deviceInfo).toContainText('v1.0.0-test');
    await expect(deviceInfo).toContainText('v5.4.1');
    await expect(deviceInfo).toContainText('ESP32-S3');
    await expect(deviceInfo).toContainText('app0');
    await expect(deviceInfo).toContainText('20260601-001');
  });

  test('memory bars show RAG colours — red when heap min free < 25%', async ({ page }) => {
    await bootAndNavigateToStatus(page, MOCK_SYSTEM_RED_HEAP);

    const memoryBars = page.locator('#sys-memory-bars');

    // The "Min Free (Watermark)" bar should be red (#c62828).
    // 30000 B < 40 KB absolute floor → red (see ragColorDram in system.js)
    const progressFills = memoryBars.locator('div[style*="background:#c62828"]');
    await expect(progressFills.first()).toBeVisible();
  });

  test('storage bar renders used/total with correct percentage', async ({ page }) => {
    await bootAndNavigateToStatus(page);

    const memBars = page.locator('#sys-memory-bars');

    // Storage section should show "Storage Free" label
    await expect(memBars).toContainText('Storage Free');

    // Free storage = 1048576 - 200000 = 848576 bytes
    // Percentage = 848576 / 1048576 ≈ 81% → Green
    await expect(memBars).toContainText('81%');
  });

  test('temperature displays with RAG threshold colouring', async ({ page }) => {
    await bootAndNavigateToStatus(page);

    // chip_temp is 42.0°C → Green (<55°C threshold)
    const deviceInfo = page.locator('#sys-device-info');
    // Should display "42.0°C" with green colour (#2e7d32)
    const tempSpan = deviceInfo.locator('span[style*="#2e7d32"]');
    await expect(tempSpan).toContainText('42.0°C');
  });

  test('restart button uses armed confirmation flow', async ({ page }) => {
    await bootAndNavigateToStatus(page);

    const restartBtn = page.locator('#btn-sys-restart');
    await expect(restartBtn).toContainText('Restart Device');

    // First click — arm
    await restartBtn.click();
    await expect(restartBtn).toContainText('Confirm Restart?');

    // Mock restart endpoint
    await page.route('**/api/system/restart', route => route.fulfill({
      json: { status: 'restarting' },
    }));

    // Second click — confirm
    await restartBtn.click();
    await expect(restartBtn).toContainText('Restarting...');
    await expect(restartBtn).toBeDisabled();
  });

  test('factory reset requires typing RESET to enable confirm button', async ({ page }) => {
    await bootAndNavigateToStatus(page);

    const factoryBtn = page.locator('#btn-sys-factory');
    const confirmArea = page.locator('#sys-factory-confirm');
    const input = page.locator('#sys-factory-input');
    const confirmBtn = page.locator('#btn-sys-factory-confirm');

    // Confirm area should be hidden initially
    await expect(confirmArea).toBeHidden();

    // Click Factory Reset to show confirmation area
    await factoryBtn.click();
    await expect(confirmArea).toBeVisible();

    // Confirm button should be disabled
    await expect(confirmBtn).toBeDisabled();

    // Type wrong text
    await input.fill('WRONG');
    await expect(confirmBtn).toBeDisabled();

    // Type correct text
    await input.fill('RESET');
    await expect(confirmBtn).toBeEnabled();

    // Mock factory reset endpoint
    await page.route('**/api/system/factory-reset', route => route.fulfill({
      json: { status: 'resetting' },
    }));

    // Click confirm
    await confirmBtn.click();
    await expect(confirmBtn).toContainText('Erasing...');
  });
});
