import React from 'react';

/** Collapsible <details> section. variant "advanced" = yellow expert zone; "info" = neutral. */
export function Drawer({ summary, variant = 'info', open = false, children, style = {} }) {
  const [isOpen, setOpen] = React.useState(open);
  const adv = variant === 'advanced';
  const color = adv ? 'var(--warning)' : 'var(--text-muted)';
  return (
    <article style={{ border: '1px solid ' + (adv ? 'var(--warning)' : 'var(--border)'),
      background: adv ? 'rgba(249,168,37,0.04)' : 'transparent',
      borderRadius: adv ? 'var(--radius-2xl)' : 'var(--radius-lg)', overflow: 'hidden',
      marginBottom: '0.75rem', padding: adv ? '1.25rem' : 0, fontFamily: 'var(--font-sans)', ...style }}>
      <div onClick={() => setOpen(o => !o)} style={{ display: 'flex', justifyContent: 'space-between',
        alignItems: 'center', cursor: 'pointer', userSelect: 'none', color,
        fontWeight: adv ? 'var(--weight-bold)' : 'var(--weight-semibold)',
        fontSize: adv ? 'var(--text-card-title)' : 'var(--text-body)',
        padding: adv ? '0.25rem 0 0.75rem' : '0.6rem 0.75rem' }}>
        <span>{summary}</span>
        <span style={{ width: 5, height: 5, borderRight: '2px solid currentColor', borderBottom: '2px solid currentColor',
          transform: 'rotate(' + (isOpen ? 45 : -45) + 'deg)', transition: 'transform var(--transition-base)', flexShrink: 0 }} />
      </div>
      {isOpen && <div style={{ padding: adv ? 0 : '0 0.75rem 0.5rem', fontSize: adv ? undefined : 'var(--text-base-sm)' }}>{children}</div>}
    </article>
  );
}
