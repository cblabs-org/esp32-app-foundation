import React from 'react';

/** Label/value row with a hairline underline. Used throughout device info panels. */
export function InfoRow({ label, value, last = false, style = {} }) {
  return (
    <div style={{ display: 'flex', justifyContent: 'space-between', padding: '0.4rem 0',
      borderBottom: last ? 'none' : '1px solid var(--border)', fontSize: 'var(--text-body)',
      fontFamily: 'var(--font-sans)', ...style }}>
      <span style={{ color: 'var(--text-muted)' }}>{label}</span>
      <strong style={{ color: 'var(--text)' }}>{value}</strong>
    </div>
  );
}
