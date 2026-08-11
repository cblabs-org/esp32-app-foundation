import React from 'react';

const VARIANTS = {
  primary:   { background: 'var(--primary)', color: '#fff', border: 'none' },
  outline:   { background: 'transparent', color: 'var(--primary)', border: '2px solid var(--primary)' },
  secondary: { background: 'var(--card-bg)', color: 'var(--text)', border: '1px solid var(--border)' },
  danger:    { background: 'transparent', color: 'var(--error)', border: '2px solid var(--error)' },
};

/** Primary action control. Variants: primary | outline | secondary | danger. */
export function Button({ variant = 'primary', busy = false, disabled = false, full = false, type = 'button', children, style = {}, ...rest }) {
  const off = disabled || busy;
  const s = {
    display: 'inline-flex', alignItems: 'center', justifyContent: 'center', gap: '0.5rem',
    fontSize: 'var(--text-md)', fontWeight: 'var(--weight-bold)', fontFamily: 'var(--font-sans)',
    padding: '0.8rem 1.25rem', borderRadius: 'var(--radius-lg)', lineHeight: 1,
    cursor: off ? 'not-allowed' : 'pointer', transition: 'var(--transition-base)',
    width: full ? '100%' : 'auto', boxSizing: 'border-box',
    ...VARIANTS[variant] || VARIANTS.primary, ...style,
    // disabled beats busy beats resting; applied last so it wins over style/variant
    opacity: disabled ? 0.5 : busy ? 0.7 : 1,
    filter: disabled ? 'grayscale(40%)' : undefined,
  };
  return (
    <button type={type} disabled={off} style={s}
      onMouseDown={e => { if (!off) e.currentTarget.style.transform = 'scale(0.98)'; }}
      onMouseUp={e => { e.currentTarget.style.transform = 'scale(1)'; }}
      onMouseLeave={e => { e.currentTarget.style.transform = 'scale(1)'; }} {...rest}>
      {busy && <Spinner variant={variant} />}
      {children}
    </button>
  );
}

function Spinner({ variant }) {
  const dark = variant === 'primary';
  return <span style={{ width: 14, height: 14, borderRadius: '50%',
    border: '2px solid ' + (dark ? 'rgba(255,255,255,0.4)' : 'rgba(0,0,0,0.2)'),
    borderTopColor: dark ? '#fff' : 'currentColor',
    display: 'inline-block', animation: 'af-spin 0.8s linear infinite' }} />;
}
