/**
 * Unit test for BLE portal tab (ble.js) DOM lifecycle.
 * Verifies that pair, forget, connect, and disconnect lifecycle events
 * update the DOM in real-time without requiring a page refresh.
 *
 * Runs with Node.js built-in test runner:
 *   node --test test/web/ble_lifecycle.test.js
 */

const test = require('node:test');
const assert = require('node:assert');
const fs = require('node:fs');
const path = require('node:path');

// Minimal mock DOM implementation to execute ble.js
function createMockEnvironment() {
  const elements = new Map();
  const listeners = new Map();

  class MockElement {
    constructor(tagName, id = '') {
      this.tagName = tagName.toUpperCase();
      this.id = id;
      this.className = '';
      this.classList = {
        _classes: new Set(),
        add: (...cls) => cls.forEach(c => this.classList._classes.add(c)),
        remove: (...cls) => cls.forEach(c => this.classList._classes.delete(c)),
        toggle: (c, force) => {
          if (force === undefined) {
            if (this.classList._classes.has(c)) this.classList._classes.delete(c);
            else this.classList._classes.add(c);
          } else if (force) {
            this.classList._classes.add(c);
          } else {
            this.classList._classes.delete(c);
          }
        },
        contains: (c) => this.classList._classes.has(c),
      };
      this.attributes = new Map();
      this.children = [];
      this.parentElement = null;
      this._innerHTML = '';
      this.textContent = '';
      this.style = {};
      this.disabled = false;
      this._eventListeners = new Map();
    }

    setAttribute(name, value) {
      this.attributes.set(name, String(value));
    }

    getAttribute(name) {
      return this.attributes.has(name) ? this.attributes.get(name) : null;
    }

    removeAttribute(name) {
      this.attributes.delete(name);
    }

    appendChild(child) {
      child.parentElement = this;
      this.children.push(child);
      return child;
    }

    addEventListener(event, fn) {
      if (!this._eventListeners.has(event)) {
        this._eventListeners.set(event, []);
      }
      this._eventListeners.get(event).push(fn);
    }

    dispatchEvent(event) {
      const fns = this._eventListeners.get(event.type) || [];
      fns.forEach(fn => fn(event));
    }

    get innerHTML() {
      if (this._innerHTML) return this._innerHTML;
      if (this.children.length > 0) {
        return this.children.map(c => c.outerHTML).join('');
      }
      return this.textContent || '';
    }

    get outerHTML() {
      const attrs = Array.from(this.attributes.entries()).map(([k, v]) => ` ${k}="${v}"`).join('');
      const classAttr = this.classList._classes.size > 0 ? ` class="${Array.from(this.classList._classes).join(' ')}"` : '';
      const idAttr = this.id ? ` id="${this.id}"` : '';
      const allAttrs = idAttr + classAttr + attrs;
      if (VOID_TAGS.has(this.tagName.toLowerCase())) {
        return `<${this.tagName.toLowerCase()}${allAttrs} />`;
      }
      return `<${this.tagName.toLowerCase()}${allAttrs}>${this.innerHTML}</${this.tagName.toLowerCase()}>`;
    }

    set innerHTML(html) {
      this._innerHTML = html;
      this.children = [];
      parseSimpleHtml(html, this);
    }

    querySelector(selector) {
      return querySelectorAll(this, selector)[0] || null;
    }

    querySelectorAll(selector) {
      return querySelectorAll(this, selector);
    }
  }

  const VOID_TAGS = new Set(['area', 'base', 'br', 'col', 'embed', 'hr', 'img', 'input', 'link', 'meta', 'param', 'source', 'track', 'wbr', 'circle', 'line', 'polyline', 'path', 'rect']);

  function parseSimpleHtml(html, rootParent) {
    const tokenRegex = /<!--[\s\S]*?-->|<(\/)?([a-zA-Z0-9]+)([^>]*)>|([^<]+)/g;
    const stack = [rootParent];
    let match;

    while ((match = tokenRegex.exec(html)) !== null) {
      if (match[0].startsWith('<!--')) continue;

      const isClosing = match[1] === '/';
      const tagName = match[2];
      const attrStr = match[3];
      const text = match[4];

      if (text) {
        const trimmed = text.trim();
        if (trimmed && stack.length > 0) {
          const top = stack[stack.length - 1];
          top.textContent = (top.textContent ? top.textContent + ' ' : '') + trimmed;
        }
        continue;
      }

      if (isClosing) {
        const lowerName = tagName.toLowerCase();
        for (let i = stack.length - 1; i > 0; i--) {
          if (stack[i].tagName.toLowerCase() === lowerName) {
            stack.splice(i);
            break;
          }
        }
        continue;
      }

      // Opening tag
      const el = new MockElement(tagName);
      if (attrStr) {
        const attrRegex = /([a-zA-Z0-9_-]+)(?:="([^"]*)")?/g;
        let aMatch;
        while ((aMatch = attrRegex.exec(attrStr)) !== null) {
          const aName = aMatch[1];
          const aVal = aMatch[2] !== undefined ? aMatch[2] : '';
          if (aName === 'id') {
            el.id = aVal;
            elements.set(aVal, el);
          } else if (aName === 'class') {
            el.className = aVal;
            aVal.split(/\s+/).filter(Boolean).forEach(c => el.classList.add(c));
          } else {
            el.setAttribute(aName, aVal);
          }
        }
      }

      stack[stack.length - 1].appendChild(el);

      const isSelfClosing = attrStr.trim().endsWith('/') || VOID_TAGS.has(tagName.toLowerCase());
      if (!isSelfClosing) {
        stack.push(el);
      }
    }
  }

  function querySelectorAll(root, selector) {
    const results = [];
    function matchNode(node) {
      if (node !== root && matchesSelector(node, selector)) {
        results.push(node);
      }
      for (const child of node.children) {
        matchNode(child);
      }
    }
    matchNode(root);
    return results;
  }

  function matchesSelector(node, sel) {
    sel = sel.trim();
    if (sel.startsWith('#')) {
      return node.id === sel.slice(1);
    }
    // Combined like .ble-provider[data-provider="..."]
    const comb = /^\.([a-zA-Z0-9_-]+)\[([a-zA-Z0-9_-]+)="([^"]*)"\]$/.exec(sel);
    if (comb) {
      return node.classList.contains(comb[1]) && node.getAttribute(comb[2]) === comb[3];
    }
    if (sel.startsWith('.')) {
      const className = sel.slice(1);
      return node.classList.contains(className);
    }
    if (sel.startsWith('[')) {
      const match = /^\[([a-zA-Z0-9_-]+)(?:="([^"]*)")?\]$/.exec(sel);
      if (match) {
        const val = node.getAttribute(match[1]);
        if (match[2] === undefined) return val !== null;
        return val === match[2];
      }
    }
    return node.tagName.toLowerCase() === sel.toLowerCase();
  }

  const document = {
    visibilityState: 'visible',
    head: new MockElement('head'),
    body: new MockElement('body'),
    createElement: (tag) => new MockElement(tag),
    getElementById: (id) => {
      if (elements.has(id)) return elements.get(id);
      return querySelectorAll(document.body, '#' + id)[0] || null;
    },
    querySelector: (sel) => querySelectorAll(document.body, sel)[0] || null,
    querySelectorAll: (sel) => querySelectorAll(document.body, sel),
    addEventListener: (event, fn) => {
      if (!listeners.has(event)) listeners.set(event, []);
      listeners.get(event).push(fn);
    },
  };

  const window = {
    document,
    apiFetch: null,
    showToast: () => {},
    addEventListener: (event, fn) => {
      if (!listeners.has(event)) listeners.set(event, []);
      listeners.get(event).push(fn);
    },
    dispatchEvent: (event) => {
      const fns = listeners.get(event.type) || [];
      fns.forEach(fn => fn(event));
    },
  };

  let intervalCallbacks = [];
  const mockSetInterval = (fn, ms) => {
    const id = { fn, ms };
    intervalCallbacks.push(id);
    return id;
  };
  const mockClearInterval = (id) => {
    intervalCallbacks = intervalCallbacks.filter(item => item !== id);
  };
  const triggerPoll = async () => {
    for (const item of [...intervalCallbacks]) {
      await item.fn();
    }
  };

  const tabBluetooth = new MockElement('div', 'tab-bluetooth');
  document.body.appendChild(tabBluetooth);
  elements.set('tab-bluetooth', tabBluetooth);

  return { window, document, elements, tabBluetooth, mockSetInterval, mockClearInterval, triggerPoll };
}

test('BLE portal tab DOM lifecycle updates without refresh', async () => {
  const { window, document, tabBluetooth, mockSetInterval, mockClearInterval, triggerPoll } = createMockEnvironment();

  try {
    // Load ble.js script in the mock environment
    const scriptContent = fs.readFileSync(
      path.resolve(__dirname, '../../modules/BLEManager/web/ble.js'),
      'utf-8'
    );

    let currentBleStatus = {
      state: 'Active',
      isScanning: false,
      isAdvertising: false,
      advertisedName: 'TestDevice-BLE',
      bleAddress: 'AA:BB:CC:11:22:33',
      devices: [
        {
          providerId: 'hr_strap',
          label: 'Heart Rate Strap',
          connected: false,
          stored: null,
          reconnect: { phase: 'Idle', suspended: false },
        },
      ],
      connections: [],
      counters: { droppedEvents: 0, scanInhibitCount: 0 },
      discovery: { active: false, providerId: '', durationMs: 0 },
    };

    let apiCalls = [];
    window.apiFetch = async (url, opts) => {
      apiCalls.push({ url, opts });
      if (url === '/api/ble/status') {
        return JSON.parse(JSON.stringify(currentBleStatus));
      }
      if (url === '/api/ble/forget') {
        currentBleStatus.devices[0].stored = null;
        currentBleStatus.devices[0].connected = false;
        return { status: 'forgotten' };
      }
      if (url === '/api/ble/pair') {
        const body = JSON.parse(opts.body);
        currentBleStatus.devices[0].stored = {
          name: 'Polar H10',
          address: body.address,
          addressType: body.addressType,
          bonded: true,
        };
        return { status: 'pairing_initiated' };
      }
      if (url === '/api/ble/scan/results') {
        return [];
      }
      return {};
    };

    // Run script
    const fn = new Function('window', 'document', 'setInterval', 'clearInterval', scriptContent);
    fn(window, document, mockSetInterval, mockClearInterval);

    // 1. Activate tab-bluetooth
    window.dispatchEvent({ type: 'tab-changed', detail: 'tab-bluetooth' });

    // Wait for initial fetch
    await new Promise(r => setTimeout(r, 20));

    // Verify Initial State: Unpaired
    const storedContainer = tabBluetooth.querySelector('.ble-stored-container');
    assert.ok(storedContainer, 'Stored container exists');
    const noDevice = storedContainer.querySelector('.ble-no-device');
    assert.ok(noDevice, 'Shows no device paired yet initially');
    assert.match(noDevice.textContent, /No device paired yet/);

    const badgeContainer = tabBluetooth.querySelector('.ble-provider-badge-container');
    assert.ok(badgeContainer, 'Badge container exists');
    assert.match(badgeContainer.innerHTML, /Disconnected/);

    // 2. Simulate Pairing a Device
    await window.apiFetch('/api/ble/pair', {
      method: 'POST',
      body: JSON.stringify({
        providerId: 'hr_strap',
        address: 'E0:11:22:33:44:55',
        addressType: 'Random',
      }),
    });

    // Background connect event arrives: device is now connected
    currentBleStatus.devices[0].connected = true;

    // Next 1 Hz background poll arrives
    await triggerPoll();
    await new Promise(r => setTimeout(r, 20));

    // Verify State After Pair: Device is displayed, "No device paired yet" is GONE
    const pairedStored = tabBluetooth.querySelector('.ble-stored');
    assert.ok(pairedStored, 'Stored device element is rendered after pair');
    const noDeviceAfterPair = tabBluetooth.querySelector('.ble-no-device');
    assert.strictEqual(noDeviceAfterPair, null, 'No device text is removed after pair');

    // Verify Badge is now Connected
    assert.match(badgeContainer.innerHTML, /Connected/);
    assert.doesNotMatch(badgeContainer.innerHTML, /Disconnected/);

    // Verify Forget button exists and is wired
    const forgetBtn = tabBluetooth.querySelector('.ble-forget-btn');
    assert.ok(forgetBtn, 'Forget button is present');

    // 3. Simulate Forget Action
    // Click 1: Arm button
    forgetBtn.dispatchEvent({ type: 'click', preventDefault: () => {} });
    assert.strictEqual(forgetBtn.textContent, 'Confirm Forget?');
    assert.strictEqual(forgetBtn.getAttribute('data-armed'), 'true');

    // Click 2: Confirm Forget
    forgetBtn.dispatchEvent({ type: 'click', preventDefault: () => {} });
    assert.strictEqual(forgetBtn.textContent, 'Forgetting...');

    // Wait for forget promise and status re-fetch
    await new Promise(r => setTimeout(r, 30));

    // Verify State After Forget: "Forgetting..." button is GONE, replaced by "No device paired yet"
    const noDeviceAfterForget = tabBluetooth.querySelector('.ble-no-device');
    assert.ok(noDeviceAfterForget, 'No device text returns after forget');
    assert.match(noDeviceAfterForget.textContent, /No device paired yet/);
    const forgetBtnAfterForget = tabBluetooth.querySelector('.ble-forget-btn');
    assert.strictEqual(forgetBtnAfterForget, null, 'Forget button is completely removed after forget');

    // Verify Badge is back to Disconnected
    assert.match(badgeContainer.innerHTML, /Disconnected/);

    // 4. Test Reconnect Phase Lifecycle
    // Pair device again
    await window.apiFetch('/api/ble/pair', {
      method: 'POST',
      body: JSON.stringify({
        providerId: 'hr_strap',
        address: 'E0:11:22:33:44:55',
        addressType: 'Random',
      }),
    });
    currentBleStatus.devices[0].connected = true;
    currentBleStatus.devices[0].reconnect = { phase: 'Idle', suspended: false };
    await triggerPoll();
    await new Promise(r => setTimeout(r, 20));
    assert.match(badgeContainer.innerHTML, /Connected/);

    // Connection drops -> Reconnecting (Fast)
    currentBleStatus.devices[0].connected = false;
    currentBleStatus.devices[0].reconnect = { phase: 'Fast', suspended: false };
    await triggerPoll();
    await new Promise(r => setTimeout(r, 20));
    assert.match(badgeContainer.innerHTML, /Reconnecting \(Fast\)/);

    // Connection retrying -> Reconnecting (Slow)
    currentBleStatus.devices[0].reconnect = { phase: 'Slow', suspended: false };
    await triggerPoll();
    await new Promise(r => setTimeout(r, 20));
    assert.match(badgeContainer.innerHTML, /Reconnecting \(Slow\)/);

    // Reconnected successfully
    currentBleStatus.devices[0].connected = true;
    currentBleStatus.devices[0].reconnect = { phase: 'Idle', suspended: false };
    await triggerPoll();
    await new Promise(r => setTimeout(r, 20));
    assert.match(badgeContainer.innerHTML, /Connected/);
  } catch (err) {
    console.error('TEST ERROR:', err);
    throw err;
  } finally {
    window.dispatchEvent({ type: 'tab-changed', detail: 'tab-other' });
  }
});
