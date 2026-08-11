import React from 'react';

/** Small pill label for status/meta. Variants: ok | warn | subtle | mode. */
export function Badge({ variant = 'subtle', children, style = {} }) {
  const variants = {
    ok:     { background: 'rgba(46,125,50,0.12)', color: 'var(--success)', padding: '4px 12px', fontSize: '0.85em', fontWeight: 'var(--weight-medium)', borderRadius: 'var(--radius-xl)', textTransform: 'none', letterSpacing: 0 },
    warn:   { background: 'rgba(249,168,37,0.12)', color: 'var(--warning)', padding: '4px 12px', fontSize: '0.85em', fontWeight: 'var(--weight-medium)', borderRadius: 'var(--radius-xl)', textTransform: 'none', letterSpacing: 0 },
    subtle: { background: 'var(--primary-alpha)', color: 'var(--primary)', padding: '0.15rem 0.5rem', fontSize: 'var(--text-2xs)', fontWeight: 'var(--weight-bold)', borderRadius: 'var(--radius-xl)', textTransform: 'uppercase', letterSpacing: '0.05em' },
    mode:   { background: 'var(--border)', color: 'var(--text-muted)', padding: '2px 8px', fontSize: 'var(--text-xs)', fontWeight: 'var(--weight-bold)', borderRadius: 'var(--radius-sm)', textTransform: 'uppercase', letterSpacing: 'var(--tracking-caps)' },
  };
  return <span style={{ display: 'inline-block', fontFamily: 'var(--font-sans)', ...variants[variant] || variants.subtle, ...style }}>{children}</span>;
}
