import { test, expect, Page } from '@playwright/test';

// ---------------------------------------------------------------------------
// Mock data
// ---------------------------------------------------------------------------

const MOCK_TABS = [
  { name: 'WiFi',     id: 'wifi',     parent: null,       order: 0   },
  { name: 'System',   id: 'system',   parent: null,       order: 100 },
  { name: 'Status',   id: 'system',   parent: 'System',   order: 0   },
  { name: 'Keys',     id: 'keys',     parent: 'System',   order: 1   },
  { name: 'MCP',      id: 'mcp',      parent: 'System',   order: 2   },
  { name: 'Backup',   id: 'backup',   parent: 'System',   order: 3   },
  { name: 'OTA',      id: 'ota',      parent: 'System',   order: 4   },
  { name: 'Security', id: 'security', parent: null,       order: 50  },
  { name: 'Password', id: 'password', parent: 'Security', order: 0   },
  { name: 'TLS',      id: 'tls',      parent: 'Security', order: 1   },
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
  ],
};

const MOCK_AUTH_STATUS = {
  authEnabled: false,
  tlsEnabled: false,
  certPresent: false,
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Boot the portal with WiFi tab active (default landing tab). */
async function bootPortal(page: Page) {
  await page.route('**/api/app', route => route.fulfill({ json: { name: 'Foundation Test App' } }));
  await page.route('**/api/tabs', route => route.fulfill({ json: MOCK_TABS }));
  await page.route('**/api/system', route => route.fulfill({ json: MOCK_SYSTEM }));
  await page.route('**/api/wifi/status', route => route.fulfill({ json: MOCK_WIFI_STATUS }));
  await page.route('**/api/wifi/scan', route => route.fulfill({ json: MOCK_WIFI_SCAN }));

  await page.goto('/');
  await page.waitForFunction(() => (window as any).apiFetch !== undefined);
  await expect(page.locator('#portal-title')).not.toHaveText('Device Portal');
}

/** Navigate from any tab to System → Status sub-pill. */
async function navigateToStatus(page: Page) {
  await page.locator('#bottom-nav [data-target="tab-system"]').click();
  await expect(page.locator('#pill-bar')).toBeVisible();
  await page.locator('#pill-bar .pill-btn[data-target="tab-status"]').click();
  await expect(page.locator('#tab-status')).toHaveClass(/active/);
  await expect(page.locator('#sys-device-info')).toContainText('ESP32');
}

/** Navigate from any tab to Security → TLS sub-pill. */
async function navigateToTls(page: Page) {
  // Mock auth endpoints needed for TLS tab rendering
  await page.route('**/api/auth/status', route => route.fulfill({ json: MOCK_AUTH_STATUS }));

  await page.locator('#bottom-nav [data-target="tab-security"]').click();
  await expect(page.locator('#pill-bar')).toBeVisible();
  await page.locator('#pill-bar .pill-btn[data-target="tab-tls"]').click();
  await expect(page.locator('#tab-tls')).toHaveClass(/active/);
}

// ---------------------------------------------------------------------------
// Cleanup — prevent reconnection polling from leaking between tests
// ---------------------------------------------------------------------------

test.afterEach(async ({ page }) => {
  await page.evaluate(() => {
    if ((window as any)._rebootPollTimer) {
      clearInterval((window as any)._rebootPollTimer);
      (window as any)._rebootPollTimer = null;
    }
  });
});

// ---------------------------------------------------------------------------
// Tests: Overlay Visibility
// ---------------------------------------------------------------------------

test.describe('Reboot Overlay — Visibility', () => {
  test('reboot overlay is hidden by default', async ({ page }) => {
    await bootPortal(page);

    // The overlay is lazily injected — it should not exist in the DOM on boot
    const overlay = page.locator('#reboot-overlay');
    await expect(overlay).toHaveCount(0);
  });

  test('showRebootOverlay displays with correct message', async ({ page }) => {
    await bootPortal(page);

    await page.evaluate(() => {
      (window as any).showRebootOverlay('Test Message', 'Test sub-message');
    });

    const overlay = page.locator('#reboot-overlay');
    await expect(overlay).toHaveClass(/visible/);
    await expect(overlay.locator('.reboot-msg')).toHaveText('Test Message');
    await expect(overlay.locator('.reboot-sub')).toHaveText('Test sub-message');
  });

  test('hideRebootOverlay removes overlay', async ({ page }) => {
    await bootPortal(page);

    // Show then hide
    await page.evaluate(() => {
      (window as any).showRebootOverlay('Temporary', 'Going away');
    });
    await expect(page.locator('#reboot-overlay')).toHaveClass(/visible/);

    await page.evaluate(() => {
      (window as any).hideRebootOverlay();
    });

    // The visible class should be removed
    const overlay = page.locator('#reboot-overlay');
    await expect(overlay).not.toHaveClass(/visible/);
  });
});

// ---------------------------------------------------------------------------
// Tests: WiFi Flow
// ---------------------------------------------------------------------------

test.describe('Reboot Overlay — WiFi Flow', () => {
  test('WiFi Save & Connect shows reboot overlay', async ({ page }) => {
    await bootPortal(page);

    // Mock the connect endpoint
    await page.route('**/api/wifi/connect', route => route.fulfill({
      json: { status: 'connecting' },
    }));

    // Wait for status fetch to populate the SSID input
    await expect(page.locator('#wifi-ssid-input')).toHaveValue('TestNetwork');

    // Fill password and click Save & Connect
    await page.locator('#wifi-pass-input').fill('testpassword');
    await page.locator('#wifi-save-btn').click();

    // Overlay should appear with WiFi-specific message
    const overlay = page.locator('#reboot-overlay');
    await expect(overlay).toHaveClass(/visible/);
    await expect(overlay.locator('.reboot-msg')).toContainText('Connecting to WiFi');
    await expect(overlay.locator('.reboot-sub')).toContainText('restart to connect');
  });

  test('WiFi Forget shows reboot overlay', async ({ page }) => {
    await bootPortal(page);

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

    // Overlay should appear with forget/AP-mode message
    const overlay = page.locator('#reboot-overlay');
    await expect(overlay).toHaveClass(/visible/);
    await expect(overlay.locator('.reboot-msg')).toContainText('Restarting device');
    await expect(overlay.locator('.reboot-sub')).toContainText('WiFi credentials have been erased');
  });
});

// ---------------------------------------------------------------------------
// Tests: System Flow
// ---------------------------------------------------------------------------

test.describe('Reboot Overlay — System Flow', () => {
  test('System Restart shows reboot overlay', async ({ page }) => {
    await bootPortal(page);
    await navigateToStatus(page);

    const restartBtn = page.locator('#btn-sys-restart');

    // First click — arm the button
    await restartBtn.click();
    await expect(restartBtn).toContainText('Confirm Restart?');

    // Mock the restart endpoint
    await page.route('**/api/system/restart', route => route.fulfill({
      json: { status: 'restarting' },
    }));

    // Second click — confirm
    await restartBtn.click();

    // Overlay should appear with restart message
    const overlay = page.locator('#reboot-overlay');
    await expect(overlay).toHaveClass(/visible/);
    await expect(overlay.locator('.reboot-msg')).toContainText('Restarting device');
    await expect(overlay.locator('.reboot-sub')).toContainText('back online shortly');
  });

  test('Factory Reset shows reboot overlay', async ({ page }) => {
    await bootPortal(page);
    await navigateToStatus(page);

    // Open the factory reset confirmation area
    await page.locator('#btn-sys-factory').click();
    await expect(page.locator('#sys-factory-confirm')).toBeVisible();

    // Type RESET to enable the confirm button
    await page.locator('#sys-factory-input').fill('RESET');
    await expect(page.locator('#btn-sys-factory-confirm')).toBeEnabled();

    // Mock the factory reset endpoint
    await page.route('**/api/system/factory-reset', route => route.fulfill({
      json: { status: 'resetting' },
    }));

    // Click confirm
    await page.locator('#btn-sys-factory-confirm').click();

    // Overlay should appear with factory reset message
    const overlay = page.locator('#reboot-overlay');
    await expect(overlay).toHaveClass(/visible/);
    await expect(overlay.locator('.reboot-msg')).toContainText('Factory reset in progress');
    await expect(overlay.locator('.reboot-sub')).toContainText('All settings will be erased');
  });
});

// ---------------------------------------------------------------------------
// Tests: Security / TLS Flow
// ---------------------------------------------------------------------------

test.describe('Reboot Overlay — TLS Flow', () => {
  test('TLS toggle shows reboot overlay', async ({ page }) => {
    await bootPortal(page);
    await navigateToTls(page);

    // Wait for TLS tab content to render (the toggle-switch label wraps a
    // visually hidden checkbox — check the label container is present)
    const tlsToggle = page.locator('#tls-enabled-chk');
    await expect(tlsToggle).toBeAttached();

    // Mock /api/auth/tls to return restartRequired: true
    await page.route('**/api/auth/tls', route => route.fulfill({
      json: { restartRequired: true },
    }));

    // Mock /api/system/restart (called after the 2s delay)
    await page.route('**/api/system/restart', route => route.fulfill({
      json: { status: 'restarting' },
    }));

    // Toggle the TLS switch via evaluate — the checkbox is zero-size inside
    // a styled toggle-switch label, so we toggle it programmatically
    await page.evaluate(() => {
      const chk = document.getElementById('tls-enabled-chk') as HTMLInputElement;
      chk.checked = true;
      chk.dispatchEvent(new Event('change'));
    });

    // The overlay appears after a 2s setTimeout — use a longer timeout
    const overlay = page.locator('#reboot-overlay');
    await expect(overlay).toHaveClass(/visible/, { timeout: 5000 });
    await expect(overlay.locator('.reboot-msg')).toContainText('Applying TLS settings');
    await expect(overlay.locator('.reboot-sub')).toContainText('restart to apply the new TLS configuration');
  });
});
