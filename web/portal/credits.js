/**
 * credits.js — Credits tab fragment
 *
 * Renders a styled card listing open-source dependencies used by this
 * firmware. Purely client-side — no API endpoint required.
 *
 * Pattern: IIFE that populates its container div via innerHTML.
 */
(function() {
  var container = document.getElementById('tab-credits');
  if (!container) return;

  var libs = [
    { name: 'ESP-IDF',        author: 'Espressif Systems',          licence: 'Apache-2.0',    url: 'https://github.com/espressif/esp-idf' },
    { name: 'Arduino-ESP32',  author: 'Espressif Systems',          licence: 'LGPL-2.1',      url: 'https://github.com/espressif/arduino-esp32' },
    { name: 'ArduinoJson',    author: 'Benoît Blanchon',            licence: 'MIT',            url: 'https://github.com/bblanchon/ArduinoJson' },
    { name: 'PsychicHttp',    author: 'Hoeken / Zach Hoeken',       licence: 'LGPL-2.1',      url: 'https://github.com/hoeken/PsychicHttp' },
    { name: 'U8g2',           author: 'Oliver Kraus (olikraus)',     licence: 'BSD-2-Clause',  url: 'https://github.com/olikraus/u8g2' },
    { name: 'mbedTLS',        author: 'TrustedFirmware.org (ARM)',   licence: 'Apache-2.0',    url: 'https://github.com/Mbed-TLS/mbedtls' },
    { name: 'GoogleTest',     author: 'Google',                      licence: 'BSD-3-Clause',  url: 'https://github.com/google/googletest' }
  ];

  var html = '<div class="card">';
  html += '<h3 class="card-title">Open Source Credits</h3>';
  html += '<p style="margin:0 0 16px;opacity:0.8;font-size:0.95em;">' +
    'This firmware is built with the following open-source components. ' +
    'We gratefully acknowledge the work of their authors.</p>';

  for (var i = 0; i < libs.length; i++) {
    var lib = libs[i];
    html += '<div style="display:flex;align-items:center;justify-content:space-between;' +
      'padding:10px 0;border-bottom:1px solid var(--card-border,rgba(255,255,255,0.08));">';
    html += '<div style="flex:1;min-width:0;">';
    html += '<strong style="font-size:1em;">' + lib.name + '</strong>';
    html += '<span style="display:inline-block;margin-left:8px;padding:2px 8px;' +
      'border-radius:12px;font-size:0.75em;background:var(--accent,#6c63ff);' +
      'color:#fff;vertical-align:middle;">' + lib.licence + '</span>';
    html += '<div style="font-size:0.85em;opacity:0.7;margin-top:2px;">' + lib.author + '</div>';
    html += '</div>';
    html += '<a href="' + lib.url + '" target="_blank" rel="noopener noreferrer" ' +
      'style="flex-shrink:0;margin-left:12px;opacity:0.7;" title="View on GitHub">' +
      '<svg viewBox="0 0 24 24" width="18" height="18" fill="none" ' +
      'stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">' +
      '<path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"/>' +
      '<polyline points="15 3 21 3 21 9"/>' +
      '<line x1="10" y1="14" x2="21" y2="3"/></svg></a>';
    html += '</div>';
  }

  html += '<div style="margin-top:16px;padding-top:12px;' +
    'border-top:1px solid var(--card-border,rgba(255,255,255,0.08));' +
    'font-size:0.9em;opacity:0.8;text-align:center;">';
  html += 'Built with <strong>ESP32 App Foundation</strong> by CB Labs &mdash; ';
  html += '<a href="https://github.com/matthewmcneill/esp32-app-foundation" ' +
    'target="_blank" rel="noopener noreferrer" style="color:var(--accent,#6c63ff);">' +
    'View on GitHub</a>';
  html += '</div>';
  html += '</div>';

  container.innerHTML = html;
})();
