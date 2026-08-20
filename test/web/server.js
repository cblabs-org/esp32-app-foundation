/**
 * Mock HTTP server for ESP32 portal web tests.
 *
 * Zero external dependencies — uses only Node.js built-ins (http, fs, path).
 * Serves static files from ../../web/portal/ and provides mock API responses
 * for every device endpoint the SPA consumes.
 *
 * Usage:
 *   node server.js              # Starts on port 3000 (default)
 *   PORT=4000 node server.js    # Starts on port 4000
 */

const http = require('http');
const fs   = require('fs');
const path = require('path');

const PORT = parseInt(process.env.PORT, 10) || 3000;

// ── Static file root ─────────────────────────────────────────────────
const PORTAL_ROOT = path.resolve(__dirname, '..', '..', 'web', 'portal');

const MIME_TYPES = {
  '.html': 'text/html',
  '.css':  'text/css',
  '.js':   'application/javascript',
  '.json': 'application/json',
  '.svg':  'image/svg+xml',
  '.png':  'image/png',
  '.ico':  'image/x-icon',
};

// ── Mock data ────────────────────────────────────────────────────────

const MOCK_TABS = [
  { name: 'WiFi',       id: 'wifi',       parent: null,      order: 0   },
  { name: 'System',     id: 'system',     parent: null,      order: 100 },
  { name: 'Status',     id: 'system',     parent: 'System',  order: 0   },
  { name: 'Keys',       id: 'keys',       parent: 'System',  order: 1   },
  { name: 'MCP',        id: 'mcp',        parent: 'System',  order: 2, script: 'mcp.js' },
  { name: 'Backup',     id: 'backup',     parent: 'System',  order: 3   },
  { name: 'OTA',        id: 'ota',        parent: 'System',  order: 4, script: 'ota.js' },
  { name: 'Display',    id: 'display',    parent: null,      order: 30, script: 'display.js' },
  { name: 'Settings',   id: 'settings',   parent: 'Display', order: 10  },
  { name: 'Screenshot', id: 'screenshot', parent: 'Display', order: 20  },
  { name: 'App',        id: 'app',        parent: null,      order: 40  },
];

const MOCK_SYSTEM = {
  firmware_version: '1.0.0-test',
  build_serial:     '20260601-001',
  sdk_version:      'v5.4.1',
  chip_model:       'ESP32-S3',
  chip_revision:    1,
  chip_cores:       2,
  flash_size:       8388608,
  partition:        'app0',
  uptime:           3661,        // 1h 1m 1s
  chip_temp:        48.5,        // Amber threshold
  heap_total:       249856,
  heap_free:        124928,      // ~50% — green threshold boundary
  heap_min_free:    45000,       // ~18% — red threshold
  heap_max_alloc:   110000,
  psram_total:      2097152,
  psram_free:       1800000,
  storage_used:     786432,
  storage_total:    1048576,     // 75% used — amber threshold
  ap_mode:          false,
  wifi_ssid:        'TestNetwork',
  wifi_ip:          '192.168.1.100',
  wifi_mac:         'AA:BB:CC:DD:EE:FF',
  wifi_rssi:        -55,
};

const MOCK_WIFI_STATUS = {
  mode:       'STA',
  ssid:       'TestNetwork',
  ip:         '192.168.1.100',
  mac:        'AA:BB:CC:DD:EE:FF',
  rssi:       -55,
  channel:    6,
  gateway:    '192.168.1.1',
  ap_mode:    false,
  passExists: true,
};

const MOCK_WIFI_SCAN = {
  networks: [
    { ssid: 'HomeWiFi',  rssi: -35, secure: true  },
    { ssid: 'OfficeNet', rssi: -60, secure: true  },
    { ssid: 'GuestNet',  rssi: -75, secure: false },
    { ssid: 'FarAway',   rssi: -90, secure: true  },
  ],
};

const MOCK_KEYS = [
  { id: 'weather_key'  },
  { id: 'wifi_password' },
  { id: 'api_token'    },
];

const MOCK_MCP_STATUS = { initialized: true, toolCount: 5 };
const MOCK_MCP_CONFIG = { enabled: true, readOnly: false };
const MOCK_MCP_RPC = {
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

const MOCK_DISPLAY_STATUS = {
  brightness:         200,
  flip:               false,
  screensaverEnabled: true,
  dimmed:             false,
  sleeping:           false,
  dimTimeout:         150000,
  blankTimeout:       300000,
  screensaverPolicy:  'FullSleep',
  rotationInterval:   8000,
  boardCount:         2,
  clockFontSize:      2,
  activeBoard:        'Clock',
};

const MOCK_CONFIG_BACKUP = {
  _meta: {
    backup_version:   1,
    firmware_version: '1.0.0-test',
    device_id:        'AA:BB:CC:DD:EE:FF',
    device_type:      'ESP32-S3',
    datetime:         '2026-06-03T22:00:00Z',
    timezone:         'Europe/London',
    module_count:     5,
  },
  wifi:    { ssid: 'TestNetwork', _v: 1 },
  mcp:     { enabled: true, readOnly: false, _v: 1 },
  time:    { timezone: 'Europe/London', ntpEnabled: true, _v: 1 },
  display: { brightness: 80, timeout: 30, _v: 1 },
  ota:     { channel: 'stable', autoCheck: true, _v: 1 },
};

const MOCK_CONFIG = {
  namespaces: ['wifi', 'mcp', 'time', 'display', 'ota'],
  count: 5,
};

// ── Helpers ──────────────────────────────────────────────────────────

/** Send a JSON response with the given status code. */
function jsonResponse(res, statusCode, data, extraHeaders) {
  const body = JSON.stringify(data);
  res.writeHead(statusCode, {
    'Content-Type':  'application/json',
    'Content-Length': Buffer.byteLength(body),
    ...extraHeaders,
  });
  res.end(body);
}

/** Collect the full body of a request and return it as a string. */
function collectBody(req) {
  return new Promise((resolve) => {
    const chunks = [];
    req.on('data', (chunk) => chunks.push(chunk));
    req.on('end', () => resolve(Buffer.concat(chunks).toString()));
  });
}

// ── API Router ───────────────────────────────────────────────────────

/**
 * Attempt to handle an API route. Returns true if a route matched,
 * false if the request should fall through to static file handling.
 */
async function handleApiRoute(req, res) {
  const { method, url } = req;

  // ── Boot-critical ────────────────────────────────────────────────
  if (method === 'GET' && url === '/api/app') {
    return jsonResponse(res, 200, { name: 'Foundation Test App' });
  }
  if (method === 'GET' && url === '/api/tabs') {
    return jsonResponse(res, 200, MOCK_TABS);
  }
  if (method === 'GET' && url === '/api/system') {
    return jsonResponse(res, 200, MOCK_SYSTEM);
  }

  // ── WiFi ─────────────────────────────────────────────────────────
  if (method === 'GET' && url === '/api/wifi/status') {
    return jsonResponse(res, 200, MOCK_WIFI_STATUS);
  }
  if (method === 'GET' && url === '/api/wifi/scan') {
    return jsonResponse(res, 200, MOCK_WIFI_SCAN);
  }
  if (method === 'POST' && url === '/api/wifi/test') {
    return jsonResponse(res, 200, { status: 'ok', ip: '192.168.1.100' });
  }
  if (method === 'POST' && url === '/api/wifi/connect') {
    return jsonResponse(res, 200, { status: 'connecting' });
  }
  if (method === 'POST' && url === '/api/wifi/forget') {
    return jsonResponse(res, 200, { status: 'forgotten' });
  }

  // ── System ───────────────────────────────────────────────────────
  if (method === 'POST' && url === '/api/system/restart') {
    return jsonResponse(res, 200, { status: 'restarting' });
  }
  if (method === 'POST' && url === '/api/system/factory-reset') {
    return jsonResponse(res, 200, { status: 'resetting' });
  }

  // ── Keys ─────────────────────────────────────────────────────────
  if (method === 'GET' && url === '/api/keys') {
    return jsonResponse(res, 200, MOCK_KEYS);
  }
  if (method === 'POST' && url === '/api/keys') {
    return jsonResponse(res, 200, { status: 'saved' });
  }
  if (method === 'DELETE' && url === '/api/keys') {
    return jsonResponse(res, 200, { status: 'deleted' });
  }

  // ── MCP ──────────────────────────────────────────────────────────
  if (method === 'GET' && url === '/api/mcp/status') {
    return jsonResponse(res, 200, MOCK_MCP_STATUS);
  }
  if (method === 'GET' && url === '/api/mcp/config') {
    return jsonResponse(res, 200, MOCK_MCP_CONFIG);
  }
  if (method === 'POST' && url === '/api/mcp/config') {
    const body = await collectBody(req);
    try {
      const parsed = JSON.parse(body);
      return jsonResponse(res, 200, parsed);
    } catch {
      return jsonResponse(res, 200, MOCK_MCP_CONFIG);
    }
  }
  if (method === 'POST' && url === '/api/mcp') {
    return jsonResponse(res, 200, MOCK_MCP_RPC);
  }

  // ── Backup / Config ──────────────────────────────────────────────
  if (method === 'GET' && url === '/api/config/backup') {
    return jsonResponse(res, 200, MOCK_CONFIG_BACKUP, {
      'Content-Disposition': 'attachment; filename="config-backup.json"',
    });
  }
  if (method === 'POST' && url === '/api/config/restore') {
    return jsonResponse(res, 200, { status: 'ok' });
  }
  if (method === 'GET' && url === '/api/config') {
    return jsonResponse(res, 200, MOCK_CONFIG);
  }

  // ── OTA ──────────────────────────────────────────────────────────
  if (method === 'POST' && url === '/api/ota') {
    const body = await collectBody(req);
    let action = '';
    try { action = JSON.parse(body).action; } catch { /* ignore */ }
    if (action === 'checkGithub') {
      return jsonResponse(res, 200, { msg: 'Update check started' });
    }
    if (action === 'status') {
      return jsonResponse(res, 200, { state: 'idle', lastError: '' });
    }
    if (action === 'rollback') {
      return jsonResponse(res, 200, { status: 'rolling back' });
    }
    return jsonResponse(res, 200, { status: 'ok' });
  }
  if (method === 'POST' && url === '/update') {
    return jsonResponse(res, 200, { status: 'ok' });
  }

  // ── Display ──────────────────────────────────────────────────────
  if (method === 'GET' && url === '/api/display/status') {
    return jsonResponse(res, 200, MOCK_DISPLAY_STATUS);
  }
  if (method === 'POST' && url === '/api/display/brightness') {
    return jsonResponse(res, 200, { status: 'ok' });
  }
  if (method === 'POST' && url === '/api/display/flip') {
    return jsonResponse(res, 200, { status: 'ok' });
  }
  if (method === 'POST' && url === '/api/display/sleep') {
    return jsonResponse(res, 200, { status: 'ok' });
  }
  if (method === 'POST' && url === '/api/display/clockFontSize') {
    return jsonResponse(res, 200, { status: 'ok' });
  }

  // ── Catch-all for unmatched /api/* routes ────────────────────────
  if (url.startsWith('/api/')) {
    return jsonResponse(res, 404, { error: 'Not found' });
  }

  // Not an API route — let static handler take over
  return false;
}

// ── Static file handler ──────────────────────────────────────────────

function serveStaticFile(req, res) {
  let filePath = req.url;

  // Default to index.html
  if (filePath === '/' || filePath === '/index.html') {
    filePath = '/index.html';
  }

  const cleanPath = filePath.split('?')[0];
  const filename = path.basename(cleanPath);

  const searchDirs = [
    PORTAL_ROOT,
    path.resolve(__dirname, '..', '..', 'web', 'display'),
  ];
  const modulesDir = path.resolve(__dirname, '..', '..', 'modules');
  if (fs.existsSync(modulesDir)) {
    for (const mod of fs.readdirSync(modulesDir)) {
      const modWeb = path.join(modulesDir, mod, 'web');
      if (fs.existsSync(modWeb) && fs.statSync(modWeb).isDirectory()) {
        searchDirs.push(modWeb);
      }
    }
  }

  let foundPath = null;
  for (const dir of searchDirs) {
    const candidate = path.join(dir, filename);
    if (fs.existsSync(candidate) && fs.statSync(candidate).isFile()) {
      foundPath = candidate;
      break;
    }
  }

  if (!foundPath) {
    res.writeHead(404, { 'Content-Type': 'text/plain' });
    res.end('Not Found');
    return;
  }

  fs.readFile(foundPath, (err, data) => {
    if (err) {
      res.writeHead(404, { 'Content-Type': 'text/plain' });
      res.end('Not Found');
      return;
    }

    const ext = path.extname(foundPath).toLowerCase();
    const mimeType = MIME_TYPES[ext] || 'application/octet-stream';

    res.writeHead(200, {
      'Content-Type':  mimeType,
      'Content-Length': data.length,
    });
    res.end(data);
  });
}

// ── Server ───────────────────────────────────────────────────────────

const server = http.createServer(async (req, res) => {
  console.log(`[mock-server] ${req.method} ${req.url}`);

  // Try API routes first
  const handled = await handleApiRoute(req, res);
  if (handled !== false) return;

  // Fall through to static file serving
  serveStaticFile(req, res);
});

server.listen(PORT, () => {
  console.log(`[mock-server] Listening on http://localhost:${PORT}`);
});
