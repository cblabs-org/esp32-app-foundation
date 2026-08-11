import React from 'react';

const V = {
  success: { color: 'var(--success)' }, error: { color: 'var(--error)' },
  warning: { color: 'var(--warning)' }, info: { color: 'var(--primary)' },
};

/** Inline feedback box with a tinted background + coloured border. */
export function Alert({ variant = 'info', children, style = {}, ...rest }) {
  const c = (V[variant] || V.info).color;
  const bg = variant === 'info' ? 'var(--primary-alpha)' : 'color-mix(in srgb, ' + c + ' 10%, transparent)';
  return (
    <div role={variant === 'error' ? 'alert' : 'status'} style={{
      padding: '0.75rem 1rem', borderRadius: 'var(--radius-md)', border: '1px solid ' + c,
      background: bg, color: c, fontSize: 'var(--text-body)', marginBottom: '0.75rem',
      fontFamily: 'var(--font-sans)', lineHeight: 'var(--leading-body)', ...style,
    }} {...rest}>{children}</div>
  );
}
