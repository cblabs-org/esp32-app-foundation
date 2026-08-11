import React from 'react';

const BG = { success: 'var(--success)', error: 'var(--error)', warning: 'var(--warning)', info: 'var(--primary)' };

/** Transient notification pill. Render inside a bottom-centred stack. */
export function Toast({ type = 'info', visible = true, children, style = {} }) {
  return (
    <div style={{ padding: '0.75rem 1rem', borderRadius: 'var(--radius-lg)', fontSize: 'var(--text-body)',
      fontWeight: 'var(--weight-semibold)', color: '#fff', background: BG[type] || '#333',
      fontFamily: 'var(--font-sans)', transition: 'all var(--transition-slow)',
      transform: visible ? 'translateY(0)' : 'translateY(20px)', opacity: visible ? 1 : 0,
      boxShadow: 'var(--shadow-card-hover)', ...style }}>{children}</div>
  );
}
