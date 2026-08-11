import React from 'react';

const COLOR = { green: 'var(--success)', red: 'var(--error)', yellow: 'var(--warning)', grey: 'var(--text-muted)' };

/** Glowing status indicator dot. Optional pulse animation and trailing label. */
export function StatusDot({ status = 'grey', size = 10, pulse = false, label, style = {} }) {
  const glow = status === 'grey' ? 0.4 : 0.8;
  const dot = <span style={{ width: size, height: size, borderRadius: '50%', display: 'inline-block',
    background: COLOR[status], boxShadow: '0 0 ' + (size * glow) + 'px ' + COLOR[status],
    animation: pulse ? 'af-pulse 1s infinite' : undefined, flexShrink: 0, ...style }} />;
  if (!label) return dot;
  return <span style={{ display: 'inline-flex', alignItems: 'center', gap: 8, fontSize: 'var(--text-body)', fontWeight: 'var(--weight-semibold)', color: 'var(--text)' }}>{dot}{label}</span>;
}
