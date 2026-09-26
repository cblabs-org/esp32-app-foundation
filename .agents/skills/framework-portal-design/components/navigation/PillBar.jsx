import React from 'react';

/** Horizontal scrolling sub-navigation of rounded pill buttons. */
export function PillBar({ items = [], active, onSelect, style = {} }) {
  return (
    <div style={{ display: 'flex', gap: 8, padding: '0.5rem 0', overflowX: 'auto', marginBottom: '0.5rem', fontFamily: 'var(--font-sans)', ...style }}>
      {items.map(it => {
        const id = typeof it === 'string' ? it : it.id;
        const label = typeof it === 'string' ? it : it.label;
        const on = id === active;
        return (
          <button key={id} onClick={() => onSelect && onSelect(id)} style={{
            padding: '0.4rem 1rem', borderRadius: 'var(--radius-pill)', whiteSpace: 'nowrap', flexShrink: 0,
            border: '1px solid ' + (on ? 'var(--primary)' : 'var(--border)'),
            background: on ? 'var(--primary)' : 'transparent', color: on ? '#fff' : 'var(--text-muted)',
            fontSize: 'var(--text-base-sm)', fontWeight: 'var(--weight-semibold)', cursor: 'pointer',
            transition: 'all var(--transition-base)' }}>{label}</button>
        );
      })}
    </div>
  );
}
