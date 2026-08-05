import { test, expect, Page } from '@playwright/test';

// ---------------------------------------------------------------------------
// Mock data
// ---------------------------------------------------------------------------

const MOCK_TABS = [
  { name: 'WiFi',       id: 'wifi',       parent: null,      order: 0   },
  { name: 'System',     id: 'system',     parent: null,      order: 100 },
  { name: 'Status',     id: 'system',     parent: 'System',  order: 0   },
  { name: 'Keys',       id: 'keys',       parent: 'System',  order: 1   },
  { name: 'MCP',        id: 'mcp',        parent: 'System',  order: 2   },
  { name: 'Backup',     id: 'backup',     parent: 'System',  order: 3   },
  { name: 'OTA',        id: 'ota',        parent: 'System',  order: 4   },
  { name: 'Display',    id: 'display',    parent: null,      order: 30  },
  { name: 'Settings',   id: 'settings',   parent: 'Display', order: 10  },
  { name: 'Screenshot', id: 'screenshot', parent: 'Display', order: 20  },
  { name: 'App',        id: 'app',        parent: null,      order: 40  },
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

const MOCK_DISPLAY_STATUS = {
  brightness:         200,
  flip:               false,
  screensaverEnabled: true,
  dimmed:             false,
  sleeping:           false,
  dimTimeout:         150000,
  blankTimeout:       300000,
  rotationInterval:   8000,
  boardCount:         2,
  clockFontSize:      2,
  activeBoard:        'Clock',
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Mock all boot-critical endpoints plus display-specific ones. */
async function bootPortal(page: Page, displayOverrides?: Partial<typeof MOCK_DISPLAY_STATUS>) {
  await page.route('**/api/app', route => route.fulfill({ json: { name: 'Foundation Test App' } }));
  await page.route('**/api/tabs', route => route.fulfill({ json: MOCK_TABS }));
  await page.route('**/api/system', route => route.fulfill({ json: MOCK_SYSTEM }));
  await page.route('**/api/wifi/status', route => route.fulfill({ json: MOCK_WIFI_STATUS }));
  await page.route('**/api/wifi/scan', route => route.fulfill({ json: { networks: [] } }));
  await page.route('**/api/display/status', route => route.fulfill({
    json: { ...MOCK_DISPLAY_STATUS, ...displayOverrides },
  }));

  // Default mocks for POST endpoints (return { status: 'ok' })
  await page.route('**/api/display/brightness', route => route.fulfill({ json: { status: 'ok' } }));
  await page.route('**/api/display/flip', route => route.fulfill({ json: { status: 'ok' } }));
  await page.route('**/api/display/sleep', route => route.fulfill({ json: { status: 'ok' } }));
  await page.route('**/api/display/clockFontSize', route => route.fulfill({ json: { status: 'ok' } }));

  await page.goto('/');
  await page.waitForFunction(() => (window as any).apiFetch !== undefined);
  await expect(page.locator('#portal-title')).not.toHaveText('Device Portal');
}

/** Navigate to the Display top-level tab and wait for the Settings sub-pill to render. */
async function navigateToDisplayTab(page: Page) {
  // Click the Display top-level tab
  await page.locator('#bottom-nav .tab-link[data-target="tab-display"]').click();
  // Wait for the Settings sub-pill content to render
  await page.locator('#display-brightness').waitFor({ state: 'visible' });
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

test.describe('Display Settings Tab', () => {

  test.beforeEach(async ({ page }) => {
    await bootPortal(page);
    await navigateToDisplayTab(page);
  });

  // ── Brightness ──────────────────────────────────────────────────

  test('loads and shows current brightness value', async ({ page }) => {
    const slider = page.locator('#display-brightness');
    const label  = page.locator('#display-brightness-value');

    await expect(slider).toHaveValue('200');
    await expect(label).toHaveText('200');
  });

  test('brightness slider change sends POST to /api/display/brightness', async ({ page }) => {
    const requestPromise = page.waitForRequest(req =>
      req.url().includes('/api/display/brightness') && req.method() === 'POST'
    );

    // Set slider to a new value
    await page.locator('#display-brightness').fill('128');

    const request = await requestPromise;
    const body = request.postDataJSON();
    expect(body.value).toBe(128);
  });

  // ── Flip Toggle ─────────────────────────────────────────────────

  test('flip toggle sends POST to /api/display/flip', async ({ page }) => {
    const requestPromise = page.waitForRequest(req =>
      req.url().includes('/api/display/flip') && req.method() === 'POST'
    );

    // Click the parent label of the hidden checkbox to toggle it
    await page.locator('#display-flip').evaluate((el: HTMLInputElement) => {
      el.checked = true;
      el.dispatchEvent(new Event('change', { bubbles: true }));
    });

    const request = await requestPromise;
    const body = request.postDataJSON();
    expect(body.value).toBe(true);
  });

  // ── Screensaver Enable/Disable ──────────────────────────────────

  test('screensaver toggle sends POST to /api/display/sleep with enabled field', async ({ page }) => {
    const requestPromise = page.waitForRequest(req =>
      req.url().includes('/api/display/sleep') && req.method() === 'POST'
    );

    // Programmatically uncheck the hidden checkbox
    await page.locator('#display-screensaver-enabled').evaluate((el: HTMLInputElement) => {
      el.checked = false;
      el.dispatchEvent(new Event('change', { bubbles: true }));
    });

    const request = await requestPromise;
    const body = request.postDataJSON();
    expect(body.enabled).toBe(false);
  });

  // ── Dim Delay ───────────────────────────────────────────────────

  test('dim delay change sends POST with dimTimeout field', async ({ page }) => {
    const requestPromise = page.waitForRequest(req =>
      req.url().includes('/api/display/sleep') && req.method() === 'POST'
    );

    await page.locator('#display-dim-delay').selectOption('300000');

    const request = await requestPromise;
    const body = request.postDataJSON();
    expect(body.dimTimeout).toBe(300000);
  });

  // ── Blank Delay ─────────────────────────────────────────────────

  test('blank delay change sends POST with blankTimeout field', async ({ page }) => {
    const requestPromise = page.waitForRequest(req =>
      req.url().includes('/api/display/sleep') && req.method() === 'POST'
    );

    await page.locator('#display-blank-delay').selectOption('600000');

    const request = await requestPromise;
    const body = request.postDataJSON();
    expect(body.blankTimeout).toBe(600000);
  });

  // ── Wake / Sleep Buttons ────────────────────────────────────────

  test('wake button sends action wake', async ({ page }) => {
    const requestPromise = page.waitForRequest(req =>
      req.url().includes('/api/display/sleep') && req.method() === 'POST'
    );

    await page.locator('#display-wake-btn').click();

    const request = await requestPromise;
    const body = request.postDataJSON();
    expect(body.action).toBe('wake');
  });

  test('sleep button sends action sleep', async ({ page }) => {
    const requestPromise = page.waitForRequest(req =>
      req.url().includes('/api/display/sleep') && req.method() === 'POST'
    );

    await page.locator('#display-sleep-btn').click();

    const request = await requestPromise;
    const body = request.postDataJSON();
    expect(body.action).toBe('sleep');
  });

  // ── Clock Font Size (now in App tab) ────────────────────────────

  test('clock font selector sends POST to /api/display/clockFontSize', async ({ page }) => {
    // Clock font is in the App tab (appconfig.js), not the Display/Settings sub-pill
    await page.locator('#bottom-nav .tab-link[data-target="tab-app"]').click();
    // Wait for the App tab container to become active (content rendered by appconfig.js)
    await page.locator('#tab-app.tab-content--active').waitFor({ state: 'attached' });

    const requestPromise = page.waitForRequest(req =>
      req.url().includes('/api/display/clockFontSize') && req.method() === 'POST'
    );

    // Programmatically check the radio since it's display:none
    await page.locator('input[name="display-font-size"][value="3"]').evaluate((el: HTMLInputElement) => {
      el.checked = true;
      el.dispatchEvent(new Event('change', { bubbles: true }));
    });

    const request = await requestPromise;
    const body = request.postDataJSON();
    expect(body.value).toBe(3);
  });

  // ── State Indicator ─────────────────────────────────────────────

  test('state indicator shows Active when not dimmed or sleeping', async ({ page }) => {
    await expect(page.locator('#display-screensaver-status')).toHaveText('Active');
  });

  test('state indicator shows Dimmed when dimmed', async ({ page }) => {
    // Re-navigate with dimmed override
    await page.unrouteAll();
    await bootPortal(page, { dimmed: true });
    await navigateToDisplayTab(page);

    await expect(page.locator('#display-screensaver-status')).toHaveText('Dimmed');
  });

  test('state indicator shows Sleeping when sleeping', async ({ page }) => {
    await page.unrouteAll();
    await bootPortal(page, { sleeping: true });
    await navigateToDisplayTab(page);

    await expect(page.locator('#display-screensaver-status')).toHaveText('Sleeping');
  });

  test('state indicator shows Disabled when screensaver is off', async ({ page }) => {
    await page.unrouteAll();
    await bootPortal(page, { screensaverEnabled: false });
    await navigateToDisplayTab(page);

    await expect(page.locator('#display-screensaver-status')).toHaveText('Disabled');
  });
});

test.describe('Display Screenshot Tab', () => {
  test.beforeEach(async ({ page }) => {
    await bootPortal(page);
    await page.route('**/api/display/screenshot', route => {
      const dummyBuffer = Buffer.alloc(240 * 240 * 2);
      route.fulfill({
        status: 200,
        contentType: 'application/octet-stream',
        headers: {
          'X-Display-Width': '240',
          'X-Display-Height': '240',
          'X-Display-Format': 'RGB565',
        },
        body: dummyBuffer,
      });
    });
  });

  test('navigates to Screenshot tab and triggers capture', async ({ page }) => {
    await page.locator('#bottom-nav .tab-link[data-target="tab-display"]').click();
    await page.locator('#pill-bar .pill-btn[data-target="tab-screenshot"]').click();

    await page.locator('#btn-ss-capture').waitFor({ state: 'visible' });
    await expect(page.locator('#ss-canvas')).toBeVisible();

    const requestPromise = page.waitForRequest(req => req.url().includes('/api/display/screenshot'));
    await page.locator('#btn-ss-capture').click();
    const req = await requestPromise;
    expect(req.url()).toContain('/api/display/screenshot');
  });
});

