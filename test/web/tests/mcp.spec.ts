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

const MOCK_MCP_STATUS_ACTIVE = { initialized: true, toolCount: 3 };
const MOCK_MCP_STATUS_INACTIVE = { initialized: false, toolCount: 0 };

const MOCK_MCP_CONFIG = { enabled: true, readOnly: false };

const MOCK_MCP_RPC_TOOLS = {
  jsonrpc: '2.0',
  id: 1,
  result: {
    tools: [
      { name: 'get_system_info',  description: 'Get device system information', mutatesState: false },
      { name: 'restart_device',   description: 'Restart the device',           mutatesState: true  },
      { name: 'list_keys',        description: 'List stored API keys',         mutatesState: false },
    ],
  },
};

const MOCK_AUTH_STATUS = { enabled: false, passwordSet: false, sessionActive: false };

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

async function bootAndNavigateToMcp(
  page: Page,
  mcpStatus = MOCK_MCP_STATUS_ACTIVE,
  mcpConfig = MOCK_MCP_CONFIG,
  mcpRpc = MOCK_MCP_RPC_TOOLS
) {
  await page.route('**/api/app', route => route.fulfill({ json: { name: 'Foundation Test App' } }));
  await page.route('**/api/tabs', route => route.fulfill({ json: MOCK_TABS }));
  await page.route('**/api/system', route => route.fulfill({ json: MOCK_SYSTEM }));
  await page.route('**/api/wifi/status', route => route.fulfill({ json: MOCK_WIFI_STATUS }));
  await page.route('**/api/wifi/scan', route => route.fulfill({ json: { networks: [] } }));
  await page.route('**/api/mcp/status', route => route.fulfill({ json: mcpStatus }));
  await page.route('**/api/mcp/config', route => {
    if (route.request().method() === 'POST') {
      return route.fulfill({ json: mcpConfig });
    }
    return route.fulfill({ json: mcpConfig });
  });
  await page.route('**/api/mcp', route => route.fulfill({ json: mcpRpc }));
  await page.route('**/api/auth/status', route => route.fulfill({ json: MOCK_AUTH_STATUS }));

  await page.goto('/');
  await page.waitForFunction(() => (window as any).apiFetch !== undefined);
  await expect(page.locator('#portal-title')).not.toHaveText('ESP32 App Foundation');

  // Navigate to System → MCP
  await page.locator('#bottom-nav [data-target="tab-system"]').click();
  await expect(page.locator('#pill-bar')).toBeVisible();
  await page.locator('#pill-bar .pill-btn[data-target="tab-mcp"]').click();
  await expect(page.locator('#tab-mcp')).toHaveClass(/tab-content--active/);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

test.describe('MCP Controls Sub-pill', () => {
  test('status indicator shows Active when initialized is true', async ({ page }) => {
    await bootAndNavigateToMcp(page);

    const controls = page.locator('#mcp-controls');
    await expect(controls).toContainText('● Active');
  });

  test('status indicator shows Inactive when initialized is false', async ({ page }) => {
    await bootAndNavigateToMcp(page, MOCK_MCP_STATUS_INACTIVE);

    const controls = page.locator('#mcp-controls');
    await expect(controls).toContainText('● Inactive');
  });

  test('tool count badge shows count from /api/mcp/status', async ({ page }) => {
    await bootAndNavigateToMcp(page);

    const controls = page.locator('#mcp-controls');
    await expect(controls).toContainText('Registered Tools:');
    await expect(controls).toContainText('3');
  });

  test('toggle switches render from /api/mcp/config and POST on change', async ({ page }) => {
    let configPostReceived = false;

    await page.route('**/api/app', route => route.fulfill({ json: { name: 'Foundation Test App' } }));
    await page.route('**/api/tabs', route => route.fulfill({ json: MOCK_TABS }));
    await page.route('**/api/system', route => route.fulfill({ json: MOCK_SYSTEM }));
    await page.route('**/api/wifi/status', route => route.fulfill({ json: MOCK_WIFI_STATUS }));
    await page.route('**/api/wifi/scan', route => route.fulfill({ json: { networks: [] } }));
    await page.route('**/api/mcp/status', route => route.fulfill({ json: MOCK_MCP_STATUS_ACTIVE }));
    await page.route('**/api/mcp/config', route => {
      if (route.request().method() === 'POST') {
        configPostReceived = true;
        return route.fulfill({ json: { enabled: false, readOnly: false } });
      }
      return route.fulfill({ json: MOCK_MCP_CONFIG });
    });
    await page.route('**/api/mcp', route => route.fulfill({ json: MOCK_MCP_RPC_TOOLS }));
    await page.route('**/api/auth/status', route => route.fulfill({ json: MOCK_AUTH_STATUS }));

    await page.goto('/');
    await page.waitForFunction(() => (window as any).apiFetch !== undefined);
    await expect(page.locator('#portal-title')).not.toHaveText('ESP32 App Foundation');

    // Navigate to MCP
    await page.locator('#bottom-nav [data-target="tab-system"]').click();
    await expect(page.locator('#pill-bar')).toBeVisible();
    await page.locator('#pill-bar .pill-btn[data-target="tab-mcp"]').click();

    // Wait for controls to render
    await expect(page.locator('#mcp-controls')).toContainText('Server Enabled');

    // The toggle checkboxes are invisible (opacity:0, width:0, height:0) as
    // they're part of a CSS toggle-switch pattern. Check they exist and are checked.
    const enabledToggle = page.locator('#mcp-toggle-enabled');
    const readOnlyToggle = page.locator('#mcp-toggle-readonly');

    // They exist in the DOM
    await expect(enabledToggle).toBeAttached();
    await expect(readOnlyToggle).toBeAttached();

    // Enabled toggle should be checked (config has enabled: true)
    await expect(enabledToggle).toBeChecked();

    // Click the enabled toggle via its parent label to toggle it
    // The checkbox is inside a <label class="toggle-switch"> — clicking the label triggers it
    await enabledToggle.evaluate((el: HTMLInputElement) => el.click());

    // Should have sent a POST
    expect(configPostReceived).toBe(true);
  });

  test('tool list renders with MUTATING and READ-ONLY badges', async ({ page }) => {
    await bootAndNavigateToMcp(page);

    const toolsList = page.locator('#mcp-tools-list');

    // Wait for tools to render
    await expect(toolsList).toContainText('get_system_info');
    await expect(toolsList).toContainText('restart_device');
    await expect(toolsList).toContainText('list_keys');

    // get_system_info (mutatesState: false) → READ-ONLY badge
    await expect(toolsList).toContainText('READ-ONLY');

    // restart_device (mutatesState: true) → MUTATING badge
    await expect(toolsList).toContainText('MUTATING');
  });
});
