import React from 'react';

/** Left-accented instruction block. Blue by default. */
export function Callout({ children, style = {} }) {
  return (
    <div style={{ borderLeft: '4px solid var(--primary)', background: 'var(--primary-alpha)',
      padding: '0.75rem 1rem', borderRadius: '0 var(--radius-md) var(--radius-md) 0',
      marginBottom: '1rem', fontSize: 'var(--text-body)', lineHeight: 'var(--leading-body)',
      color: 'var(--text)', fontFamily: 'var(--font-sans)', ...style }}>{children}</div>
  );
}
