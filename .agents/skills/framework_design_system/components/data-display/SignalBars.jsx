import React from 'react';

function rssiToBars(rssi) {
  if (rssi === 0) return 0;
  if (rssi >= -50) return 4; if (rssi >= -60) return 3;
  if (rssi >= -70) return 2; if (rssi >= -80) return 1; return 0;
}

/** 4-bar WiFi signal strength from an RSSI (dBm) value, or an explicit bars count. */
export function SignalBars({ rssi, bars, style = {} }) {
  const n = bars != null ? bars : rssiToBars(rssi ?? 0);
  return (
    <span aria-label={'Signal: ' + n + ' of 4 bars'} style={{ display: 'inline-flex', alignItems: 'flex-end', gap: 2, height: 16, verticalAlign: 'middle', ...style }}>
      {[0, 1, 2, 3].map(i => (
        <span key={i} style={{ display: 'inline-block', width: 3, height: 4 + i * 3, borderRadius: 1,
          background: i < n ? 'var(--primary)' : 'var(--border)' }} />
      ))}
    </span>
  );
}
