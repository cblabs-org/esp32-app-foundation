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
  mac: 'AA:BB:CC:DD:EE:FF',
  rssi: -55,
  channel: 6,
  gateway: '192.168.1.1',
  passExists: true,
  ap_mode: false,
};

const MOCK_WIFI_SCAN = {
  networks: [
    { ssid: 'HomeWiFi',  rssi: -35, secure: true  },
    { ssid: 'OfficeNet', rssi: -60, secure: true  },
    { ssid: 'GuestNet',  rssi: -75, secure: false },
    { ssid: 'FarAway',   rssi: -90, secure: true  },
  ],
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Mock all boot-critical endpoints plus WiFi-specific ones. */
async function bootPortal(page: Page) {
  await page.route('**/api/app', route => route.fulfill({ json: { name: 'Foundation Test App' } }));
  await page.route('**/api/tabs', route => route.fulfill({ json: MOCK_TABS }));
  await page.route('**/api/system', route => route.fulfill({ json: MOCK_SYSTEM }));
  await page.route('**/api/wifi/status', route => route.fulfill({ json: MOCK_WIFI_STATUS }));
  // Default: scan returns the network list immediately
  await page.route('**/api/wifi/scan', route => route.fulfill({ json: MOCK_WIFI_SCAN }));

  await page.goto('/');
  await page.waitForFunction(() => (window as any).apiFetch !== undefined);
  await expect(page.locator('body')).toHaveClass(/portal-ready/);
  // WiFi tab is active by default, so it auto-fetches status & starts scan
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

test.describe('WiFi Settings Tab', () => {
  test.beforeEach(async ({ page }) => {
    await bootPortal(page);
  });

  test('status header renders mode badge, SSID, IP, and signal bars', async ({ page }) => {
    // Wait for status to populate
    await expect(page.locator('#wifi-mode-badge')).toHaveText('STA');
    await expect(page.locator('#wifi-status-ssid')).toHaveText('TestNetwork');
    await expect(page.locator('#wifi-status-ip')).toHaveText('192.168.1.100');

    // Signal bars area should contain the RSSI text
    await expect(page.locator('#wifi-status-signal')).toContainText('-55 dBm');
  });

  test('SSID auto-populates from current connection', async ({ page }) => {
    // Wait for the status fetch to complete and SSID input to be filled
    await expect(page.locator('#wifi-ssid-input')).toHaveValue('TestNetwork');
  });

  test('network scan renders list with signal bars and lock icons', async ({ page }) => {
    // The scan auto-starts on tab activation, so wait for networks to render
    await expect(page.locator('#wifi-network-list .wifi-net-item')).toHaveCount(4);

    // Check first network (HomeWiFi) is listed
    const firstNet = page.locator('#wifi-network-list .wifi-net-item').first();
    await expect(firstNet).toContainText('HomeWiFi');
    await expect(firstNet).toContainText('-35');

    // HomeWiFi is secure — should have a lock SVG
    const lockSvg = firstNet.locator('svg').nth(1); // 2nd SVG is the lock
    await expect(lockSvg).toBeVisible();

    // GuestNet is open (not secure) — third in list (0-indexed: 2)
    const guestNet = page.locator('#wifi-network-list .wifi-net-item').nth(2);
    await expect(guestNet).toContainText('GuestNet');
    // Only signal bars SVG, no lock SVG
    const guestSvgs = guestNet.locator('svg');
    await expect(guestSvgs).toHaveCount(1);
  });

  test('network selection fills the SSID input field', async ({ page }) => {
    // Wait for scan results
    await expect(page.locator('#wifi-network-list .wifi-net-item')).toHaveCount(4);

    // Click OfficeNet
    await page.locator('#wifi-network-list .wifi-net-item[data-ssid="OfficeNet"]').click();

    // SSID input should now show OfficeNet
    await expect(page.locator('#wifi-ssid-input')).toHaveValue('OfficeNet');
  });

  test('password shows saved placeholder when passExists is true', async ({ page }) => {
    const passInput = page.locator('#wifi-pass-input');
    // Wait for status fetch to complete
    await expect(page.locator('#wifi-mode-badge')).toHaveText('STA');

    // Placeholder should show saved indicator
    await expect(passInput).toHaveAttribute('placeholder', '●●●●●●●● (saved)');
  });

  test('eye toggle is disabled for saved password, enabled when user types', async ({ page }) => {
    const eyeBtn = page.locator('#wifi-eye-btn');
    const passInput = page.locator('#wifi-pass-input');
    const wrapper = page.locator('.password-input-wrapper');

    // Wait for status fetch
    await expect(page.locator('#wifi-mode-badge')).toHaveText('STA');

    // Verify .password-input-wrapper wraps the input and eye button
    await expect(wrapper).toBeVisible();
    await expect(wrapper.locator('#wifi-pass-input')).toBeVisible();
    await expect(wrapper.locator('#wifi-eye-btn')).toBeVisible();
    await expect(eyeBtn).toHaveClass(/eye-btn/);
    await expect(eyeBtn).toHaveAttribute('aria-label', 'Show password');

    // Eye button should be visually dimmed (opacity 0.3) and pointer-events none
    await expect(eyeBtn).toHaveCSS('opacity', '0.3');
    await expect(eyeBtn).toHaveCSS('pointer-events', 'none');

    // Type in the password field
    await passInput.fill('newsecretpass');

    // Eye button should now be active
    await expect(eyeBtn).toHaveCSS('opacity', '1');
    await expect(eyeBtn).toHaveCSS('pointer-events', 'auto');

    // Clicking eye toggle reveals password
    await eyeBtn.click();
    await expect(passInput).toHaveAttribute('type', 'text');
    await expect(eyeBtn).toHaveAttribute('aria-label', 'Hide password');

    // Clicking again hides password
    await eyeBtn.click();
    await expect(passInput).toHaveAttribute('type', 'password');
    await expect(eyeBtn).toHaveAttribute('aria-label', 'Show password');
  });

  test('test connection sends POST and shows success with IP', async ({ page }) => {
    // Mock the test endpoint
    await page.route('**/api/wifi/test', route => route.fulfill({
      json: { status: 'ok', ip: '192.168.1.100' },
    }));

    // Select a network first
    await expect(page.locator('#wifi-network-list .wifi-net-item')).toHaveCount(4);
    await page.locator('#wifi-network-list .wifi-net-item[data-ssid="HomeWiFi"]').click();

    // Click Test Connection
    await page.locator('#wifi-test-btn').click();

    // Should show success with IP
    await expect(page.locator('#wifi-action-result')).toContainText('192.168.1.100');
  });

  test('forget network shows armed confirmation then executes', async ({ page }) => {
    const forgetBtn = page.locator('#wifi-forget-btn');

    // First click — arm the button
    await forgetBtn.click();
    await expect(forgetBtn).toContainText('Confirm Forget');

    // Mock the forget endpoint
    await page.route('**/api/wifi/forget', route => route.fulfill({
      json: { status: 'forgotten' },
    }));

    // Second click — confirm
    await forgetBtn.click();

    // The universal reboot overlay should appear (device "restarts" after forget)
    await expect(page.locator('#reboot-overlay')).toHaveClass(/visible/);
  });
});
