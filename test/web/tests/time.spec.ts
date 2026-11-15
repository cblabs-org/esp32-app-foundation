import { test, expect, Page } from '@playwright/test';

// ---------------------------------------------------------------------------
// Mock data
// ---------------------------------------------------------------------------

// The `script` field is load-bearing: app.js discovers optional-module JS from
// /api/tabs and injects `/time.js`. Match TimeManager::registerTabs (order 15).
const MOCK_TABS = [
  { name: 'WiFi',   id: 'wifi',   parent: null,     order: 0 },
  { name: 'System', id: 'system', parent: null,     order: 100 },
  { name: 'Status', id: 'system', parent: 'System', order: 0 },
  { name: 'Keys',   id: 'keys',   parent: 'System', order: 1 },
  { name: 'Time',   id: 'time',   parent: 'System', order: 15, script: '/time.js' },
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

const MOCK_AUTH_STATUS = { enabled: false, passwordSet: false, sessionActive: false };

const MOCK_TIME_ZONES: Record<string, string> = {
  'America/New_York': 'EST5EDT,M3.2.0,M11.1.0',
  'Europe/London':    'GMT0BST,M3.5.0/1,M10.5.0',
  'Europe/Paris':     'CET-1CEST,M3.5.0,M10.5.0/3',
  'UTC':              'GMT0',
};

interface TimeConfig {
  timezone: string;
  posix: string;
  activeTimezone: string;
  activePosix: string;
  ntpServer1: string;
  ntpServer2: string;
  ntpServer3: string;
  dhcpNtp: boolean;
  dhcpTz: boolean;
  dhcpNtpActive: boolean;
  dhcpNtpServers: string[];
  dhcpTzActive: boolean;
  dhcpTzSource: 'none' | 'pcode' | 'tcode';
  configured: boolean;
  syncStatus: 'UNSYNCED' | 'IN_PROGRESS' | 'SYNCED';
  localTime?: string;
}

const DEFAULT_TIME_CONFIG: TimeConfig = {
  timezone: 'Europe/London',
  posix: 'GMT0BST,M3.5.0/1,M10.5.0',
  activeTimezone: 'America/New_York',
  activePosix: 'EST5EDT,M3.2.0,M11.1.0',
  ntpServer1: 'pool.ntp.org',
  ntpServer2: 'time.nist.gov',
  ntpServer3: '',
  dhcpNtp: true,
  dhcpTz: true,
  dhcpNtpActive: true,
  dhcpNtpServers: ['192.168.1.1', '10.0.0.5'],
  dhcpTzActive: true,
  dhcpTzSource: 'tcode',
  configured: true,
  syncStatus: 'SYNCED',
  localTime: '2026-06-03T22:00:00',
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

async function bootAndNavigateToTime(
  page: Page,
  initialConfig: TimeConfig,
  onSavePost?: (payload: Record<string, unknown>) => void,
  zones: Record<string, string> = MOCK_TIME_ZONES
) {
  let currentConfig: TimeConfig = { ...initialConfig };

  await page.route('**/api/app', route => route.fulfill({ json: { name: 'Foundation Test App' } }));
  await page.route('**/api/tabs', route => route.fulfill({ json: MOCK_TABS }));
  await page.route('**/api/system', route => route.fulfill({ json: MOCK_SYSTEM }));
  await page.route('**/api/wifi/status', route => route.fulfill({ json: MOCK_WIFI_STATUS }));
  await page.route('**/api/wifi/scan', route => route.fulfill({ json: { networks: [] } }));
  await page.route('**/api/auth/status', route => route.fulfill({ json: MOCK_AUTH_STATUS }));
  await page.route('**/api/time/zones', route => route.fulfill({ json: zones }));
  await page.route('**/api/time/config', async route => {
    if (route.request().method() === 'POST') {
      const parsed = JSON.parse(route.request().postData() || '{}') as Record<string, unknown>;
      if (onSavePost) onSavePost(parsed);
      currentConfig = {
        ...currentConfig,
        timezone: typeof parsed.timezone === 'string' ? parsed.timezone : currentConfig.timezone,
        posix: typeof parsed.posix === 'string' ? parsed.posix : currentConfig.posix,
        ntpServer1: typeof parsed.ntpServer1 === 'string' ? parsed.ntpServer1 : currentConfig.ntpServer1,
        ntpServer2: typeof parsed.ntpServer2 === 'string' ? parsed.ntpServer2 : currentConfig.ntpServer2,
        ntpServer3: typeof parsed.ntpServer3 === 'string' ? parsed.ntpServer3 : currentConfig.ntpServer3,
        dhcpNtp: typeof parsed.dhcpNtp === 'boolean' ? parsed.dhcpNtp : currentConfig.dhcpNtp,
        dhcpTz: typeof parsed.dhcpTz === 'boolean' ? parsed.dhcpTz : currentConfig.dhcpTz,
      };
      if (!currentConfig.dhcpTz) {
        currentConfig.dhcpTzActive = false;
        currentConfig.dhcpTzSource = 'none';
        currentConfig.activeTimezone = currentConfig.timezone;
        currentConfig.activePosix = currentConfig.posix;
      }
      if (!currentConfig.dhcpNtp) {
        currentConfig.dhcpNtpActive = false;
      }
      return route.fulfill({ json: { status: 'ok' } });
    }
    return route.fulfill({ json: currentConfig });
  });

  await page.goto('/');
  await page.waitForFunction(() => (window as any).apiFetch !== undefined);
  await expect(page.locator('body')).toHaveClass(/portal-ready/);

  await page.locator('#bottom-nav [data-target="tab-system"]').click();
  await expect(page.locator('#pill-bar')).toBeVisible();
  await page.locator('#pill-bar .pill-btn[data-target="tab-time"]').click();
  await expect(page.locator('#tab-time')).toHaveClass(/tab-content--active/);
  await expect(page.locator('#time-dhcp-tz')).toBeVisible();
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

test.describe('Time Tab — DHCP NTP & Timezone Controls', () => {
  test('displays active DHCP Option 101 (TCode) badge and Option 42 NTP servers', async ({ page }) => {
    await bootAndNavigateToTime(page, DEFAULT_TIME_CONFIG);

    const syncCard = page.locator('#time-sync-status');
    await expect(syncCard).toContainText('America/New_York');
    await expect(syncCard).toContainText('DHCP Option 101 (TCode)');
    await expect(syncCard).toContainText('EST5EDT,M3.2.0,M11.1.0');
    await expect(syncCard).toContainText('192.168.1.1');
    await expect(syncCard).toContainText('10.0.0.5');
    await expect(syncCard).toContainText('Option 42');

    await expect(page.locator('#time-dhcp-tz')).toBeChecked();
    const tzStatus = page.locator('#time-dhcp-tz-status');
    await expect(tzStatus).toContainText('DHCP Option 101 (TCode)');
    await expect(tzStatus).toContainText('America/New_York');
    await expect(tzStatus).toContainText('Active POSIX: EST5EDT,M3.2.0,M11.1.0');
    await expect(tzStatus).toContainText('Saved fallback: Europe/London');

    await page.locator('#tab-time summary.advanced-summary').click();
    await expect(page.locator('#time-dhcp-ntp')).toBeChecked();
    const ntpList = page.locator('#time-dhcp-ntp-servers');
    await expect(ntpList).toContainText('Option 42 Active');
    await expect(ntpList).toContainText('192.168.1.1');
    await expect(ntpList).toContainText('10.0.0.5');
    await expect(page.locator('#time-ntp1')).toHaveValue('pool.ntp.org');
  });

  test('displays active DHCP Option 100 (PCode) badge when router advertises PCode POSIX', async ({ page }) => {
    await bootAndNavigateToTime(page, {
      ...DEFAULT_TIME_CONFIG,
      activeTimezone: 'CET-1CEST,M3.5.0,M10.5.0/3',
      activePosix: 'CET-1CEST,M3.5.0,M10.5.0/3',
      dhcpTzActive: true,
      dhcpTzSource: 'pcode',
    });

    await expect(page.locator('#time-sync-status')).toContainText('DHCP Option 100 (PCode)');
    const tzStatus = page.locator('#time-dhcp-tz-status');
    await expect(tzStatus).toContainText('DHCP Option 100 (PCode)');
    await expect(tzStatus).toContainText('CET-1CEST,M3.5.0,M10.5.0/3');
    await expect(tzStatus).toContainText('Saved fallback: Europe/London');
  });

  test('shows manual fallback status cleanly when DHCP toggles are disabled', async ({ page }) => {
    await bootAndNavigateToTime(page, {
      ...DEFAULT_TIME_CONFIG,
      activeTimezone: 'Europe/London',
      activePosix: 'GMT0BST,M3.5.0/1,M10.5.0',
      dhcpNtp: false,
      dhcpTz: false,
      dhcpNtpActive: false,
      dhcpNtpServers: [],
      dhcpTzActive: false,
      dhcpTzSource: 'none',
    });

    await expect(page.locator('#time-dhcp-tz')).not.toBeChecked();
    const syncCard = page.locator('#time-sync-status');
    await expect(syncCard).toContainText('Europe/London');
    await expect(syncCard).not.toContainText('Option 100');
    await expect(syncCard).not.toContainText('Option 101');
    await expect(syncCard).not.toContainText('Option 42');

    const tzStatus = page.locator('#time-dhcp-tz-status');
    await expect(tzStatus).toContainText('Manual');
    await expect(tzStatus).toContainText('DHCP timezone override disabled');

    await page.locator('#tab-time summary.advanced-summary').click();
    await expect(page.locator('#time-dhcp-ntp')).not.toBeChecked();
    const ntpList = page.locator('#time-dhcp-ntp-servers');
    await expect(ntpList).toContainText('Manual');
    await expect(ntpList).toContainText('using static NTP servers below');
  });

  test('shows enabled-waiting fallback state when DHCP is enabled but router omits options', async ({ page }) => {
    await bootAndNavigateToTime(page, {
      ...DEFAULT_TIME_CONFIG,
      activeTimezone: 'Europe/London',
      activePosix: 'GMT0BST,M3.5.0/1,M10.5.0',
      dhcpNtp: true,
      dhcpTz: true,
      dhcpNtpActive: false,
      dhcpNtpServers: [],
      dhcpTzActive: false,
      dhcpTzSource: 'none',
    });

    await expect(page.locator('#time-dhcp-tz')).toBeChecked();
    const tzStatus = page.locator('#time-dhcp-tz-status');
    await expect(tzStatus).toContainText('Manual Fallback');
    await expect(tzStatus).toContainText('Europe/London');

    await page.locator('#tab-time summary.advanced-summary').click();
    await expect(page.locator('#time-dhcp-ntp')).toBeChecked();
    await expect(page.locator('#time-dhcp-ntp-servers')).toContainText('No DHCP NTP servers advertised');
  });

  test('updates badges immediately on toggle and persists dhcpTz/dhcpNtp booleans on Save', async ({ page }) => {
    let savedPayload: Record<string, unknown> | null = null;
    await bootAndNavigateToTime(page, DEFAULT_TIME_CONFIG, payload => {
      savedPayload = payload;
    });

    await page.locator('#time-dhcp-tz').uncheck();
    await expect(page.locator('#time-dhcp-tz-status')).toContainText('Manual');

    await page.locator('#tab-time summary.advanced-summary').click();
    await page.locator('#time-dhcp-ntp').uncheck();
    await expect(page.locator('#time-dhcp-ntp-servers')).toContainText('DHCP Option 42 disabled');

    await page.locator('#time-ntp1').fill('time.cloudflare.com');
    await page.locator('#btn-time-save').click();

    await expect.poll(() => savedPayload).not.toBeNull();
    expect(savedPayload).toMatchObject({
      timezone: 'Europe/London',
      posix: 'GMT0BST,M3.5.0/1,M10.5.0',
      ntpServer1: 'time.cloudflare.com',
      ntpServer2: 'time.nist.gov',
      ntpServer3: '',
      dhcpTz: false,
      dhcpNtp: false,
    });
  });

  test('escapes untrusted XSS payloads in DHCP PCode/TCode and NTP server strings', async ({ page }) => {
    const xssImg = '<img src=x onerror="window.__xssTriggered=true">';
    const xssScript = '<script>window.__xssTriggered=true</script>';
    const xssAttr = '" autofocus onfocus="window.__xssTriggered=true';

    await bootAndNavigateToTime(page, {
      ...DEFAULT_TIME_CONFIG,
      timezone: xssScript,
      posix: xssAttr,
      activeTimezone: xssImg,
      activePosix: `${xssScript}`,
      dhcpNtpServers: [xssImg, '192.168.1.1'],
      ntpServer1: xssAttr,
      dhcpTzActive: true,
      dhcpTzSource: 'pcode',
      dhcpNtpActive: true,
    });

    await page.locator('#tab-time summary.advanced-summary').click();

    expect(await page.evaluate(() => (window as any).__xssTriggered)).toBeUndefined();
    await expect(page.locator('#tab-time img')).toHaveCount(0);
    await expect(page.locator('#tab-time script')).toHaveCount(0);

    await expect(page.locator('#time-dhcp-tz-status')).toContainText(xssImg);
    await expect(page.locator('#time-dhcp-tz-status')).toContainText(xssScript);
    await expect(page.locator('#time-dhcp-ntp-servers')).toContainText(xssImg);
    await expect(page.locator('#time-ntp1')).toHaveValue(xssAttr);
  });

  test('mock server serves /api/time/config and /api/time/zones contracts', async ({ request }) => {
    const configRes = await request.get('/api/time/config');
    expect(configRes.status()).toBe(200);
    const configJson = await configRes.json();
    expect(configJson).toMatchObject({
      dhcpNtp: true,
      dhcpTz: true,
      dhcpTzSource: 'tcode',
      dhcpTzActive: true,
      dhcpNtpActive: true,
    });
    expect(Array.isArray(configJson.dhcpNtpServers)).toBe(true);

    const zonesRes = await request.get('/api/time/zones');
    expect(zonesRes.status()).toBe(200);
    const zonesJson = await zonesRes.json();
    expect(zonesJson['Europe/London']).toBe('GMT0BST,M3.5.0/1,M10.5.0');
  });
});
